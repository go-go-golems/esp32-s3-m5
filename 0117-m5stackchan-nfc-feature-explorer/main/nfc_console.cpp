#include "nfc_console.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "esp_console.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nfc_explorer.hpp"

namespace {
NfcExplorer *g_explorer{};

bool parse_u8(const char *text, uint8_t &value)
{
    if (!text || !*text) {
        return false;
    }
    errno = 0;
    char *end{};
    const unsigned long parsed = std::strtoul(text, &end, 0);
    if (errno || !end || *end || parsed > 0xFF) {
        return false;
    }
    value = static_cast<uint8_t>(parsed);
    return true;
}

bool parse_u32(const char *text, uint32_t &value)
{
    if (!text || !*text) {
        return false;
    }
    errno = 0;
    char *end{};
    const unsigned long parsed = std::strtoul(text, &end, 0);
    if (errno || !end || *end) {
        return false;
    }
    value = static_cast<uint32_t>(parsed);
    return true;
}

bool confirmation(int argc, char **argv, int flag_index, const char *token)
{
    return argc > flag_index + 1 && std::strcmp(argv[flag_index], "--confirm") == 0 &&
           std::strcmp(argv[flag_index + 1], token) == 0;
}

int cmd_capabilities(int, char **)
{
    printf("NFC_CAPABILITY name=quick-scan command=\"nfc-scan [timeout_ms]\" mutation=none\n");
    printf("NFC_CAPABILITY name=identification command=nfc-info mutation=none\n");
    printf("NFC_CAPABILITY name=complete-dump command=nfc-dump mutation=none\n");
    printf("NFC_CAPABILITY name=direct-read command=\"nfc-raw-read <address>\" mutation=none\n");
    printf("NFC_CAPABILITY name=direct-write-test command=\"nfc-write-test <address> --confirm RESTORE-AFTER-TEST\" mutation=reversible\n");
    printf("NFC_CAPABILITY name=ndef-read command=nfc-ndef-read mutation=none\n");
    printf("NFC_CAPABILITY name=ndef-write command=\"nfc-ndef-write-demo --confirm REPLACE-NDEF\" mutation=replace\n");
    printf("NFC_CAPABILITY name=value-inspect command=nfc-value-inspect mutation=none card=MIFARE-Classic\n");
    printf("NFC_CAPABILITY name=wallet command=\"nfc-wallet-demo <block> <non-rechargeable|rechargeable> --confirm MUTATE-CLASSIC\" mutation=structural card=MIFARE-Classic\n");
    printf("NFC_CAPABILITY name=tag-emulation command=\"nfc-mode <emulation-ultralight|emulation-ntag213> --confirm REBOOT\" mutation=firmware-mode\n");
    return 0;
}

int cmd_mode(int argc, char **argv)
{
    if (argc == 1) {
        printf("NFC_MODE current=%s ready=%u\n", nfc_boot_mode_name(g_explorer->mode()), g_explorer->ready());
        return 0;
    }
    if (argc != 4 || !confirmation(argc, argv, 2, "REBOOT")) {
        printf("usage: nfc-mode <reader|emulation-ultralight|emulation-ntag213> --confirm REBOOT\n");
        return 1;
    }

    NfcBootMode mode{};
    if (std::strcmp(argv[1], "reader") == 0) {
        mode = NfcBootMode::Reader;
    } else if (std::strcmp(argv[1], "emulation-ultralight") == 0) {
        mode = NfcBootMode::EmulationUltralight;
    } else if (std::strcmp(argv[1], "emulation-ntag213") == 0) {
        mode = NfcBootMode::EmulationNtag213;
    } else {
        printf("NFC_MODE ok=0 reason=unknown-mode value=%s\n", argv[1]);
        return 1;
    }

    const esp_err_t err = nfc_store_boot_mode(mode);
    printf("NFC_MODE_STORE ok=%u next=%s error=%s\n", err == ESP_OK, nfc_boot_mode_name(mode), esp_err_to_name(err));
    if (err != ESP_OK) {
        return 1;
    }
    fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_restart();
    return 0;
}

int cmd_scan(int argc, char **argv)
{
    uint32_t timeout = 1000;
    if (argc > 2 || (argc == 2 && !parse_u32(argv[1], timeout))) {
        printf("usage: nfc-scan [timeout_ms]\n");
        return 1;
    }
    return g_explorer->scan(timeout) ? 0 : 1;
}

int cmd_info(int argc, char **)
{
    if (argc != 1) {
        printf("usage: nfc-info\n");
        return 1;
    }
    return g_explorer->info() ? 0 : 1;
}

int cmd_dump(int argc, char **)
{
    if (argc != 1) {
        printf("usage: nfc-dump\n");
        return 1;
    }
    return g_explorer->dump() ? 0 : 1;
}

int cmd_raw_read(int argc, char **argv)
{
    uint8_t address{};
    if (argc != 2 || !parse_u8(argv[1], address)) {
        printf("usage: nfc-raw-read <address 0..255>\n");
        return 1;
    }
    return g_explorer->raw_read(address) ? 0 : 1;
}

int cmd_write_test(int argc, char **argv)
{
    uint8_t address{};
    if (argc != 4 || !parse_u8(argv[1], address) || !confirmation(argc, argv, 2, "RESTORE-AFTER-TEST")) {
        printf("usage: nfc-write-test <address> --confirm RESTORE-AFTER-TEST\n");
        printf("warning: use only a sacrificial tag; restoration can fail if communication is interrupted\n");
        return 1;
    }
    return g_explorer->reversible_write_test(address) ? 0 : 1;
}

int cmd_ndef_read(int argc, char **)
{
    if (argc != 1) {
        printf("usage: nfc-ndef-read\n");
        return 1;
    }
    return g_explorer->ndef_read() ? 0 : 1;
}

int cmd_ndef_write_demo(int argc, char **argv)
{
    if (argc != 3 || !confirmation(argc, argv, 1, "REPLACE-NDEF")) {
        printf("usage: nfc-ndef-write-demo --confirm REPLACE-NDEF\n");
        printf("warning: replaces the current NDEF message; non-NDEF tags are refused\n");
        return 1;
    }
    return g_explorer->ndef_write_demo() ? 0 : 1;
}

int cmd_value_inspect(int argc, char **)
{
    if (argc != 1) {
        printf("usage: nfc-value-inspect\n");
        return 1;
    }
    return g_explorer->value_inspect() ? 0 : 1;
}

int cmd_wallet_demo(int argc, char **argv)
{
    uint8_t block{};
    if (argc != 5 || !parse_u8(argv[1], block) || !confirmation(argc, argv, 3, "MUTATE-CLASSIC")) {
        printf("usage: nfc-wallet-demo <block> <non-rechargeable|rechargeable> --confirm MUTATE-CLASSIC\n");
        printf("warning: changes Classic access bits and data; use only a sacrificial default-key card\n");
        return 1;
    }
    bool rechargeable{};
    if (std::strcmp(argv[2], "rechargeable") == 0) {
        rechargeable = true;
    } else if (std::strcmp(argv[2], "non-rechargeable") != 0) {
        printf("NFC_WALLET ok=0 reason=unknown-mode value=%s\n", argv[2]);
        return 1;
    }
    return g_explorer->wallet_demo(block, rechargeable) ? 0 : 1;
}

int cmd_emulation_status(int argc, char **)
{
    if (argc != 1) {
        printf("usage: nfc-emulation-status\n");
        return 1;
    }
    g_explorer->print_emulation_status();
    return 0;
}

void register_command(const char *name, const char *help, esp_console_cmd_func_t function, const char *hint = nullptr)
{
    const esp_console_cmd_t command{
        .command = name,
        .help = help,
        .hint = hint,
        .func = function,
        .argtable = nullptr,
        .func_w_context = nullptr,
        .context = nullptr,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&command));
}
}  // namespace

