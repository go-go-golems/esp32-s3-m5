#include "ui_model.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

namespace {

static float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int clampi(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int step_px(uint8_t mods)
{
    if (mods & UI_MOD_CTRL) return 120;
    if (mods & UI_MOD_ALT) return 64;
    if (mods & UI_MOD_FN) return 8;
    return 24;
}

static int nearest_index(const UiState *ui)
{
    if (!ui || ui->line_step_px <= 0.0f) return 0;
    const float center_px = ui->scroll.pos_px + (float)ui->screen_w * 0.5f;
    const int idx = (int)lroundf(center_px / ui->line_step_px);
    return clampi(idx, 0, ui->line_count - 1);
}

static void clamp_targets(UiState *ui)
{
    ui->scroll.target_px = clampf(ui->scroll.target_px, ui->scroll.min_px, ui->scroll.max_px);
    ui->scroll.pos_px = clampf(ui->scroll.pos_px, ui->scroll.min_px, ui->scroll.max_px);
    ui->selected_index = nearest_index(ui);
    ui->active_index = ui->selected_index;
}

static void nudge(UiState *ui, int delta_px)
{
    ui->scroll.target_px += (float)delta_px;
    clamp_targets(ui);
    ui->dirty = true;
}

} // namespace

void ui_model_init(UiState *ui, int screen_w, int screen_h, int line_count, uint32_t autoplay_period_ms)
{
    memset(ui, 0, sizeof(*ui));
    ui->mode = UiMode::Browse;
    ui->autoplay = false;
    ui->autoplay_dir = 1;
    ui->autoplay_period_ms = autoplay_period_ms;
    ui->screen_w = screen_w;
    ui->screen_h = screen_h;
    ui->line_count = line_count;
    ui->line_step_px = 28.0f;
    ui->virtual_width_px = (float)((line_count - 1) > 0 ? (line_count - 1) : 0) * ui->line_step_px;
    ui->scroll.pos_px = 0.0f;
    ui->scroll.target_px = 0.0f;
    ui->scroll.min_px = 0.0f;
    ui->scroll.max_px = ui->virtual_width_px;
    ui->scroll.easing = 0.18f;
    ui->scroll.snap_epsilon_px = 0.2f;
    ui->scroll.animating = false;
    ui->dirty = true;
    strcpy(ui->last_text, "-");
    clamp_targets(ui);
}

void ui_model_handle_event(UiState *ui, const ui_key_event_t *ev)
{
    if (!ui || !ev) return;

    ui->last_kind = ev->kind;
    ui->last_mods = ev->mods;
    ui->last_keynum = ev->keynum;
    if (ev->kind == UI_KEY_TEXT && ev->text[0]) {
        strncpy(ui->last_text, ev->text, sizeof(ui->last_text) - 1);
        ui->last_text[sizeof(ui->last_text) - 1] = '\0';
    } else {
        strcpy(ui->last_text, "-");
    }

    if (ev->kind == UI_KEY_TAB) {
        ui->mode = (ui->mode == UiMode::Help) ? UiMode::Browse : UiMode::Help;
        ui->dirty = true;
        return;
    }

    if (ev->kind == UI_KEY_BACK) {
        ui->mode = UiMode::Browse;
        ui->autoplay = false;
        ui->dirty = true;
        return;
    }

    if (ui->mode == UiMode::Help) {
        if (ev->kind == UI_KEY_ENTER) {
            ui->mode = UiMode::Browse;
            ui->dirty = true;
        }
        return;
    }

    switch (ev->kind) {
    case UI_KEY_LEFT:
        nudge(ui, -step_px(ev->mods));
        break;
    case UI_KEY_RIGHT:
        nudge(ui, step_px(ev->mods));
        break;
    case UI_KEY_UP:
        nudge(ui, -4 * step_px(ev->mods));
        break;
    case UI_KEY_DOWN:
        nudge(ui, 4 * step_px(ev->mods));
        break;
    case UI_KEY_ENTER:
    case UI_KEY_SPACE:
        ui->autoplay = !ui->autoplay;
        ui->dirty = true;
        break;
    case UI_KEY_DEL:
        ui->scroll.target_px = 0.0f;
        clamp_targets(ui);
        ui->dirty = true;
        break;
    case UI_KEY_TEXT:
        if (strcmp(ev->text, "r") == 0 || strcmp(ev->text, "R") == 0) {
            ui->scroll.pos_px = 0.0f;
            ui->scroll.target_px = 0.0f;
            ui->autoplay = false;
            clamp_targets(ui);
            ui->dirty = true;
        } else if (strcmp(ev->text, "1") == 0) {
            ui->scroll.easing = 0.10f;
            ui->dirty = true;
        } else if (strcmp(ev->text, "2") == 0) {
            ui->scroll.easing = 0.18f;
            ui->dirty = true;
        } else if (strcmp(ev->text, "3") == 0) {
            ui->scroll.easing = 0.28f;
            ui->dirty = true;
        }
        break;
    default:
        break;
    }
}

bool ui_model_tick(UiState *ui, int64_t now_us)
{
    if (!ui) return false;

    bool changed = false;

    if (ui->autoplay) {
        const int64_t period_us = (int64_t)ui->autoplay_period_ms * 1000;
        if (ui->last_autoplay_us == 0 || (now_us - ui->last_autoplay_us) >= period_us) {
            ui->last_autoplay_us = now_us;
            ui->scroll.target_px += (float)(ui->autoplay_dir * 20);
            if (ui->scroll.target_px >= ui->scroll.max_px) {
                ui->scroll.target_px = ui->scroll.max_px;
                ui->autoplay_dir = -1;
            }
            if (ui->scroll.target_px <= ui->scroll.min_px) {
                ui->scroll.target_px = ui->scroll.min_px;
                ui->autoplay_dir = 1;
            }
            changed = true;
        }
    }

    const float delta = ui->scroll.target_px - ui->scroll.pos_px;
    ui->scroll.pos_px += delta * ui->scroll.easing;
    if (fabsf(delta) < ui->scroll.snap_epsilon_px) {
        ui->scroll.pos_px = ui->scroll.target_px;
        ui->scroll.animating = false;
    } else {
        ui->scroll.animating = true;
        changed = true;
    }

    clamp_targets(ui);

    if (ui->dirty) {
        changed = true;
        ui->dirty = false;
    }
    return changed;
}
