#include "picocalc_lcd.h"

#include <stdbool.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "picocalc_lcd";

// Same-position physical RPico adapter mapping from 0099.
#define LCD_HOST               SPI2_HOST
#define LCD_PIN_SCK            3
#define LCD_PIN_MOSI           2
#define LCD_PIN_MISO           (-1)
#define LCD_PIN_CS             7
#define LCD_PIN_DC             24
#define LCD_PIN_RST            25

// The older RP2350 PicoCalc firmware defaulted to 75 MHz after testing. On the
// ESP32-P4, SPI_CLK_SRC_DEFAULT is XTAL (40 MHz), which rejects high SCLK values.
// 0099 validated the SPLL source and defaulted to 80 MHz.
#define LCD_DEFAULT_SPI_HZ        (80 * 1000 * 1000)
#define LCD_SPI_CLK_SRC           SPI_CLK_SRC_SPLL
#define LCD_SPI_MAX_TRANSFER_SZ   (32 * 1024)
#define LCD_FILL_DMA_CHUNK_BYTES  LCD_SPI_MAX_TRANSFER_SZ

#define LCD_CMD_SWRESET        0x01
#define LCD_CMD_SLPOUT         0x11
#define LCD_CMD_INVON          0x21
#define LCD_CMD_DISPON         0x29
#define LCD_CMD_CASET          0x2A
#define LCD_CMD_RASET          0x2B
#define LCD_CMD_RAMWR          0x2C
#define LCD_CMD_MADCTL         0x36
#define LCD_CMD_COLMOD         0x3A

static spi_device_handle_t s_lcd = NULL;
static bool s_lcd_initialized = false;
static int s_lcd_spi_hz = LCD_DEFAULT_SPI_HZ;
static uint8_t *s_lcd_dma_buf = NULL;
static size_t s_lcd_dma_buf_len = 0;

static esp_err_t lcd_tx(const void *data, size_t len)
{
    if (!s_lcd || !data || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    const uint8_t *p = (const uint8_t *)data;
    while (len > 0) {
        const size_t chunk = len > LCD_SPI_MAX_TRANSFER_SZ ? LCD_SPI_MAX_TRANSFER_SZ : len;
        spi_transaction_t t = {
            .length = chunk * 8,
            .tx_buffer = p,
        };
        esp_err_t err = spi_device_polling_transmit(s_lcd, &t);
        if (err != ESP_OK) {
            return err;
        }
        p += chunk;
        len -= chunk;
    }
    return ESP_OK;
}

static esp_err_t lcd_ensure_dma_buffer(size_t min_len)
{
    if (s_lcd_dma_buf && s_lcd_dma_buf_len >= min_len) {
        return ESP_OK;
    }

    if (s_lcd_dma_buf) {
        heap_caps_free(s_lcd_dma_buf);
        s_lcd_dma_buf = NULL;
        s_lcd_dma_buf_len = 0;
    }

    s_lcd_dma_buf = (uint8_t *)heap_caps_malloc(min_len, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!s_lcd_dma_buf) {
        return ESP_ERR_NO_MEM;
    }
    s_lcd_dma_buf_len = min_len;
    ESP_LOGI(TAG, "LCD DMA buffer allocated: %u bytes", (unsigned)s_lcd_dma_buf_len);
    return ESP_OK;
}

static esp_err_t lcd_cmd(uint8_t cmd)
{
    gpio_set_level(LCD_PIN_DC, 0);
    return lcd_tx(&cmd, 1);
}

static esp_err_t lcd_data(const void *data, size_t len)
{
    gpio_set_level(LCD_PIN_DC, 1);
    return lcd_tx(data, len);
}

static esp_err_t lcd_cmd_data(uint8_t cmd, const void *data, size_t len)
{
    ESP_RETURN_ON_ERROR(lcd_cmd(cmd), TAG, "cmd");
    if (data && len > 0) {
        return lcd_data(data, len);
    }
    return ESP_OK;
}

static void lcd_reset(void)
{
    gpio_set_level(LCD_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(LCD_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(30));
    gpio_set_level(LCD_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(150));
}

static esp_err_t lcd_add_device(void)
{
    if (s_lcd) {
        return ESP_OK;
    }

    spi_device_interface_config_t devcfg = {
        .clock_source = LCD_SPI_CLK_SRC,
        .clock_speed_hz = s_lcd_spi_hz,
        .mode = 0,
        .spics_io_num = LCD_PIN_CS,
        .queue_size = 4,
    };

    ESP_RETURN_ON_ERROR(spi_bus_add_device(LCD_HOST, &devcfg, &s_lcd), TAG, "add lcd device");

    int actual_khz = 0;
    esp_err_t err = spi_device_get_actual_freq(s_lcd, &actual_khz);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "LCD SPI device ready: clk_src=%d requested=%d Hz actual=%d kHz",
                 LCD_SPI_CLK_SRC, s_lcd_spi_hz, actual_khz);
    } else {
        ESP_LOGW(TAG, "LCD SPI device ready but actual freq unavailable: %s", esp_err_to_name(err));
    }
    return ESP_OK;
}

