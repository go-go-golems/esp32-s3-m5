// SPDX-License-Identifier: MIT
//
// Demo NDEF messages — example data, not component policy.

#include "demo_messages.hpp"
#include "gogolem/nfc/ndef.hpp"

namespace gogolem::nfc::example {

NdefMessage make_demo_ndef() {
    NdefMessage msg;
    msg.records.push_back(make_uri_record("https://m5stack.com/esp60"));
    msg.records.push_back(make_text_record("Native ESP-IDF M5StackChan NFC", "en"));
    return msg;
}

}  // namespace gogolem::nfc::example
