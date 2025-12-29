/*
 * ESP32-S3 MicroQuickJS Serial REPL with SPIFFS Storage
 * 
 * A JavaScript REPL with flash storage for loading libraries on startup
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "driver/uart.h"

#include "mquickjs.h"
#include "cutils.h"
#include "minimal_stdlib.h"

static const char *TAG = "mqjs-repl";

#define UART_NUM UART_NUM_0
#define BUF_SIZE (1024)
#define REPL_PROMPT "js> "
#define SPIFFS_BASE_PATH "/spiffs"
#define AUTOLOAD_DIR "/spiffs/autoload"

// Memory buffer for JavaScript engine
#define JS_MEM_SIZE (64 * 1024)  // 64KB for JS engine
static uint8_t js_mem_buf[JS_MEM_SIZE];
static JSContext *js_ctx = NULL;

// Input line buffer
static char line_buf[512];
static int line_pos = 0;

static void js_log_func(void *opaque, const void *buf, size_t buf_len)
{
    printf("%.*s", (int)buf_len, (const char *)buf);
}

static esp_err_t init_spiffs(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");
    
    esp_vfs_spiffs_conf_t conf = {
        .base_path = SPIFFS_BASE_PATH,
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = true
    };
    
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }
    
    size_t total = 0, used = 0;
    ret = esp_spiffs_info("storage", &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS: total=%d bytes, used=%d bytes", total, used);
    }
    
    return ESP_OK;
}

static char* load_file(const char *filename, size_t *out_size)
{
    FILE *f = fopen(filename, "r");
    if (!f) {
        ESP_LOGW(TAG, "Failed to open file: %s", filename);
        return NULL;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (fsize <= 0) {
        fclose(f);
        return NULL;
    }
    
    char *content = malloc(fsize + 1);
    if (!content) {
        fclose(f);
        ESP_LOGE(TAG, "Failed to allocate memory for file");
        return NULL;
    }
    
    size_t read_size = fread(content, 1, fsize, f);
    fclose(f);
    
    content[read_size] = '\0';
    if (out_size) {
        *out_size = read_size;
    }
    
    return content;
}

static void load_js_file(const char *filepath)
{
    ESP_LOGI(TAG, "Loading JavaScript file: %s", filepath);
    
    size_t size;
    char *content = load_file(filepath, &size);
    if (!content) {
        return;
    }
    
    JSValue val = JS_Eval(js_ctx, content, size, filepath, 0);
    
    if (JS_IsException(val)) {
        JSValue obj = JS_GetException(js_ctx);
        printf("Error loading %s: ", filepath);
        JS_PrintValueF(js_ctx, obj, JS_DUMP_LONG);
        printf("\n");
    } else {
        ESP_LOGI(TAG, "Successfully loaded: %s", filepath);
    }
    
    free(content);
}

static void load_autoload_libraries(void)
{
    DIR *dir = opendir(AUTOLOAD_DIR);
    if (!dir) {
        ESP_LOGW(TAG, "Autoload directory not found: %s", AUTOLOAD_DIR);
        ESP_LOGI(TAG, "Creating autoload directory...");
        mkdir(AUTOLOAD_DIR, 0755);
        return;
    }
    
    ESP_LOGI(TAG, "Loading libraries from %s", AUTOLOAD_DIR);
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Only load .js files
        size_t name_len = strlen(entry->d_name);
        if (name_len > 3 && strcmp(entry->d_name + name_len - 3, ".js") == 0) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", AUTOLOAD_DIR, entry->d_name);
            load_js_file(filepath);
        }
    }
    
    closedir(dir);
}

static void create_example_libraries(void)
{
    ESP_LOGI(TAG, "Creating example JavaScript libraries");
    
    // Create math utilities library
    const char *math_lib = 
        "var MathUtils = {};\n"
        "MathUtils.square = function(x) { return x * x; };\n"
        "MathUtils.cube = function(x) { return x * x * x; };\n"
        "MathUtils.factorial = function(n) {\n"
        "    if (n <= 1) return 1;\n"
        "    return n * MathUtils.factorial(n - 1);\n"
        "};\n"
        "MathUtils.fibonacci = function(n) {\n"
        "    if (n <= 1) return n;\n"
        "    return MathUtils.fibonacci(n - 1) + MathUtils.fibonacci(n - 2);\n"
        "};\n";
    
    // Create string utilities library
    const char *string_lib =
        "var StringUtils = {};\n"
        "StringUtils.reverse = function(str) {\n"
        "    var result = '';\n"
        "    for (var i = str.length - 1; i >= 0; i--) {\n"
        "        result = result + str[i];\n"
        "    }\n"
        "    return result;\n"
        "};\n"
        "StringUtils.repeat = function(str, n) {\n"
        "    var result = '';\n"
        "    for (var i = 0; i < n; i++) {\n"
        "        result = result + str;\n"
        "    }\n"
        "    return result;\n"
        "};\n";
    
    // Create array utilities library
    const char *array_lib =
        "var ArrayUtils = {};\n"
        "ArrayUtils.sum = function(arr) {\n"
        "    var total = 0;\n"
        "    for (var i = 0; i < arr.length; i++) {\n"
        "        total = total + arr[i];\n"
        "    }\n"
        "    return total;\n"
        "};\n"
        "ArrayUtils.max = function(arr) {\n"
        "    var m = arr[0];\n"
        "    for (var i = 1; i < arr.length; i++) {\n"
        "        if (arr[i] > m) m = arr[i];\n"
        "    }\n"
        "    return m;\n"
        "};\n"
        "ArrayUtils.min = function(arr) {\n"
        "    var m = arr[0];\n"
        "    for (var i = 1; i < arr.length; i++) {\n"
        "        if (arr[i] < m) m = arr[i];\n"
        "    }\n"
        "    return m;\n"
        "};\n";
    
    // Write libraries to SPIFFS
    FILE *f;
    
    f = fopen("/spiffs/autoload/math.js", "w");
    if (f) {
        fwrite(math_lib, 1, strlen(math_lib), f);
        fclose(f);
        ESP_LOGI(TAG, "Created math.js");
    }
    
    f = fopen("/spiffs/autoload/string.js", "w");
    if (f) {
        fwrite(string_lib, 1, strlen(string_lib), f);
        fclose(f);
        ESP_LOGI(TAG, "Created string.js");
    }
    
    f = fopen("/spiffs/autoload/array.js", "w");
    if (f) {
        fwrite(array_lib, 1, strlen(array_lib), f);
        fclose(f);
        ESP_LOGI(TAG, "Created array.js");
    }
}

static void init_js_engine(void)
{
    ESP_LOGI(TAG, "Initializing JavaScript engine with %d bytes", JS_MEM_SIZE);
    
    // Create context with minimal stdlib
    js_ctx = JS_NewContext(js_mem_buf, JS_MEM_SIZE, &js_stdlib);
    if (!js_ctx) {
        ESP_LOGE(TAG, "Failed to create JavaScript context");
        return;
    }
    
    JS_SetLogFunc(js_ctx, js_log_func);
    
    ESP_LOGI(TAG, "JavaScript engine initialized successfully");
}

static void eval_and_print(const char *code)
{
    JSValue val;
    
    if (!js_ctx) {
        printf("Error: JavaScript engine not initialized\n");
        return;
    }
    
    val = JS_Eval(js_ctx, code, strlen(code), "<input>", 0);
    
    if (JS_IsException(val)) {
        JSValue obj = JS_GetException(js_ctx);
        printf("Error: ");
        JS_PrintValueF(js_ctx, obj, JS_DUMP_LONG);
        printf("\n");
    } else if (!JS_IsUndefined(val)) {
        JS_PrintValueF(js_ctx, val, JS_DUMP_LONG);
        printf("\n");
    }
}

static void process_char(char c)
{
    if (c == '\n' || c == '\r') {
        if (line_pos > 0) {
            printf("\n");
            line_buf[line_pos] = '\0';
            eval_and_print(line_buf);
            line_pos = 0;
        }
        printf(REPL_PROMPT);
        fflush(stdout);
    } else if (c == '\b' || c == 127) {  // Backspace or DEL
        if (line_pos > 0) {
            line_pos--;
            printf("\b \b");  // Erase character on screen
            fflush(stdout);
        }
    } else if (c >= 32 && c < 127) {  // Printable characters
        if (line_pos < sizeof(line_buf) - 1) {
            line_buf[line_pos++] = c;
            putchar(c);
            fflush(stdout);
        }
    }
}

void repl_task(void *pvParameters)
{
    uint8_t data[BUF_SIZE];
    
    // Print banner
    printf("\n");
    printf("========================================\n");
    printf("  ESP32-S3 MicroQuickJS REPL\n");
    printf("  with Flash Storage Support\n");
    printf("========================================\n");
    printf("JavaScript Engine: MicroQuickJS\n");
    printf("Memory: %d bytes\n", JS_MEM_SIZE);
    printf("Free heap: %lu bytes\n", esp_get_free_heap_size());
    printf("========================================\n");
    printf("Loaded libraries:\n");
    printf("  - MathUtils (square, cube, factorial, fibonacci)\n");
    printf("  - StringUtils (reverse, repeat)\n");
    printf("  - ArrayUtils (sum, max, min)\n");
    printf("========================================\n");
    printf("Try: MathUtils.factorial(5)\n");
    printf("     StringUtils.reverse('hello')\n");
    printf("     ArrayUtils.sum([1,2,3,4,5])\n");
    printf("========================================\n\n");
    printf(REPL_PROMPT);
    fflush(stdout);
    
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE, portMAX_DELAY);
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                process_char(data[i]);
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ESP32-S3 MicroQuickJS REPL with Storage");
    
    // Initialize SPIFFS
    if (init_spiffs() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPIFFS");
        return;
    }
    
    // Check if autoload directory exists and version file
    struct stat st;
    bool need_create = false;
    
    if (stat(AUTOLOAD_DIR, &st) != 0) {
        ESP_LOGI(TAG, "First boot - creating example libraries");
        need_create = true;
    } else {
        // Check version file
        FILE *vf = fopen("/spiffs/version.txt", "r");
        if (!vf) {
            ESP_LOGI(TAG, "No version file - recreating libraries");
            need_create = true;
        } else {
            char version[32];
            fgets(version, sizeof(version), vf);
            fclose(vf);
            if (strcmp(version, "2\n") != 0) {
                ESP_LOGI(TAG, "Old version - recreating libraries");
                need_create = true;
            }
        }
    }
    
    if (need_create) {
        create_example_libraries();
        // Write version file
        FILE *vf = fopen("/spiffs/version.txt", "w");
        if (vf) {
            fprintf(vf, "2\n");
            fclose(vf);
        }
    }
    
    // Configure UART
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    
    // Initialize JavaScript engine
    init_js_engine();
    
    // Load libraries from autoload directory
    load_autoload_libraries();
    
    // Create REPL task
    xTaskCreate(repl_task, "repl_task", 8192, NULL, 5, NULL);
}
