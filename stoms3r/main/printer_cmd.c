/*
 * printer_cmd.c — Console commands for the thermal printer.
 *
 * Registers the following commands with esp_console:
 *   printer_init         — Reset printer
 *   printer_text <str>   — Print a text line
 *   printer_feed [n]     — Feed n lines (default 3)
 *   printer_size <n>     — Set font size 0–7
 *   printer_bold <on|off>— Enable/disable bold
 *   printer_align <n>    — Set alignment 0=left 1=center 2=right
 *   printer_barcode <type> <data> — Print barcode
 *   printer_qr <text>    — Print QR code
 *   printer_bitmap_test  — Print test pattern (alternating lines)
 *   printer_baud <rate>  — Change ESP32 UART baud only (recovery)
 *   set_baudrate <rate>  — Tell printer to change baud, then switch ESP32 UART
 */

#include "printer_cmd.h"

#include <stdlib.h>
#include <string.h>
#include "argtable3/argtable3.h"
#include "driver/uart.h"
#include "esp_console.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "printer_drv.h"

/* TAG used for future logging in this module */
#define TAG "printer_cmd"

/* ========================================================================
 * printer_init
 * ======================================================================== */

static int do_printer_init(int argc, char **argv)
{
    esp_err_t err = printer_drv_reset();
    if (err != ESP_OK) {
        printf("Error: %s\n", esp_err_to_name(err));
        return 1;
    }
    printf("Printer reset OK\n");
    return 0;
}

/* ========================================================================
 * printer_text <string>
 * ======================================================================== */

static struct {
    struct arg_str *text;
    struct arg_end *end;
} text_args;

static int do_printer_text(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&text_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, text_args.end, argv[0]);
        return 1;
    }
    esp_err_t err = printer_drv_print_text(text_args.text->sval[0]);
    if (err != ESP_OK) {
        printf("Error: %s\n", esp_err_to_name(err));
        return 1;
    }
    return 0;
}

/* ========================================================================
 * printer_feed [n]
 * ======================================================================== */

static struct {
    struct arg_int *lines;
    struct arg_end *end;
} feed_args;

static int do_printer_feed(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&feed_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, feed_args.end, argv[0]);
        return 1;
    }
    int lines = (feed_args.lines->count > 0) ? feed_args.lines->ival[0] : 3;
    if (lines < 1) lines = 1;
    if (lines > 255) lines = 255;
    esp_err_t err = printer_drv_feed((uint8_t)lines);
    if (err != ESP_OK) {
        printf("Error: %s\n", esp_err_to_name(err));
        return 1;
    }
    return 0;
}

/* ========================================================================
 * printer_size <n>
 * ======================================================================== */

static struct {
    struct arg_int *size;
    struct arg_end *end;
} size_args;

static int do_printer_size(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&size_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, size_args.end, argv[0]);
        return 1;
    }
    int sz = size_args.size->ival[0];
    if (sz < 0) sz = 0;
    if (sz > 7) sz = 7;
    esp_err_t err = printer_drv_set_font_size((uint8_t)sz);
    if (err != ESP_OK) {
        printf("Error: %s\n", esp_err_to_name(err));
        return 1;
    }
    printf("Font size set to %d\n", sz);
    return 0;
}

/* ========================================================================
 * printer_bold <on|off>
 * ======================================================================== */

static struct {
    struct arg_str *state;
    struct arg_end *end;
} bold_args;

static int do_printer_bold(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&bold_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, bold_args.end, argv[0]);
        return 1;
    }
    const char *val = bold_args.state->sval[0];
    bool on;
    if (strcmp(val, "on") == 0) {
        on = true;
    } else if (strcmp(val, "off") == 0) {
        on = false;
    } else {
        printf("Usage: printer_bold <on|off>\n");
        return 1;
    }
    esp_err_t err = printer_drv_set_bold(on);
    if (err != ESP_OK) {
        printf("Error: %s\n", esp_err_to_name(err));
        return 1;
    }
    printf("Bold %s\n", on ? "on" : "off");
    return 0;
}

/* ========================================================================
 * printer_align <n>
 * ======================================================================== */

static struct {
    struct arg_int *align;
    struct arg_end *end;
} align_args;

