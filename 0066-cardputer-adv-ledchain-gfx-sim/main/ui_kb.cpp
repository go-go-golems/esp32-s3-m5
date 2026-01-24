#include "ui_kb.h"

#include <string.h>

#include "sdkconfig.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include "freertos/task.h"

#include "tca8418.h"

namespace {

static const char *TAG = "0066_kb";

struct KeyValue {
    const char *first;
    const char *second;
};

// Vendor legend matches the "picture" coordinate system (4x14).
// Cardputer-ADV keyboard: TCA8418 event remapping yields these same (row,col) positions.
// Derived from `0036-cardputer-adv-led-matrix-console/main/matrix_console.c`.
constexpr KeyValue kKeyMap[4][14] = {
    {{"`", "~"}, {"1", "!"}, {"2", "@"}, {"3", "#"}, {"4", "$"}, {"5", "%"}, {"6", "^"}, {"7", "&"}, {"8", "*"}, {"9", "("}, {"0", ")"}, {"-", "_"}, {"=", "+"}, {"del", "del"}},
    {{"tab", "tab"}, {"q", "Q"}, {"w", "W"}, {"e", "E"}, {"r", "R"}, {"t", "T"}, {"y", "Y"}, {"u", "U"}, {"i", "I"}, {"o", "O"}, {"p", "P"}, {"[", "{"}, {"]", "}"}, {"\\", "|"}},
    {{"shift", "shift"}, {"capslock", "capslock"}, {"a", "A"}, {"s", "S"}, {"d", "D"}, {"f", "F"}, {"g", "G"}, {"h", "H"}, {"j", "J"}, {"k", "K"}, {"l", "L"}, {";", ":"}, {"'", "\""}, {"enter", "enter"}},
    {{"ctrl", "ctrl"}, {"opt", "opt"}, {"alt", "alt"}, {"z", "Z"}, {"x", "X"}, {"c", "C"}, {"v", "V"}, {"b", "B"}, {"n", "N"}, {"m", "M"}, {",", "<"}, {".", ">"}, {"/", "?"}, {"space", "space"}},
};

static void q_send_best_effort(QueueHandle_t q, const ui_key_event_t &ev) {
    if (!q) return;
    (void)xQueueSend(q, &ev, 0);
}

struct KbdKey {
    bool pressed = false;
    uint8_t row = 0; // 0..3 in picture coords
    uint8_t col = 0; // 0..13 in picture coords
};

static bool decode_tca_event(uint8_t event_raw, KbdKey *out)
{
    if (!out) return false;
    if (event_raw == 0) return false;

    // TCA8418 KEY_EVENT: bit7 is state, bits6..0 are key number (1-based).
    out->pressed = (event_raw & 0x80) != 0;
    uint8_t keynum = (uint8_t)(event_raw & 0x7F);
    if (keynum == 0) return false;
    keynum--; // 0-based

    // Convert 1..80 key number into row/col for a 10-wide internal matrix, then remap to Cardputer-style layout.
    const uint8_t row = (uint8_t)(keynum / 10);
    const uint8_t col = (uint8_t)(keynum % 10);

    // Remap to match Cardputer "picture" coordinates (4 rows x 14 columns).
    uint8_t out_col = (uint8_t)(row * 2);
    if (col > 3) out_col++;
    uint8_t out_row = (uint8_t)((col + 4) % 4);

    out->row = out_row;
    out->col = out_col;
    return (out->row < 4 && out->col < 14);
}

static bool key_is(const KeyValue *kv, const char *s)
{
    if (!kv || !s) return false;
    return strcmp(kv->first, s) == 0;
}

static bool key_is_letter(const char *s)
{
    if (!s || s[0] == '\0' || s[1] != '\0') return false;
    const char c = s[0];
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static uint8_t mods_from_state(bool shift, bool ctrl, bool alt, bool opt)
{
    uint8_t mods = 0;
    if (shift) mods |= UI_MOD_SHIFT;
    if (ctrl) mods |= UI_MOD_CTRL;
    if (alt) mods |= UI_MOD_ALT;
    // Cardputer-ADV doesn't expose a dedicated "Fn" in the vendor map; we treat Opt as the Fn-like modifier.
    if (opt) mods |= UI_MOD_FN;
    return mods;
}

static i2c_master_bus_handle_t s_bus = nullptr;
static tca8418_t s_kbd = {};
static QueueHandle_t s_evt_q = nullptr;
static bool s_ready = false;

static void IRAM_ATTR kbd_int_isr(void *arg)
{
    (void)arg;
    if (!s_evt_q) return;
    const uint32_t one = 1;
    BaseType_t hp = pdFALSE;
    (void)xQueueSendFromISR(s_evt_q, &one, &hp);
    if (hp) portYIELD_FROM_ISR();
}

static esp_err_t kb_hw_init(void)
{
    if (s_ready) return ESP_OK;

    // Cardputer-ADV keyboard wiring: I2C0 SDA=GPIO8 SCL=GPIO9, INT=GPIO11, addr=0x34.
    // (See 0036-cardputer-adv-led-matrix-console.)
    static constexpr i2c_port_num_t kI2cPort = I2C_NUM_0;
    static constexpr gpio_num_t kSda = GPIO_NUM_8;
    static constexpr gpio_num_t kScl = GPIO_NUM_9;
    static constexpr gpio_num_t kInt = GPIO_NUM_11;
    static constexpr uint32_t kHz = 400000;
    static constexpr uint8_t kAddr7 = 0x34;
    static constexpr uint8_t kRows = 7;
    static constexpr uint8_t kCols = 8;

    if (!s_bus) {
        i2c_master_bus_config_t bus_cfg = {};
        bus_cfg.i2c_port = kI2cPort;
        bus_cfg.sda_io_num = kSda;
        bus_cfg.scl_io_num = kScl;
        bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
        bus_cfg.glitch_ignore_cnt = 7;
        bus_cfg.intr_priority = 0;
        bus_cfg.trans_queue_depth = 0; // synchronous mode
        bus_cfg.flags.enable_internal_pullup = 1;
        ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &s_bus), TAG, "i2c_new_master_bus failed");
    }

