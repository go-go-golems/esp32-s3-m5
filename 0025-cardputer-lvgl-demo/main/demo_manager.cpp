#include "demo_manager.h"

#include "esp_log.h"

static const char *TAG = "demo_mgr";

lv_obj_t *demo_menu_create(DemoManager *mgr);
void demo_menu_bind_group(DemoManager *mgr);
lv_obj_t *demo_basics_create(DemoManager *mgr);
void demo_basics_bind_group(DemoManager *mgr);
lv_obj_t *demo_pomodoro_create(DemoManager *mgr);
void demo_pomodoro_bind_group(DemoManager *mgr);
void demo_pomodoro_apply_minutes(DemoManager *mgr, int minutes);
lv_obj_t *demo_split_console_create(DemoManager *mgr);
void demo_split_console_bind_group(DemoManager *mgr);

static lv_obj_t *create_screen_for(DemoManager *mgr, DemoId id) {
    switch (id) {
    case DemoId::Menu: return demo_menu_create(mgr);
    case DemoId::Basics: return demo_basics_create(mgr);
    case DemoId::Pomodoro: return demo_pomodoro_create(mgr);
    case DemoId::SplitConsole: return demo_split_console_create(mgr);
    }
    return nullptr;
}

void demo_manager_init(DemoManager *mgr, lv_indev_t *kb_indev) {
    if (!mgr) return;
    mgr->kb_indev = kb_indev;
    mgr->group = lv_group_create();
    lv_group_set_default(mgr->group);
    lv_group_set_wrap(mgr->group, true);

    if (mgr->kb_indev) {
        lv_indev_set_group(mgr->kb_indev, mgr->group);
    } else {
        ESP_LOGW(TAG, "kb indev is null; no keypad group binding");
    }
}

void demo_manager_load(DemoManager *mgr, DemoId id) {
    if (!mgr) return;

    lv_obj_t *cur = lv_scr_act();
    lv_obj_t *next = create_screen_for(mgr, id);
    if (!next) {
        ESP_LOGE(TAG, "failed to create screen for demo id=%d", (int)id);
        return;
    }

    mgr->active = id;

    if (mgr->group) {
        lv_group_remove_all_objs(mgr->group);
        switch (id) {
        case DemoId::Menu: demo_menu_bind_group(mgr); break;
        case DemoId::Basics: demo_basics_bind_group(mgr); break;
        case DemoId::Pomodoro: demo_pomodoro_bind_group(mgr); break;
        case DemoId::SplitConsole: demo_split_console_bind_group(mgr); break;
        }
    }

    lv_scr_load(next);
    if (cur && cur != next) {
        lv_obj_del_async(cur);
    }
}

bool demo_manager_handle_global_key(DemoManager *mgr, uint32_t key) {
    if (!mgr) return false;

    if (key == LV_KEY_ESC && mgr->active != DemoId::Menu) {
        demo_manager_load(mgr, DemoId::Menu);
        return true;
    }

    return false;
}

void demo_manager_pomodoro_set_minutes(DemoManager *mgr, int minutes) {
    if (!mgr) return;
    if (minutes < 1) minutes = 1;
    if (minutes > 99) minutes = 99;
    mgr->pomodoro_minutes = minutes;

    if (mgr->active == DemoId::Pomodoro) {
        demo_pomodoro_apply_minutes(mgr, minutes);
    }
}
