// SPDX-License-Identifier: MIT
// ESP-62 Phase 4 — esp_console REPL with the full `qr` command set over USB
// Serial/JTAG. Subcommands: status, info, start, stop, mode, light,
// brightness, beep, reset.
#include "qr_console.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"

#include "qr_module.h"

static const char *kTag = "qr_console";
static QRModule *s_module = nullptr;

static void print_usage(void) {
    printf("usage:\n");
    printf("  qr status            # read firmware(0xC1)+serial(0xC5)\n");
    printf("  qr info              # alias for status\n");
    printf("  qr raw <hex...>       # send raw hex bytes, dump any reply (debug)\n");
    printf("  qr start             # start decoding (software trigger)\n");
    printf("  qr stop              # stop decoding\n");
    printf("  qr mode <key|cont|auto|pulse|sense>\n");
    printf("  qr light <off|decode|on>\n");
    printf("  qr brightness <0-100>\n");
    printf("  qr beep <on|off>\n");
    printf("  qr uart              # request RS232/UART output mode\n");
    printf("  qr suffix            # enable CRLF scan suffix\n");
    printf("  qr power-cycle       # cycle scanner power via expander ch0\n");
    printf("  qr trig <low|high|pulse> # drive hardware TRIG via expander ch4\n");
    printf("  qr lines             # read power/TRIG outputs and G14 RX level\n");
    printf("  qr baud-probe        # probe documented scanner UART baud rates\n");
    printf("  qr route-probe       # probe safe QRCode DIP UART routes\n");
    printf("  qr reset CONFIRM-21424000 # factory reset; may disable UART\n");
}

static int print_result(const char *operation, QRCodeM14::CmdResult result,
                        bool no_ack = false) {
    printf("%s: %s%s\n", operation, QRCodeM14::resultName(result),
           no_ack && result == QRCodeM14::OK ? " (write accepted; no ACK defined)" : "");
    return result == QRCodeM14::OK ? 0 : 1;
}

static int parse_mode(const char *s, QRCodeM14::TriggerMode *out) {
    if (!strcmp(s, "key")) { *out = QRCodeM14::KEY; return 0; }
    if (!strcmp(s, "cont")) { *out = QRCodeM14::CONTINUOUS; return 0; }
    if (!strcmp(s, "auto")) { *out = QRCodeM14::AUTO; return 0; }
    if (!strcmp(s, "pulse")) { *out = QRCodeM14::PULSE; return 0; }
    if (!strcmp(s, "sense")) { *out = QRCodeM14::MOTION; return 0; }
    return -1;
}

static int do_status() {
    printf("[qr status] probing...\n");
    fflush(stdout);
    char fw[64] = {0};
    char sn[64] = {0};
    bool got_fw = s_module->getInfo(0xC1, fw, sizeof(fw));
    bool got_sn = s_module->getInfo(0xC5, sn, sizeof(sn));
    if (!got_fw && !got_sn) {
        printf("qr status: NO REPLY -- if hardware TRIG lights the scanner, "
               "scan Serial Communication programming barcode 21424000; "
               "otherwise check 12V, UART switch, routing, and stack.\n");
        return 1;
    }
    printf("qr firmware=%s\n", got_fw ? fw : "(no reply)");
    printf("qr serial   =%s\n", got_sn ? sn : "(no reply)");
    return 0;
}

