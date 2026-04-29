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
 */

#include "printer_cmd.h"

#include <stdlib.h>
#include <string.h>
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "esp_log.h"

#include "printer_drv.h"

static const char *TAG = "printer_cmd";

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
}
