#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void keylog_init(void);
void keylog_set_enabled(bool enabled);
bool keylog_is_enabled(void);

// Enqueue raw HID input report bytes for printing/decoding.
void keylog_submit_report(uint8_t report_id, const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