    ESP_RETURN_ON_ERROR(tca8418_open(&s_kbd, s_bus, kAddr7, kHz, kRows, kCols), TAG, "tca8418_open failed");
    ESP_RETURN_ON_ERROR(tca8418_begin(&s_kbd), TAG, "tca8418_begin failed");

    if (!s_evt_q) {
        s_evt_q = xQueueCreate(8, sizeof(uint32_t));
        if (!s_evt_q) return ESP_ERR_NO_MEM;
    }

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << (uint32_t)kInt);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG, "gpio_config failed");

    esp_err_t err = gpio_install_isr_service((int)ESP_INTR_FLAG_IRAM);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

    err = gpio_isr_handler_add(kInt, &kbd_int_isr, nullptr);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

    s_ready = true;
    return ESP_OK;
}

static void kb_task_main(void *arg) {
    QueueHandle_t q = (QueueHandle_t)arg;

    const esp_err_t init = kb_hw_init();
    if (init != ESP_OK) {
        ESP_LOGE(TAG, "keyboard init failed: %s", esp_err_to_name(init));
        vTaskDelete(nullptr);
        return;
    }

    bool shift = false;
    bool ctrl = false;
    bool alt = false;
    bool opt = false;
    bool caps = false;

    while (true) {
        uint32_t token = 0;
        (void)xQueueReceive(s_evt_q, &token, pdMS_TO_TICKS(CONFIG_TUTORIAL_0066_KB_SCAN_PERIOD_MS));

        for (;;) {
            uint8_t count = 0;
            const esp_err_t st = tca8418_available(&s_kbd, &count);
            if (st != ESP_OK || count == 0) break;

            for (uint8_t i = 0; i < count; i++) {
                uint8_t evt = 0;
                if (tca8418_get_event(&s_kbd, &evt) != ESP_OK) break;
                if (evt == 0) break;

                KbdKey k;
                if (!decode_tca_event(evt, &k)) continue;
                const KeyValue *kv = &kKeyMap[k.row][k.col];

                // Update modifier state. (We treat opt as "Fn-like".)
                if (key_is(kv, "shift")) shift = k.pressed;
                if (key_is(kv, "ctrl")) ctrl = k.pressed;
                if (key_is(kv, "alt")) alt = k.pressed;
                if (key_is(kv, "opt")) opt = k.pressed;
                if (key_is(kv, "capslock") && k.pressed) caps = !caps;

                // Only emit actions on press.
                if (!k.pressed) continue;

                const uint8_t mods = mods_from_state(shift, ctrl, alt, opt);

                // Special key kinds.
                if (key_is(kv, "tab")) {
                    ui_key_event_t ev = {.kind = UI_KEY_TAB, .mods = mods, .text = {0}, .keynum = 0};
                    q_send_best_effort(q, ev);
                    continue;
                }
                if (key_is(kv, "enter")) {
                    ui_key_event_t ev = {.kind = UI_KEY_ENTER, .mods = mods, .text = {0}, .keynum = 0};
                    q_send_best_effort(q, ev);
                    continue;
                }
                if (key_is(kv, "del")) {
                    ui_key_event_t ev = {.kind = UI_KEY_DEL, .mods = mods, .text = {0}, .keynum = 0};
                    q_send_best_effort(q, ev);
                    continue;
                }
                if (key_is(kv, "space")) {
                    ui_key_event_t ev = {.kind = UI_KEY_SPACE, .mods = mods, .text = {0}, .keynum = 0};
                    q_send_best_effort(q, ev);
                    continue;
                }

                const bool use_shifted = key_is_letter(kv->first) ? (shift ^ caps) : shift;
                const char *s = use_shifted ? kv->second : kv->first;
                if (!s || !*s) continue;

                ui_key_event_t ev = {};
                ev.kind = UI_KEY_TEXT;
                ev.mods = mods;
                ev.keynum = 0;
                strncpy(ev.text, s, sizeof(ev.text) - 1);
                ev.text[sizeof(ev.text) - 1] = '\0';
                q_send_best_effort(q, ev);
            }
        }

        // Try to clear K_INT once we've drained events.
        (void)tca8418_write_reg8(&s_kbd, TCA8418_REG_INT_STAT, TCA8418_INTSTAT_K_INT);
    }
}

} // namespace

extern "C" void ui_kb_start(QueueHandle_t q) {
    // Stack is conservative: i2c + event decode + C++.
    (void)xTaskCreate(&kb_task_main, "0066_kb", 4096, (void *)q, 6, nullptr);
}

extern "C" bool ui_kb_is_ready(void)
{
    return s_ready;
}
