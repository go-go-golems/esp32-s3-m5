// ESP-56: framebuffer capture for the screenshot-driven design pass.
// Runs on the OWNER (console op Shot) so it never races a present; the
// QOI bytes stream over the same USB serial the console uses, framed by
// QOI_BEGIN <len> / QOI_END from the component.
#include "app_shot.h"

#include <M5Unified.hpp>

#include "screenshot_qoi.h"

namespace pulp {

bool ShotToConsole(uint32_t *out_len) {
    size_t len = 0;
    const bool ok = screenshot_qoi_to_usb_serial_jtag_ex(M5.Display, &len);
    if (out_len != nullptr) {
        *out_len = static_cast<uint32_t>(len);
    }
    return ok;
}

}  // namespace pulp