static esp_err_t lcd_init_bus_gpio_and_host(void)
{
    gpio_config_t out = {
        .pin_bit_mask = (1ULL << LCD_PIN_DC) | (1ULL << LCD_PIN_RST),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&out), TAG, "gpio config");
    gpio_set_level(LCD_PIN_DC, 0);
    gpio_set_level(LCD_PIN_RST, 1);

    spi_bus_config_t buscfg = {
        .mosi_io_num = LCD_PIN_MOSI,
        .miso_io_num = LCD_PIN_MISO,
        .sclk_io_num = LCD_PIN_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_SPI_MAX_TRANSFER_SZ,
    };

    esp_err_t err = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }
    return ESP_OK;
}

static esp_err_t lcd_init_bus(void)
{
    ESP_RETURN_ON_ERROR(lcd_init_bus_gpio_and_host(), TAG, "lcd bus gpio/host");
    ESP_RETURN_ON_ERROR(lcd_add_device(), TAG, "add lcd device");
    ESP_LOGI(TAG, "LCD SPI ready: sck=%d mosi=%d cs=%d dc=%d rst=%d clk_src=%d requested_hz=%d max_transfer=%d",
             LCD_PIN_SCK, LCD_PIN_MOSI, LCD_PIN_CS, LCD_PIN_DC, LCD_PIN_RST,
             LCD_SPI_CLK_SRC, s_lcd_spi_hz, LCD_SPI_MAX_TRANSFER_SZ);
    return ESP_OK;
}

static esp_err_t lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t col[] = {x0 >> 8, x0 & 0xff, x1 >> 8, x1 & 0xff};
    uint8_t row[] = {y0 >> 8, y0 & 0xff, y1 >> 8, y1 & 0xff};
    ESP_RETURN_ON_ERROR(lcd_cmd_data(LCD_CMD_CASET, col, sizeof(col)), TAG, "caset");
    ESP_RETURN_ON_ERROR(lcd_cmd_data(LCD_CMD_RASET, row, sizeof(row)), TAG, "raset");
    return lcd_cmd(LCD_CMD_RAMWR);
}

esp_err_t picocalc_lcd_init(void)
{
    if (s_lcd_initialized) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(lcd_init_bus(), TAG, "lcd_init_bus");

    lcd_reset();
    ESP_RETURN_ON_ERROR(lcd_cmd(LCD_CMD_SWRESET), TAG, "swreset");
    vTaskDelay(pdMS_TO_TICKS(150));
    ESP_RETURN_ON_ERROR(lcd_cmd(LCD_CMD_SLPOUT), TAG, "sleep out");
    vTaskDelay(pdMS_TO_TICKS(150));

    const uint8_t colmod = 0x55; // RGB565
    ESP_RETURN_ON_ERROR(lcd_cmd_data(LCD_CMD_COLMOD, &colmod, 1), TAG, "colmod");
    vTaskDelay(pdMS_TO_TICKS(10));

    const uint8_t madctl = 0x48; // MX | BGR, matching 0099
    ESP_RETURN_ON_ERROR(lcd_cmd_data(LCD_CMD_MADCTL, &madctl, 1), TAG, "madctl");
    ESP_RETURN_ON_ERROR(lcd_cmd(LCD_CMD_INVON), TAG, "invon");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(lcd_cmd(LCD_CMD_DISPON), TAG, "display on");
    vTaskDelay(pdMS_TO_TICKS(50));

    s_lcd_initialized = true;
    ESP_LOGI(TAG, "LCD panel initialized (%dx%d RGB565)", PICOCALC_LCD_WIDTH, PICOCALC_LCD_HEIGHT);
    return ESP_OK;
}

