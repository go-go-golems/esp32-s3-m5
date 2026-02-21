#include "matrix_console.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_console.h"
#include "esp_err.h"

#include "matrix_engine.h"

static void usage(void)
{
    printf("matrix commands:\n");
    printf("  matrix status\n");
    printf("  matrix examples\n");
    printf("  matrix text <TEXT>\n");
    printf("  matrix scroll on <TEXT> [fps] [pause_ms] [repeat_count]\n");
    printf("  matrix scroll wave <TEXT> [fps] [pause_ms] [repeat_count]\n");
    printf("  matrix scroll off\n");
    printf("  matrix anim drop <TEXT> [fps] [pause_ms] [repeat_count]\n");
    printf("  matrix anim off\n");
    printf("  matrix test on|off\n");
    printf("  matrix intensity <0..15>\n");
    printf("  matrix spi [hz]\n");
    printf("  matrix chain [n]\n");
    printf("  matrix reverse on|off\n");
    printf("  matrix flipv on|off\n");
}

static void examples(void)
{
    printf("matrix examples:\n");
    printf("  matrix text HELLO\n");
    printf("  matrix scroll wave \"HELLO WIFI\" 20 250\n");
    printf("  matrix scroll wave \"HELLO WIFI\" 20 250 2\n");
    printf("  matrix anim drop BOUNCE 18 400 3\n");
    printf("  matrix intensity 8\n");
    printf("  matrix test on\n");
    printf("  matrix test off\n");
}

static int cmd_matrix(int argc, char **argv)
{
    if (argc < 2) {
        usage();
        return 0;
    }

    if (strcmp(argv[1], "status") == 0) {
        matrix_status_t st = {0};
        if (matrix_engine_get_status(&st) != ESP_OK) {
            printf("status failed\n");
            return 1;
        }
        const char *mode = "idle";
        if (st.mode == MATRIX_MODE_TEXT) mode = "text";
        if (st.mode == MATRIX_MODE_SCROLL) mode = "scroll";
        if (st.mode == MATRIX_MODE_DROP) mode = "drop";
        printf("ok: ready=%s mode=%s chain=%d width=%d spi_hz=%d intensity=%d test=%s fps=%u pause_ms=%u repeat=%u reverse=%s flipv=%s text=%s\n",
               st.ready ? "yes" : "no",
               mode,
               st.chain_len,
               st.width,
               st.spi_hz,
               st.intensity,
               st.test_mode ? "on" : "off",
               (unsigned)st.fps,
               (unsigned)st.pause_ms,
               (unsigned)st.repeat_count,
               st.reverse_modules ? "on" : "off",
               st.flip_vertical ? "on" : "off",
               st.text[0] ? st.text : "-");
        return 0;
    }

    if (strcmp(argv[1], "examples") == 0) {
        examples();
        return 0;
    }

    if (strcmp(argv[1], "text") == 0) {
        if (argc < 3) return 1;
        return matrix_engine_set_text(argv[2]) == ESP_OK ? 0 : 1;
    }

    if (strcmp(argv[1], "scroll") == 0) {
        if (argc < 3) return 1;
        if (strcmp(argv[2], "off") == 0) return matrix_engine_stop() == ESP_OK ? 0 : 1;
        bool wave = (strcmp(argv[2], "wave") == 0);
        if (!wave && strcmp(argv[2], "on") != 0) return 1;
        if (argc < 4) return 1;
        uint32_t fps = 15;
        uint32_t pause_ms = 250;
        uint32_t repeat_count = 0;
        if (argc >= 5) fps = (uint32_t)strtoul(argv[4], NULL, 0);
        if (argc >= 6) pause_ms = (uint32_t)strtoul(argv[5], NULL, 0);
        if (argc >= 7) repeat_count = (uint32_t)strtoul(argv[6], NULL, 0);
        return matrix_engine_start_scroll(argv[3], fps, pause_ms, repeat_count, wave) == ESP_OK ? 0 : 1;
    }

    if (strcmp(argv[1], "anim") == 0) {
        if (argc < 3) return 1;
        if (strcmp(argv[2], "off") == 0) return matrix_engine_stop() == ESP_OK ? 0 : 1;
        if (strcmp(argv[2], "drop") != 0 || argc < 4) return 1;
        uint32_t fps = 15;
        uint32_t pause_ms = 250;
        uint32_t repeat_count = 0;
        if (argc >= 5) fps = (uint32_t)strtoul(argv[4], NULL, 0);
        if (argc >= 6) pause_ms = (uint32_t)strtoul(argv[5], NULL, 0);
        if (argc >= 7) repeat_count = (uint32_t)strtoul(argv[6], NULL, 0);
        return matrix_engine_start_drop(argv[3], fps, pause_ms, repeat_count) == ESP_OK ? 0 : 1;
    }

    if (strcmp(argv[1], "intensity") == 0 && argc >= 3) {
        return matrix_engine_set_intensity((int)strtol(argv[2], NULL, 0)) == ESP_OK ? 0 : 1;
    }

    if (strcmp(argv[1], "test") == 0 && argc >= 3) {
        if (strcmp(argv[2], "on") == 0) return matrix_engine_set_test(true) == ESP_OK ? 0 : 1;
        if (strcmp(argv[2], "off") == 0) return matrix_engine_set_test(false) == ESP_OK ? 0 : 1;
        return 1;
    }

    if (strcmp(argv[1], "spi") == 0) {
        if (argc < 3) {
            matrix_status_t st = {0};
            (void)matrix_engine_get_status(&st);
            printf("ok: spi_hz=%d\n", st.spi_hz);
            return 0;
        }
        return matrix_engine_set_spi_hz((int)strtol(argv[2], NULL, 0)) == ESP_OK ? 0 : 1;
    }

    if (strcmp(argv[1], "chain") == 0) {
        if (argc < 3) {
            matrix_status_t st = {0};
            (void)matrix_engine_get_status(&st);
            printf("ok: chain=%d\n", st.chain_len);
            return 0;
        }
        return matrix_engine_set_chain_len((int)strtol(argv[2], NULL, 0)) == ESP_OK ? 0 : 1;
    }

    if (strcmp(argv[1], "reverse") == 0 && argc >= 3) {
        matrix_status_t st = {0};
        (void)matrix_engine_get_status(&st);
        bool on = strcmp(argv[2], "on") == 0;
        return matrix_engine_set_orientation(on, st.flip_vertical) == ESP_OK ? 0 : 1;
    }

    if (strcmp(argv[1], "flipv") == 0 && argc >= 3) {
        matrix_status_t st = {0};
        (void)matrix_engine_get_status(&st);
        bool on = strcmp(argv[2], "on") == 0;
        return matrix_engine_set_orientation(st.reverse_modules, on) == ESP_OK ? 0 : 1;
    }

    usage();
    return 1;
}

void matrix_console_register_commands(void)
{
    esp_console_cmd_t cmd = {
        .command = "matrix",
        .help = "LED matrix control (run `matrix examples`)",
        .func = cmd_matrix,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