static int do_printer_align(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&align_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, align_args.end, argv[0]);
        return 1;
    }
    int a = align_args.align->ival[0];
    if (a < 0) a = 0;
    if (a > 2) a = 2;
    esp_err_t err = printer_drv_set_align((uint8_t)a);
    if (err != ESP_OK) {
        printf("Error: %s\n", esp_err_to_name(err));
        return 1;
    }
    printf("Alignment set to %d (%s)\n", a,
           a == 0 ? "left" : a == 1 ? "center" : "right");
    return 0;
}

/* ========================================================================
 * printer_barcode <type> <data>
 * ======================================================================== */

static struct {
    struct arg_str *type;
    struct arg_str *data;
    struct arg_end *end;
} barcode_args;

static int barcode_type_from_str(const char *s, printer_barcode_type_t *out)
{
    if (strcasecmp(s, "UPC_A")    == 0) { *out = PRINTER_BC_UPC_A;   return 0; }
    if (strcasecmp(s, "UPC_E")    == 0) { *out = PRINTER_BC_UPC_E;   return 0; }
    if (strcasecmp(s, "EAN13")    == 0) { *out = PRINTER_BC_EAN13;   return 0; }
    if (strcasecmp(s, "EAN8")     == 0) { *out = PRINTER_BC_EAN8;    return 0; }
    if (strcasecmp(s, "CODE39")   == 0) { *out = PRINTER_BC_CODE39;  return 0; }
    if (strcasecmp(s, "ITF")      == 0) { *out = PRINTER_BC_ITF;     return 0; }
    if (strcasecmp(s, "CODABAR")  == 0) { *out = PRINTER_BC_CODABAR; return 0; }
    if (strcasecmp(s, "CODE93")   == 0) { *out = PRINTER_BC_CODE93;  return 0; }
    if (strcasecmp(s, "CODE128")  == 0) { *out = PRINTER_BC_CODE128; return 0; }
    return -1;
}

static int do_printer_barcode(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&barcode_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, barcode_args.end, argv[0]);
        return 1;
    }

    printer_barcode_type_t type;
    if (barcode_type_from_str(barcode_args.type->sval[0], &type) != 0) {
        printf("Unknown barcode type: %s\n", barcode_args.type->sval[0]);
        printf("Valid types: UPC_A UPC_E EAN13 EAN8 CODE39 ITF CODABAR CODE93 CODE128\n");
        return 1;
    }

    esp_err_t err = printer_drv_print_barcode(type, barcode_args.data->sval[0]);
    if (err != ESP_OK) {
        printf("Error: %s\n", esp_err_to_name(err));
        return 1;
    }
    return 0;
}

/* ========================================================================
 * printer_qr <text>
 * ======================================================================== */

static struct {
    struct arg_str *data;
    struct arg_end *end;
} qr_args;

static int do_printer_qr(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&qr_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, qr_args.end, argv[0]);
        return 1;
    }
    esp_err_t err = printer_drv_print_qr(PRINTER_QR_EC_M, qr_args.data->sval[0]);
    if (err != ESP_OK) {
        printf("Error: %s\n", esp_err_to_name(err));
        return 1;
    }
    return 0;
}

/* ========================================================================
 * printer_bitmap_test — print alternating-line test pattern
 * ======================================================================== */

static int do_printer_bitmap_test(int argc, char **argv)
{
    const int width_dots    = 384;
    const int bytes_per_row = width_dots / 8; /* 48 */
    const int height        = 100;

    printf("Printing test bitmap: %d x %d pixels...\n", width_dots, height);

    /* Build the bitmap in a buffer.  48 * 100 = 4800 bytes — fits in
     * internal SRAM easily, but we use PSRAM via heap_caps for
     * consistency with larger future images. */
    size_t total = (size_t)bytes_per_row * height;
    uint8_t *bitmap = (uint8_t *)malloc(total);
    if (!bitmap) {
        printf("Failed to allocate %zu bytes for bitmap\n", total);
        return 1;
    }

    for (int row = 0; row < height; row++) {
        if (row % 2 == 0) {
            memset(bitmap + row * bytes_per_row, 0xFF, bytes_per_row);
        } else {
            memset(bitmap + row * bytes_per_row, 0x00, bytes_per_row);
        }
    }

    esp_err_t err = printer_drv_print_bitmap(width_dots, height, bitmap);
    free(bitmap);

    if (err != ESP_OK) {
        printf("Error: %s\n", esp_err_to_name(err));
        return 1;
    }
    printf("Bitmap sent\n");
    return 0;
}

