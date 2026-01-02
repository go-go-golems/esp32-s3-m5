#include "lvgl_port_cardputer_kb.h"

#include <deque>

uint32_t lvgl_port_cardputer_kb_translate(const KeyEvent &ev) {
    if (ev.key == "up") return LV_KEY_UP;
    if (ev.key == "down") return LV_KEY_DOWN;
    if (ev.key == "left") return LV_KEY_LEFT;
    if (ev.key == "right") return LV_KEY_RIGHT;
    if (ev.key == "enter") return LV_KEY_ENTER;
    if (ev.key == "tab") return LV_KEY_NEXT;
    if (ev.key == "esc") return LV_KEY_ESC;
    if (ev.key == "bksp") return LV_KEY_BACKSPACE;
    if (ev.key == "space") return (uint32_t)' ';

    if (ev.key.size() == 1) {
        return (uint32_t)(unsigned char)ev.key[0];
    }

    return 0;
}

namespace {

struct KeyQueue {
    std::deque<uint32_t> keys;
    size_t max = 64;

    void push(uint32_t key) {
        if (keys.size() >= max) {
            keys.pop_front();
        }
        keys.push_back(key);
    }

    bool pop(uint32_t *out) {
        if (keys.empty()) {
            return false;
        }
        *out = keys.front();
        keys.pop_front();
        return true;
    }
};

static KeyQueue s_queue;
static lv_indev_t *s_indev = nullptr;

static void indev_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    (void)drv;
    uint32_t key = 0;
    if (s_queue.pop(&key) && key != 0) {
        data->state = LV_INDEV_STATE_PR;
        data->key = key;
        return;
    }

    data->state = LV_INDEV_STATE_REL;
    data->key = 0;
}

} // namespace

lv_indev_t *lvgl_port_cardputer_kb_init(void) {
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = indev_read_cb;
    s_indev = lv_indev_drv_register(&indev_drv);
    return s_indev;
}

void lvgl_port_cardputer_kb_feed(const std::vector<KeyEvent> &events) {
    for (const auto &ev : events) {
        const uint32_t key = lvgl_port_cardputer_kb_translate(ev);
        if (key != 0) {
            s_queue.push(key);
        }
    }
}
