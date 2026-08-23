// SPDX-License-Identifier: MIT
// ESP-62 Phase 2 — esp_console REPL. `qr status` reads the module firmware
// version + serial number (the on-device probe). Backend: USB Serial/JTAG.
#include "qr_console.h"

#include <stdio.h>
#include <string.h>

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"

#include "qr_module.h"

static const char *kTag = "qr_console";
static QRModule *s_module = nullptr;

static int cmd_qr(int argc, char **argv) {
    if (argc < 2) {
        printf("usage:\n  qr status   # read module firmware/serial (probe)\n");
        return 0;
    }
    if (strcmp(argv[1], "status") == 0) {
        char fw[64] = {0};
        char sn[64] = {0};
        QRCodeM14 &eng = s_module->engine();
        bool got_fw = eng.getInfos(0xC1, fw, sizeof(fw));  // firmware version
        bool got_sn = eng.getInfos(0xC5, sn, sizeof(sn));  // serial number
        if (!got_fw && !got_sn) {
            printf("qr status: NO REPLY -- check 12V power, DIP switch (UART), "
                   "and that the module is stacked.\n");
            return 1;
        }
        printf("qr firmware=%s\n", got_fw ? fw : "(no reply)");
        printf("qr serial   =%s\n", got_sn ? sn : "(no reply)");
        return 0;
    }
    printf("unknown subcommand: %s\n", argv[1]);
    return 1;
}

static void register_commands() {
    esp_console_cmd_t cmd = {};
    cmd.command = "qr";
    cmd.help = "QR scanner: qr status";
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
    ESP_LOGI(kTag, "esp_console ready (try: qr status)");
}
