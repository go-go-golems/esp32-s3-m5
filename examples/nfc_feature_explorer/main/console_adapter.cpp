// SPDX-License-Identifier: MIT
//
// Console adapter — registers esp_console commands that call the Engine.
// All printing, confirmation strings, and human-readable output live here;
// the Engine returns structured results.

#include "console_adapter.hpp"

#include <cstdio>
#include <cstring>
#include <string>

#include "demo_messages.hpp"
#include "demo_profiles.hpp"
#include "nvs_mode_store.hpp"

#include "esp_console.h"
#include "esp_system.h"

#include "gogolem/nfc/engine.hpp"
#include "gogolem/nfc/mutation.hpp"
#include "gogolem/nfc/ndef.hpp"

namespace gogolem::nfc::example {

static Engine* g_engine = nullptr;
static i2c_master_bus_handle_t g_bus = nullptr;

static void print_tag(const char* prefix, const TagInfo& tag) {
    printf("%s uid=", prefix);
    for (uint8_t i = 0; i < tag.uid_length; ++i) printf("%02X", tag.uid[i]);
    printf(" family=%s atqa=%04X sak=%02X blocks=%u unit=%u user=%lu total=%lu ndef=%u\n",
           tag_family_name(tag.family), tag.atqa, tag.sak,
           tag.block_or_page_count, tag.unit_size,
           static_cast<unsigned long>(tag.user_bytes),
           static_cast<unsigned long>(tag.total_bytes),
           tag.supports_ndef ? 1u : 0u);
}

static void print_hex(const char* prefix, const uint8_t* data, size_t len) {
    printf("%s len=%u hex=", prefix, static_cast<unsigned>(len));
    for (size_t i = 0; i < len; ++i) {
        printf("%02X", data[i]);
        if (i + 1 != len) putchar(':');
    }
    putchar('\n');
}

// ---- Commands -------------------------------------------------------------

static int cmd_capabilities(int argc, char** argv) {
    printf("NFC_CAPABILITY name=quick-scan command=\"nfc-scan\" mutation=none\n");
    printf("NFC_CAPABILITY name=identification command=\"nfc-info\" mutation=none\n");
    printf("NFC_CAPABILITY name=complete-dump command=\"nfc-dump\" mutation=none\n");
    printf("NFC_CAPABILITY name=direct-read command=\"nfc-raw-read <address>\" mutation=none\n");
    printf("NFC_CAPABILITY name=direct-write-test command=\"nfc-write-test <address> --confirm RESTORE-AFTER-TEST\" mutation=reversible\n");
    printf("NFC_CAPABILITY name=ndef-read command=\"nfc-ndef-read\" mutation=none\n");
    printf("NFC_CAPABILITY name=ndef-write command=\"nfc-ndef-write-demo --confirm REPLACE-NDEF\" mutation=replace\n");
    printf("NFC_CAPABILITY name=tag-emulation command=\"nfc-mode <reader|emulation-ultralight|emulation-ntag213> --confirm REBOOT\" mutation=firmware-mode\n");
    return 0;
}

static int cmd_scan(int argc, char** argv) {
    auto r = g_engine->scan(1000);
    printf("NFC_RESULT op=scan ok=%u detected=%u\n",
           r.ok() ? 1u : 0u,
           r.ok() ? static_cast<unsigned>(r.value().tags.size()) : 0u);
    if (r.ok()) {
        for (const auto& tag : r.value().tags) print_tag("NFC_SCAN_PICC", tag);
    }
    return r.ok() ? 0 : 1;
}

static int cmd_info(int argc, char** argv) {
    auto r = g_engine->activate_one();
    if (r.ok()) {
        print_tag("NFC_PICC", r.value().tag);
        printf("NFC_ACTIVATE source=%s\n", r.value().source == ActivationSource::WUPA ? "WUPA" : "REQA");
        g_engine->deactivate();
    }
    printf("NFC_RESULT op=info ok=%u\n", r.ok() ? 1u : 0u);
    return r.ok() ? 0 : 1;
}

static int cmd_dump(int argc, char** argv) {
    auto r = g_engine->dump();
    printf("NFC_RESULT op=dump ok=%u\n", r.ok() ? 1u : 0u);
    return r.ok() ? 0 : 1;
}

static int cmd_raw_read(int argc, char** argv) {
    if (argc < 2) { printf("usage: nfc-raw-read <address 0..255>\n"); return 1; }
    uint8_t addr = static_cast<uint8_t>(atoi(argv[1]));
    auto r = g_engine->raw_read(addr);
    if (r.ok()) print_hex("NFC_RAW_READ", r.value().data(), r.value().size());
    printf("NFC_RESULT op=raw-read ok=%u address=%u\n", r.ok() ? 1u : 0u, addr);
    return r.ok() ? 0 : 1;
}

static int cmd_write_test(int argc, char** argv) {
    if (argc < 2) { printf("usage: nfc-write-test <address> --confirm RESTORE-AFTER-TEST\n"); return 1; }
    uint8_t addr = static_cast<uint8_t>(atoi(argv[1]));
    bool confirmed = false;
    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "RESTORE-AFTER-TEST") == 0) confirmed = true;
    }
    if (!confirmed) {
        printf("usage: nfc-write-test <address> --confirm RESTORE-AFTER-TEST\n");
        printf("warning: use only a sacrificial tag; restoration can fail if communication is interrupted\n");
        return 1;
    }
    auto act = g_engine->activate_one();
    if (!act.ok()) { printf("NFC_RESULT op=write-test ok=0\n"); return 1; }
    TagInfo tag = act.value().tag;
    g_engine->deactivate();
    MutationPermit permit{};
    permit.allowed = MutationKind::ReversibleWrite;
    memcpy(permit.expected_uid.data(), tag.uid.data(), tag.uid_length);
    permit.expected_uid_length = tag.uid_length;
    auto r = g_engine->reversible_write(addr, permit);
    printf("NFC_RESULT op=write-test ok=%u address=%u write=%u verify=%u restore=%u\n",
           r.ok() ? 1u : 0u, addr,
           r.ok() ? (r.value().write_succeeded ? 1u : 0u) : 0u,
           r.ok() ? (r.value().verification_succeeded ? 1u : 0u) : 0u,
           r.ok() ? (r.value().restoration_succeeded ? 1u : 0u) : 0u);
    if (!r.ok()) printf("NFC_WRITE_TEST err=%s detail=%s\n",
                        error_layer_name(r.error().layer), r.error().detail.data());
    return r.ok() ? 0 : 1;
}