/* ========================================================================
 * printer_probe — query printer status and dump RX buffer
 * ======================================================================== */

static int do_printer_probe(int argc, char **argv)
{
    printf("=== Printer probe ===\n");
    printf("UART config: port=UART%d TX=GPIO%d RX=GPIO%d CTS=GPIO%d baud=%d%s\n",
           PRINTER_UART_NUM, PRINTER_TX_GPIO, PRINTER_RX_GPIO, PRINTER_CTS_GPIO,
           printer_drv_get_baud(),
           printer_drv_is_swapped() ? " (SWAPPED)" : "");

    /* 1. Drain any stale RX bytes */
    printf("\nDraining RX buffer...\n");
    int drained = printer_drv_drain_rx();
    printf("Drained %d stale bytes\n", drained);

    /* 2. Query real-time status: DLE EOT n for n=1..4 */
    printf("\nQuerying printer status (4 DLE EOT queries)...\n");
    bool any_response = false;
    for (uint8_t n = 1; n <= 4; n++) {
        uint8_t resp = 0;
        esp_err_t err = printer_drv_query_status(n, &resp);
        if (err == ESP_OK) {
            any_response = true;
            const char *name[] = { "", "printer", "offline", "error", "paper" };
            printf("  Status n=%d (%s): 0x%02X\n", n, name[n], resp);
        } else {
            printf("  Status n=%d: NO RESPONSE\n", n);
        }
    }

    /* 3. Send ESC @ (init) and check for response */
    printf("\nSending ESC @ (init)...\n");
    esp_err_t reset_err = printer_drv_reset();
    printf("  send result: %s\n", esp_err_to_name(reset_err));

    /* 4. Wait and drain again */
    vTaskDelay(pdMS_TO_TICKS(200));
    int after = printer_drv_drain_rx();
    if (after > 0) {
        printf("  Got %d bytes after init (printer is alive!)\n", after);
        any_response = true;
    }

    printf("\n=== Result: %s ===\n",
           any_response ? "PRINTER RESPONDED" : "NO RESPONSE (check wiring/power)");

    if (!any_response) {
        printf("\nTroubleshooting:\n");
        printf("  1. Is 12V power supply connected to the printer carrier board?\n");
        printf("  2. Is the HY2.0-4P cable plugged in?\n");
        printf("  3. Check TX<->RX crossover: ESP TX(GPIO%d) -> printer RX\n", PRINTER_TX_GPIO);
        printf("  4. GND must be shared between AtomS3R and printer board\n");
        printf("  5. Are GPIO%d/GPIO%d/GPIO%d the correct pins for your K118 cable?\n",
               PRINTER_TX_GPIO, PRINTER_RX_GPIO, PRINTER_CTS_GPIO);
        printf("     (K118 designed for ATOM Lite: TX=GPIO23 RX=GPIO33 CTS=GPIO19)\n");
        printf("  6. Try 'printer_swap on' to flip TX<->RX and re-probe\n");
    }

    return any_response ? 0 : 1;
}

/* ========================================================================
 * printer_raw <hex> — send raw hex bytes
 * ======================================================================== */

static struct {
    struct arg_str *hex;
    struct arg_end *end;
} raw_args;

static int do_printer_raw(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&raw_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, raw_args.end, argv[0]);
        return 1;
    }

    /* Parse hex string like "1B40" or "1B 40" */
    const char *hex = raw_args.hex->sval[0];
    size_t hex_len = strlen(hex);
    uint8_t buf[256];
    size_t buf_len = 0;

    /* Remove spaces */
    char clean[256];
    size_t ci = 0;
    for (size_t i = 0; i < hex_len && ci < sizeof(clean) - 1; i++) {
        if (hex[i] != ' ') clean[ci++] = hex[i];
    }
    clean[ci] = '\0';

    if (ci % 2 != 0) {
        printf("Hex string must have even length (got %zu)\n", ci);
        return 1;
    }

    for (size_t i = 0; i < ci; i += 2) {
        unsigned int val;
        if (sscanf(clean + i, "%2x", &val) != 1) {
            printf("Invalid hex at position %zu\n", i);
            return 1;
        }
        buf[buf_len++] = (uint8_t)val;
    }

    printf("Sending %zu raw bytes\n", buf_len);
    esp_err_t err = printer_drv_send_raw(buf, buf_len);
    if (err != ESP_OK) {
        printf("Error: %s\n", esp_err_to_name(err));
        return 1;
    }

    /* Wait and check for response */
    vTaskDelay(pdMS_TO_TICKS(200));
    int got = printer_drv_drain_rx();
    if (got > 0) printf("(printer sent %d bytes back)\n", got);
    return 0;
}