static int cmd_qr(int argc, char **argv) {
    if (argc < 2) {
        print_usage();
        return 0;
    }
    const char *sub = argv[1];

    if (!strcmp(sub, "status") || !strcmp(sub, "info")) {
        return do_status();
    }
    if (!strcmp(sub, "raw")) {
        // qr raw 43 02 C1  -> send raw hex, dump any reply (debug)
        if (argc < 3) { printf("qr raw needs hex bytes, e.g. 'qr raw 43 02 C1'\n"); return 1; }
        uint8_t cmd[16]; int n = 0;
        for (int i = 2; i < argc && n < (int)sizeof(cmd); i++) {
            cmd[n++] = (uint8_t)strtoul(argv[i], nullptr, 16);
        }
        uint8_t dump[128] = {0};
        size_t got = 0;
        if (!s_module->rawCommand(cmd, n, dump, sizeof(dump), &got)) {
            printf("raw command failed (module unavailable or request queue full)\n");
            return 1;
        }
        printf("rx %u bytes:", (unsigned)got);
        for (size_t i = 0; i < got; i++) printf(" %02x", dump[i]);
        printf("\n");
        return 0;
    }
    if (!strcmp(sub, "start")) {
        return print_result("start", s_module->startScan(), true);
    }
    if (!strcmp(sub, "stop")) {
        return print_result("stop", s_module->stopScan());
    }
    if (!strcmp(sub, "reset")) {
        if (argc != 3 || strcmp(argv[2], "CONFIRM-21424000")) {
            printf("REFUSED: factory reset can persist USB mode and disable "
                   "UART control. Keep recovery barcode 21424000 available, "
                   "then run: qr reset CONFIRM-21424000\n");
            return 1;
        }
        return print_result("factory-reset", s_module->factoryReset(), true);
    }
    if (!strcmp(sub, "uart")) {
        return print_result("uart-mode", s_module->setModeUart(), true);
    }
    if (!strcmp(sub, "suffix")) {
        return print_result("suffix-crlf", s_module->enableSuffixCrLf());
    }
    if (!strcmp(sub, "power-cycle")) {
        return print_result("power-cycle", s_module->powerCycle());
    }
    if (!strcmp(sub, "trig")) {
        if (argc < 3) { printf("qr trig needs <low|high|pulse>\n"); return 1; }
        if (!strcmp(argv[2], "low")) {
            return print_result("trig-low", s_module->setHardwareTrigger(false));
        }
        if (!strcmp(argv[2], "high")) {
            return print_result("trig-high", s_module->setHardwareTrigger(true));
        }
        if (!strcmp(argv[2], "pulse")) {
            return print_result("trig-pulse", s_module->pulseHardwareTrigger());
        }
        printf("bad trigger action: %s\n", argv[2]);
        return 1;
    }
    if (!strcmp(sub, "lines")) {
        char state[64] = {0};
        if (!s_module->getElectricalState(state, sizeof(state))) {
            printf("lines: unavailable\n");
            return 1;
        }
        printf("lines: %s\n", state);
        return 0;
    }
    if (!strcmp(sub, "baud-probe")) {
        char found[64] = {0};
        if (!s_module->probeBauds(found, sizeof(found))) {
            printf("baud-probe: no response; restored host baud=115200\n");
            return 1;
        }
        printf("baud-probe: %s\n", found);
        return 0;
    }
    if (!strcmp(sub, "route-probe")) {
        char found[64] = {0};
        if (!s_module->probeRoutes(found, sizeof(found))) {
            printf("route-probe: no response; restored G13/G14\n");
            return 1;
        }
        printf("route-probe: %s\n", found);
        return 0;
    }
    if (!strcmp(sub, "mode")) {
        if (argc < 3) { printf("qr mode needs <key|cont|auto|pulse|sense>\n"); return 1; }
        QRCodeM14::TriggerMode m;
        if (parse_mode(argv[2], &m) != 0) { printf("bad mode: %s\n", argv[2]); return 1; }
        return print_result("mode", s_module->setTriggerMode(m));
    }
    if (!strcmp(sub, "light")) {
        if (argc < 3) { printf("qr light needs <off|decode|on>\n"); return 1; }
        QRCodeM14::FillLightMode m = QRCodeM14::FILL_ON_DECODE;
        if (!strcmp(argv[2], "off")) m = QRCodeM14::FILL_OFF;
        else if (!strcmp(argv[2], "decode")) m = QRCodeM14::FILL_ON_DECODE;
        else if (!strcmp(argv[2], "on")) m = QRCodeM14::FILL_ON;
        else { printf("bad light: %s\n", argv[2]); return 1; }
        return print_result("light", s_module->setFillLightMode(m));
    }
    if (!strcmp(sub, "brightness")) {
        if (argc < 3) { printf("qr brightness needs <0-100>\n"); return 1; }
        int v = atoi(argv[2]);
        return print_result("brightness", s_module->setBrightness(v));
    }
    if (!strcmp(sub, "beep")) {
        if (argc < 3) { printf("qr beep needs <on|off>\n"); return 1; }
        return print_result("beep", s_module->setBeep(!strcmp(argv[2], "on") ? 1 : 0));
    }
    printf("unknown subcommand: %s\n", sub);
    print_usage();
    return 1;
}

static void register_commands() {
    esp_console_cmd_t cmd = {};
    cmd.command = "qr";
    cmd.help = "QR scanner: status|info|start|stop|mode|light|brightness|beep|reset";
    cmd.func = &cmd_qr;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void QRConsole::start(QRModule &module) {
    s_module = &module;
    esp_console_repl_t *repl = nullptr;
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "cores3-qr> ";
    repl_cfg.task_stack_size = 4096;

#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_dev_usb_serial_jtag_config_t hw = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    esp_err_t err = esp_console_new_repl_usb_serial_jtag(&hw, &repl_cfg, &repl);
#else
    esp_err_t err = ESP_ERR_NOT_SUPPORTED;
#endif
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "esp_console unavailable: %s", esp_err_to_name(err));
        return;
    }
    esp_console_register_help_command();
    register_commands();
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
    ESP_LOGI(kTag, "esp_console ready (try: qr, help)");
}