void nfc_console_register(NfcExplorer &explorer)
{
    g_explorer = &explorer;
    register_command("nfc-capabilities", "List official-sketch-equivalent NFC features", cmd_capabilities);
    register_command("nfc-mode", "Show or persist reader/emulation mode and reboot", cmd_mode,
                     "[reader|emulation-ultralight|emulation-ntag213 --confirm REBOOT]");
    register_command("nfc-scan", "Enumerate and identify NFC-A PICCs", cmd_scan, "[timeout_ms]");
    register_command("nfc-info", "Identify and print one PICC capability record", cmd_info);
    register_command("nfc-dump", "Read-only complete card dump using Classic default Key A", cmd_dump);
    register_command("nfc-raw-read", "Read one 16-byte block or four Type-2 pages", cmd_raw_read, "<address>");
    register_command("nfc-write-test", "Write, verify, and restore one user block/page (destructive risk)", cmd_write_test,
                     "<address> --confirm RESTORE-AFTER-TEST");
    register_command("nfc-ndef-read", "Validate and parse the selected tag's NDEF message", cmd_ndef_read);
    register_command("nfc-ndef-write-demo", "Replace NDEF with URI + text records (destructive)", cmd_ndef_write_demo,
                     "--confirm REPLACE-NDEF");
    register_command("nfc-value-inspect", "Read-only scan for MIFARE Classic value blocks", cmd_value_inspect);
    register_command("nfc-wallet-demo", "Run and restore a Classic value-block demonstration (high risk)", cmd_wallet_demo,
                     "<block> <non-rechargeable|rechargeable> --confirm MUTATE-CLASSIC");
    register_command("nfc-emulation-status", "Show Ultralight/NTAG213 target-emulation state", cmd_emulation_status);
}