esp_err_t picocalc_lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rgb565)
{
    if (!s_lcd_initialized) {
        ESP_RETURN_ON_ERROR(picocalc_lcd_init(), TAG, "lcd init");
    }
    if (x >= PICOCALC_LCD_WIDTH || y >= PICOCALC_LCD_HEIGHT || w == 0 || h == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (x + w > PICOCALC_LCD_WIDTH) {
        w = PICOCALC_LCD_WIDTH - x;
    }
    if (y + h > PICOCALC_LCD_HEIGHT) {
        h = PICOCALC_LCD_HEIGHT - y;
    }

    ESP_RETURN_ON_ERROR(lcd_set_window(x, y, x + w - 1, y + h - 1), TAG, "window");
    ESP_RETURN_ON_ERROR(lcd_ensure_dma_buffer(LCD_FILL_DMA_CHUNK_BYTES), TAG, "alloc fill dma buffer");

    for (size_t i = 0; i < s_lcd_dma_buf_len; i += 2) {
        s_lcd_dma_buf[i] = (uint8_t)(rgb565 >> 8);
        s_lcd_dma_buf[i + 1] = (uint8_t)(rgb565 & 0xff);
    }

    size_t bytes = (size_t)w * (size_t)h * 2;
    gpio_set_level(LCD_PIN_DC, 1);
    while (bytes > 0) {
        const size_t chunk = bytes > s_lcd_dma_buf_len ? s_lcd_dma_buf_len : bytes;
        ESP_RETURN_ON_ERROR(lcd_tx(s_lcd_dma_buf, chunk), TAG, "pixels");
        bytes -= chunk;
    }
    return ESP_OK;
}

esp_err_t picocalc_lcd_fill(uint16_t rgb565)
{
    return picocalc_lcd_fill_rect(0, 0, PICOCALC_LCD_WIDTH, PICOCALC_LCD_HEIGHT, rgb565);
}

esp_err_t picocalc_lcd_blit_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                                  const uint16_t *pixels, size_t pixel_count)
{
    if (!pixels || w == 0 || h == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if ((size_t)w * (size_t)h > pixel_count) {
        return ESP_ERR_INVALID_SIZE;
    }
    if (!s_lcd_initialized) {
        ESP_RETURN_ON_ERROR(picocalc_lcd_init(), TAG, "lcd init");
    }
    if (x >= PICOCALC_LCD_WIDTH || y >= PICOCALC_LCD_HEIGHT ||
        x + w > PICOCALC_LCD_WIDTH || y + h > PICOCALC_LCD_HEIGHT) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(lcd_set_window(x, y, x + w - 1, y + h - 1), TAG, "window");
    gpio_set_level(LCD_PIN_DC, 1);
    return lcd_tx(pixels, (size_t)w * (size_t)h * 2);
}

esp_err_t picocalc_lcd_blit_row(uint16_t y, uint16_t h, const uint16_t *pixels, size_t pixel_count)
{
    return picocalc_lcd_blit_rect(0, y, PICOCALC_LCD_WIDTH, h, pixels, pixel_count);
}

int picocalc_lcd_actual_khz(void)
{
    int actual_khz = 0;
    if (s_lcd && spi_device_get_actual_freq(s_lcd, &actual_khz) == ESP_OK) {
        return actual_khz;
    }
    return 0;
}

int picocalc_lcd_requested_hz(void)
{
    return s_lcd_spi_hz;
}

size_t picocalc_lcd_max_transfer_bytes(void)
{
    return LCD_SPI_MAX_TRANSFER_SZ;
}