/* ========================================================================
 * printer_swap <on|off> — toggle TX<->RX crossover
 * ======================================================================== */

static struct {
    struct arg_str *state;
    struct arg_end *end;
} swap_args;

static int do_printer_swap(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&swap_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, swap_args.end, argv[0]);
        return 1;
    }
    const char *val = swap_args.state->sval[0];
    bool swap;
    if (strcmp(val, "on") == 0) {
        swap = true;
    } else if (strcmp(val, "off") == 0) {
        swap = false;
    } else {
        printf("Usage: printer_swap <on|off>\n");
        return 1;
    }
    esp_err_t err = printer_drv_swap_pins(swap);
    if (err != ESP_OK) {
        printf("Error: %s\n", esp_err_to_name(err));
        return 1;
    }
    printf("Pins %s: TX=GPIO%d RX=GPIO%d\n",
           swap ? "SWAPPED" : "NORMAL",
           swap ? PRINTER_RX_GPIO : PRINTER_TX_GPIO,
           swap ? PRINTER_TX_GPIO : PRINTER_RX_GPIO);
    return 0;
}

/* ========================================================================
 * printer_baud <rate> — change ESP32 UART baud rate only (recovery)
 * ======================================================================== */

static struct {
    struct arg_int *rate;
    struct arg_end *end;
} baud_args;

static const char *supported_baud_list(void)
{
    return "9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600";
}

static bool is_supported_baud(int rate)
{
    switch (rate) {
        case 9600:
        case 19200:
        case 38400:
        case 57600:
        case 115200:
        case 230400:
        case 460800:
        case 921600:
            return true;
        default:
            return false;
    }
}

static bool is_experimental_baud(int rate)
{
    return rate > 115200;
}

static int do_printer_baud(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&baud_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, baud_args.end, argv[0]);
        return 1;
    }
    int rate = baud_args.rate->ival[0];
    if (!is_supported_baud(rate)) {
        printf("Unsupported baud rate: %d\n", rate);
        printf("Supported: %s\n", supported_baud_list());
        return 1;
    }
    if (is_experimental_baud(rate)) {
        printf("Warning: rates above 115200 are experimental on K118.\n");
    }
    esp_err_t err = printer_drv_set_baud(rate);
    if (err != ESP_OK) {
        printf("Error: %s\n", esp_err_to_name(err));
        return 1;
    }
    printf("ESP32 UART baud rate set to %d (printer was NOT commanded)\n", rate);
    return 0;
}

/* ========================================================================
 * set_baudrate <rate> — command printer baud, then switch ESP32 UART
 * ======================================================================== */

static struct {
    struct arg_int *rate;
    struct arg_end *end;
} set_baudrate_args;

static int do_set_baudrate(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&set_baudrate_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, set_baudrate_args.end, argv[0]);
        return 1;
    }
    int rate = set_baudrate_args.rate->ival[0];
    if (!is_supported_baud(rate)) {
        printf("Unsupported baud rate: %d\n", rate);
        printf("Supported: %s\n", supported_baud_list());
        return 1;
    }

    printf("Sending K118 Set Baud Rate command for %d baud...\n", rate);
    if (is_experimental_baud(rate)) {
        printf("Warning: rates above 115200 are experimental on K118; be ready to power-cycle.\n");
    }
    printf("If communication is lost, use printer_baud <rate> to resync the ESP32 side,\n");
    printf("or power-cycle the printer to return it to its default 9600 baud.\n");

    esp_err_t err = printer_drv_set_printer_baudrate(rate);
    if (err != ESP_OK) {
        printf("Error: %s\n", esp_err_to_name(err));
        return 1;
    }
    printf("Printer and ESP32 UART baud rate set to %d\n", rate);
    return 0;
}

