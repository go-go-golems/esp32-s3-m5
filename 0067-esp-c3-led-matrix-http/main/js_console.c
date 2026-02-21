#include "js_console.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_console.h"
#include "esp_err.h"

#include "mqjs/js_runtime_bridge.h"

static void usage(void)
{
    printf("js commands:\n");
    printf("  js status\n");
    printf("  js eval <CODE>\n");
    printf("  js reset [hard]\n");
    printf("  js stop\n");
    printf("  js mem\n");
    printf("  js examples\n");
}

static void examples(void)
{
    printf("js examples:\n");
    printf("  js eval matrix.setText('HELLO')\n");
    printf("  js eval matrix.startScroll('HELLO WIFI', {fps:20,pauseMs:250,repeat:2,wave:true})\n");
    printf("  js eval matrix.clear(); for (var x=0; x<matrix.width(); x++) { matrix.setPixel(x, 3, 1); } matrix.present();\n");
    printf("  js eval var h=every(50,function(){ matrix.setPixel((Date.now()/50)%%matrix.width(),2,1); matrix.present(); });\n");
    printf("  js reset          # soft reset (default)\n");
    printf("  js reset hard     # hard reset (recreate VM)\n");
    printf("  js stop\n");
}

static char *join_args(int argc, char **argv, int from)
{
    size_t need = 1;
    for (int i = from; i < argc; i++) need += strlen(argv[i]) + 1;
    char *out = (char *)malloc(need);
    if (!out) return NULL;
    out[0] = '\0';
    for (int i = from; i < argc; i++) {
        if (i > from) strlcat(out, " ", need);
        strlcat(out, argv[i], need);
    }
    return out;
}

static int cmd_js(int argc, char **argv)
{
    if (argc < 2) {
        usage();
        return 0;
    }

    if (strcmp(argv[1], "status") == 0) {
        js_service_status_t st = {0};
        if (js_service_get_status(&st) != ESP_OK) {
            printf("js status failed\n");
            return 1;
        }
        printf("ok: started=%s busy=%s stop_requested=%s eval_count=%u last_eval_ms=%u timed_out=%s last_error=%s\n",
               st.started ? "yes" : "no",
               st.busy ? "yes" : "no",
               st.stop_requested ? "yes" : "no",
               (unsigned)st.eval_count,
               (unsigned)st.last_eval_ms,
               st.last_timed_out ? "yes" : "no",
               st.last_error[0] ? st.last_error : "-");
        return 0;
    }

    if (strcmp(argv[1], "examples") == 0) {
        examples();
        return 0;
    }

    if (strcmp(argv[1], "reset") == 0) {
        if (argc >= 3 && strcmp(argv[2], "hard") == 0) {
            return js_service_hard_reset() == ESP_OK ? 0 : 1;
        }
        return js_service_reset() == ESP_OK ? 0 : 1;
    }

    if (strcmp(argv[1], "stop") == 0) {
        return js_service_request_stop() == ESP_OK ? 0 : 1;
    }

    if (strcmp(argv[1], "mem") == 0) {
        char *txt = NULL;
        if (js_service_dump_memory_text(&txt) != ESP_OK) {
            printf("js mem failed\n");
            return 1;
        }
        if (txt) {
            printf("%s", txt);
            js_service_free(txt);
        }
        return 0;
    }

    if (strcmp(argv[1], "eval") == 0) {
        if (argc < 3) return 1;
        char *code = join_args(argc, argv, 2);
        if (!code) return 1;

        char *json = NULL;
        int rc = 0;
        esp_err_t err = js_service_eval_json(code, strlen(code), 0, "<console>", &json);
        if (err != ESP_OK) rc = 1;
        if (json) {
            printf("%s\n", json);
            js_service_free(json);
        }
        free(code);
        return rc;
    }

    usage();
    return 1;
}

void js_console_register_commands(void)
{
    const esp_console_cmd_t cmd = {
        .command = "js",
        .help = "MicroQuickJS runtime (run `js examples`)",
        .func = cmd_js,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