static int cmd_ndef_read(int argc, char** argv) {
    auto r = g_engine->read_ndef();
    if (r.ok()) {
        printf("NFC_NDEF_RECORDS count=%u\n", static_cast<unsigned>(r.value().records.size()));
        for (size_t i = 0; i < r.value().records.size(); ++i) {
            std::string lang;
            auto uri = uri_record_to_string(r.value().records[i]);
            auto text = text_record_to_string(r.value().records[i], lang);
            if (!uri.empty()) printf("NFC_NDEF_RECORD index=%u uri=%s\n", static_cast<unsigned>(i), uri.c_str());
            if (!text.empty()) printf("NFC_NDEF_RECORD index=%u text=%s lang=%s\n", static_cast<unsigned>(i), text.c_str(), lang.c_str());
        }
    }
    printf("NFC_RESULT op=ndef-read ok=%u\n", r.ok() ? 1u : 0u);
    return r.ok() ? 0 : 1;
}

static int cmd_ndef_write_demo(int argc, char** argv) {
    bool confirmed = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "REPLACE-NDEF") == 0) confirmed = true;
    }
    if (!confirmed) {
        printf("usage: nfc-ndef-write-demo --confirm REPLACE-NDEF\n");
        printf("warning: replaces the current NDEF message\n");
        return 1;
    }
    auto act = g_engine->activate_one();
    if (!act.ok()) { printf("NFC_RESULT op=ndef-write ok=0\n"); return 1; }
    TagInfo tag = act.value().tag;
    g_engine->deactivate();
    MutationPermit permit{};
    permit.allowed = MutationKind::ReplaceNdef;
    memcpy(permit.expected_uid.data(), tag.uid.data(), tag.uid_length);
    permit.expected_uid_length = tag.uid_length;
    auto r = g_engine->write_ndef(make_demo_ndef(), permit);
    printf("NFC_RESULT op=ndef-write-demo ok=%u\n", r.ok() ? 1u : 0u);
    if (!r.ok()) printf("NFC_NDEF_WRITE err=%s detail=%s\n",
                        error_layer_name(r.error().layer), r.error().detail.data());
    return r.ok() ? 0 : 1;
}

static int cmd_mode(int argc, char** argv) {
    if (argc < 2) {
        printf("NFC_MODE current=%s\n", example::mode_name(g_engine->mode()));
        return 0;
    }
    bool confirmed = false;
    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "REBOOT") == 0) confirmed = true;
    }
    if (!confirmed) {
        printf("usage: nfc-mode <reader|emulation-ultralight|emulation-ntag213> --confirm REBOOT\n");
        return 1;
    }
    Mode new_mode = Mode::Reader;
    if (strcmp(argv[1], "emulation-ultralight") == 0) new_mode = Mode::EmulationUltralight;
    else if (strcmp(argv[1], "emulation-ntag213") == 0) new_mode = Mode::EmulationNtag213;
    else if (strcmp(argv[1], "reader") == 0) new_mode = Mode::Reader;
    else { printf("NFC_MODE ok=0 reason=unknown-mode\n"); return 1; }
    store_boot_mode(new_mode);
    printf("NFC_MODE_STORE ok=1 next=%s\n", example::mode_name(new_mode));
    esp_restart();
    return 0;
}

static int cmd_emulation_status(int argc, char** argv) {
    auto s = g_engine->emulation_state();
    const char* names[] = {"none", "off", "idle", "ready", "active", "halt"};
    printf("NFC_EMULATION_STATUS ok=1 mode=%s state=%s\n",
           example::mode_name(g_engine->mode()),
           s <= EmulationState::Halt ? names[static_cast<int>(s)] : "unknown");
    return 0;
}

// ---- Registration ---------------------------------------------------------

static void register_cmd(const char* name, const char* help, esp_console_cmd_func_t func) {
    esp_console_cmd_t cmd{};
    cmd.command = name;
    cmd.help = help;
    cmd.func = func;
    esp_console_cmd_register(&cmd);
}

void register_console_commands(Engine& engine, i2c_master_bus_handle_t bus) {
    g_engine = &engine;
    g_bus = bus;
    register_cmd("nfc-capabilities", "List capabilities", cmd_capabilities);
    register_cmd("nfc-scan", "Quick scan", cmd_scan);
    register_cmd("nfc-info", "Identify tag", cmd_info);
    register_cmd("nfc-dump", "Dump card", cmd_dump);
    register_cmd("nfc-raw-read", "Raw read", cmd_raw_read);
    register_cmd("nfc-write-test", "Reversible write test", cmd_write_test);
    register_cmd("nfc-ndef-read", "Read NDEF", cmd_ndef_read);
    register_cmd("nfc-ndef-write-demo", "Write demo NDEF", cmd_ndef_write_demo);
    register_cmd("nfc-mode", "Set/switch mode", cmd_mode);
    register_cmd("nfc-emulation-status", "Emulation status", cmd_emulation_status);
}

}  // namespace gogolem::nfc::example
