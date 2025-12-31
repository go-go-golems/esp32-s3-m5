#include "keylog.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"

static const char *TAG = "keylog";

typedef struct {
    uint8_t report_id;
    uint16_t len;
    uint8_t data[32];
} keylog_msg_t;

static QueueHandle_t s_q = NULL;
static bool s_enabled = false;

static bool hid_kbd_usage_to_ascii(uint8_t usage, bool shift, char *out) {
    if (!out) return false;

    // Very small US mapping (enough for first bring-up).
    if (usage >= 0x04 && usage <= 0x1d) { // a-z
        char c = (char)('a' + (usage - 0x04));
        if (shift) c = (char)('A' + (usage - 0x04));
        *out = c;
        return true;
    }
    if (usage >= 0x1e && usage <= 0x27) { // 1-0
        static const char unshift[] = {'1','2','3','4','5','6','7','8','9','0'};
        static const char shifted[] = {'!','@','#','$','%','^','&','*','(',')'};
        *out = shift ? shifted[usage - 0x1e] : unshift[usage - 0x1e];
        return true;
    }

    switch (usage) {
    case 0x28: *out = '\n'; return true; // Enter
    case 0x2c: *out = ' '; return true;  // Space
    case 0x2d: *out = shift ? '_' : '-'; return true;
    case 0x2e: *out = shift ? '+' : '='; return true;
    case 0x2f: *out = shift ? '{' : '['; return true;
    case 0x30: *out = shift ? '}' : ']'; return true;
    case 0x33: *out = shift ? ':' : ';'; return true;
    case 0x34: *out = shift ? '"' : '\''; return true;
    case 0x36: *out = shift ? '<' : ','; return true;
    case 0x37: *out = shift ? '>' : '.'; return true;
    case 0x38: *out = shift ? '?' : '/'; return true;
    default:
        return false;
    }
}

static bool contains_u8(const uint8_t *arr, size_t n, uint8_t v) {
    for (size_t i = 0; i < n; i++) {
        if (arr[i] == v) return true;
    }
    return false;
}

static void keylog_task(void *arg) {
    (void)arg;

    uint8_t prev_keys[6] = {0};

    while (true) {
        keylog_msg_t m = {0};
        if (xQueueReceive(s_q, &m, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        if (!s_enabled) {
            continue;
        }

        // Raw dump.
        printf("hid report id=%u len=%u:", (unsigned)m.report_id, (unsigned)m.len);
        for (unsigned i = 0; i < (unsigned)m.len; i++) {
            printf(" %02x", (unsigned)m.data[i]);
        }
        printf("\n");

        // Attempt to decode as "boot keyboard" report.
        if (m.len < 8) {
            continue;
        }

        const uint8_t modifiers = m.data[0];
        const bool shift = (modifiers & (1u << 1)) || (modifiers & (1u << 5)); // LShift/RShift
        const uint8_t *keys = &m.data[2];                                      // 6 key slots

        for (int i = 0; i < 6; i++) {
            const uint8_t kc = keys[i];
            if (kc == 0) continue;
            if (contains_u8(prev_keys, 6, kc)) continue;

            char ch = 0;
            if (hid_kbd_usage_to_ascii(kc, shift, &ch)) {
                if (ch == '\n') {
                    printf("key down: usage=0x%02x (enter)\n", (unsigned)kc);
                } else if (ch >= 0x20 && ch <= 0x7e) {
                    printf("key down: usage=0x%02x char='%c'\n", (unsigned)kc, ch);
                } else {
                    printf("key down: usage=0x%02x char=0x%02x\n", (unsigned)kc, (unsigned)(uint8_t)ch);
                }
            } else {
                printf("key down: usage=0x%02x modifiers=0x%02x\n", (unsigned)kc, (unsigned)modifiers);
            }
        }

        for (int i = 0; i < 6; i++) {
            const uint8_t prev = prev_keys[i];
            if (prev == 0) continue;
            if (contains_u8(keys, 6, prev)) continue;
            printf("key up: usage=0x%02x\n", (unsigned)prev);
        }

        memcpy(prev_keys, keys, 6);
    }
}

void keylog_init(void) {
    s_q = xQueueCreate(32, sizeof(keylog_msg_t));
    if (!s_q) {
        ESP_LOGE(TAG, "xQueueCreate failed");
        return;
    }
    xTaskCreate(keylog_task, "keylog", 4096, NULL, 10, NULL);
}

void keylog_set_enabled(bool enabled) {
    s_enabled = enabled;
}

bool keylog_is_enabled(void) {
    return s_enabled;
}

void keylog_submit_report(uint8_t report_id, const uint8_t *data, uint16_t len) {
    if (!s_q || !data || len == 0) {
        return;
    }
    keylog_msg_t m = {0};
    m.report_id = report_id;
    if (len > sizeof(m.data)) len = (uint16_t)sizeof(m.data);
    m.len = len;
    memcpy(m.data, data, len);
    (void)xQueueSend(s_q, &m, 0);
}

