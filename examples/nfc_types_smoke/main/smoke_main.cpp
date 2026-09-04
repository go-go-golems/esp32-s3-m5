// SPDX-License-Identifier: MIT
//
// gogolem_nfc Phase 1 build/link smoke. Constructs domain types, exercises
// name helpers, and prints the component version over USB Serial/JTAG. No NFC
// hardware is touched; this only proves the component integrates under ESP-IDF.

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstdio>

#include "gogolem/nfc/result.hpp"
#include "gogolem/nfc/types.hpp"
#include "gogolem/nfc/version.hpp"

using namespace gogolem::nfc;

extern "C" void app_main(void) {
    printf("gogolem_nfc version=%s\n", version());

    TagInfo tag{};
    tag.uid = {0x04, 0x91, 0xD4, 0x4C, 0x9E, 0x61, 0x80, 0, 0, 0};
    tag.uid_length = 7;
    tag.atqa = 0x0044;
    tag.sak = 0x00;
    tag.family = TagFamily::Ntag21x;
    tag.user_bytes = 504;
    tag.supports_ndef = true;
    tag.nfc_forum_tag_type = 2;

    printf("tag family=%s atqa=%04X sak=%02X ndef=%u\n",
           tag_family_name(tag.family), tag.atqa, tag.sak,
           tag.supports_ndef ? 1u : 0u);

    auto ok = Result<void>::success();
    Error transport_err{};
    transport_err.layer = ErrorLayer::Transport;
    transport_err.esp_code = ESP_CODE_ERR_NOT_FOUND;
    transport_err.operation = Operation::Scan;
    transport_err.set_detail("no-tag");
    auto err = Result<void>::failure(transport_err);
    printf("ok=%u err-layer=%s err-detail=%s\n", ok.ok() ? 1u : 0u,
           error_layer_name(err.error().layer), err.error().detail.data());

    printf("mode=%s lifecycle=%s op=%s\n", mode_name(Mode::Reader),
           lifecycle_state_name(LifecycleState::ReadyReader),
           operation_name(Operation::Activate));

    // Loop-print the full public surface so a single USB Serial/JTAG capture
    // window proves the entire component API runs on target, not just one
    // accessor. The component itself is not involved in timing here.
    for (;;) {
        printf("smoke version=%s tag=%s ndef=%u ok=%u err=%s/%s\n",
               version(), tag_family_name(tag.family),
               tag.supports_ndef ? 1u : 0u, ok.ok() ? 1u : 0u,
               error_layer_name(err.error().layer), err.error().detail.data());
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