/* ========================================================================
 * Registration
 * ======================================================================== */

static void reg(const char *name, const char *help,
                esp_console_cmd_func_t func, void *argtable)
{
    const esp_console_cmd_t cmd = {
        .command  = name,
        .help     = help,
        .func     = func,
        .argtable = argtable,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void printer_cmd_register(void)
{
    /* ---- printer_init (no args) ---- */
    reg("printer_init", "Reset the thermal printer (ESC @)",
        do_printer_init, NULL);

    /* ---- printer_text ---- */
    text_args.text = arg_str1(NULL, NULL, "<text>", "Text to print");
    text_args.end  = arg_end(1);
    reg("printer_text", "Print a line of text", do_printer_text, &text_args);

    /* ---- printer_feed ---- */
    feed_args.lines = arg_int0(NULL, NULL, "[<n>]", "Number of lines to feed (default 3)");
    feed_args.end   = arg_end(1);
    reg("printer_feed", "Feed paper by n lines", do_printer_feed, &feed_args);

    /* ---- printer_size ---- */
    size_args.size = arg_int1(NULL, NULL, "<n>", "Font size 0-7");
    size_args.end  = arg_end(1);
    reg("printer_size", "Set font size (0-7)", do_printer_size, &size_args);

    /* ---- printer_bold ---- */
    bold_args.state = arg_str1(NULL, NULL, "<on|off>", "Enable or disable bold");
    bold_args.end   = arg_end(1);
    reg("printer_bold", "Enable/disable bold text", do_printer_bold, &bold_args);

    /* ---- printer_align ---- */
    align_args.align = arg_int1(NULL, NULL, "<n>", "Alignment: 0=left 1=center 2=right");
    align_args.end   = arg_end(1);
    reg("printer_align", "Set text alignment", do_printer_align, &align_args);

    /* ---- printer_barcode ---- */
    barcode_args.type = arg_str1(NULL, NULL, "<type>", "Barcode type (CODE128, EAN13, ...)");
    barcode_args.data = arg_str1(NULL, NULL, "<data>", "Barcode data string");
    barcode_args.end  = arg_end(2);
    reg("printer_barcode", "Print a barcode", do_printer_barcode, &barcode_args);

    /* ---- printer_qr ---- */
    qr_args.data = arg_str1(NULL, NULL, "<text>", "QR code content");
    qr_args.end  = arg_end(1);
    reg("printer_qr", "Print a QR code", do_printer_qr, &qr_args);

    /* ---- printer_bitmap_test (no args) ---- */
    reg("printer_bitmap_test", "Print a test bitmap pattern (alternating lines)",
        do_printer_bitmap_test, NULL);

    /* ---- printer_probe (no args) ---- */
    reg("printer_probe", "Query printer status and diagnose connection",
        do_printer_probe, NULL);

    /* ---- printer_raw <hex> ---- */
    raw_args.hex = arg_str1(NULL, NULL, "<hex>", "Hex bytes to send, e.g. 1B40 or \"1B 40\"");
    raw_args.end = arg_end(1);
    reg("printer_raw", "Send raw hex bytes to the printer",
        do_printer_raw, &raw_args);

    /* ---- printer_swap <on|off> ---- */
    swap_args.state = arg_str1(NULL, NULL, "<on|off>", "Swap TX<->RX pins");
    swap_args.end   = arg_end(1);
    reg("printer_swap", "Swap TX/RX pins (try if printer_probe gets no response)",
        do_printer_swap, &swap_args);

    /* ---- printer_baud <rate> ---- */
    baud_args.rate = arg_int1(NULL, NULL, "<rate>", "ESP32 UART baud only: 9600..921600");
    baud_args.end  = arg_end(1);
    reg("printer_baud", "Change ESP32 UART baud only (recovery)",
        do_printer_baud, &baud_args);

    /* ---- set_baudrate <rate> ---- */
    set_baudrate_args.rate = arg_int1(NULL, NULL, "<rate>", "Printer+ESP32 baud: 9600..921600 (above 115200 experimental)");
    set_baudrate_args.end  = arg_end(1);
    reg("set_baudrate", "Tell K118 to change baud, then switch ESP32 UART",
        do_set_baudrate, &set_baudrate_args);
}
