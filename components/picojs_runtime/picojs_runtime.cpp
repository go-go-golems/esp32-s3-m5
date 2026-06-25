#include "picojs_runtime.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>

#include "esp_log.h"

namespace {
constexpr const char *kTag = "picojs_runtime";
constexpr uint32_t kDefaultFrameIntervalMs = 100;
constexpr size_t kMaxCells = PICOJS_RUNTIME_DEFAULT_COLS * PICOJS_RUNTIME_DEFAULT_ROWS;
} // namespace

struct ScreenCell {
    char ch = ' ';
};

struct picojs_runtime {
    bool initialized = false;
    uint16_t cols = PICOJS_RUNTIME_DEFAULT_COLS;
    uint16_t rows = PICOJS_RUNTIME_DEFAULT_ROWS;
    uint32_t frame_interval_ms = kDefaultFrameIntervalMs;
    uint32_t frame_count = 0;
    uint32_t app_count = 0;
    uint32_t mounted_app_count = 0;
    uint32_t last_frame_ms = 0;
    uint32_t last_error_count = 0;
    ScreenCell cells[kMaxCells];
    char last_key[16] = {};
};

namespace {
void clear_screen(picojs_runtime *rt)
{
    if (!rt) return;
    for (auto &cell : rt->cells) cell.ch = ' ';
}

void put_text(picojs_runtime *rt, uint16_t x, uint16_t y, const char *text)
{
    if (!rt || !text || y >= rt->rows) return;
    for (uint16_t col = x; col < rt->cols && *text; ++col, ++text) {
        rt->cells[y * rt->cols + col].ch = (*text >= 0x20 && *text <= 0x7e) ? *text : '?';
    }
}

void render_banner(picojs_runtime *rt)
{
    if (!rt) return;
    clear_screen(rt);
    put_text(rt, 0, 0, "PicoJS runtime ready");
    char line[64] = {};
    std::snprintf(line, sizeof(line), "grid=%ux%u frame=%u", rt->cols, rt->rows, (unsigned)rt->frame_count);
    put_text(rt, 0, 1, line);
    if (rt->last_key[0]) {
        std::snprintf(line, sizeof(line), "last_key=%s", rt->last_key);
        put_text(rt, 0, 2, line);
    }
}
} // namespace

esp_err_t picojs_runtime_create(const picojs_runtime_config_t *cfg, picojs_runtime_t **out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = nullptr;

    auto rt = std::make_unique<picojs_runtime>();
    if (cfg) {
        rt->cols = cfg->cols ? cfg->cols : PICOJS_RUNTIME_DEFAULT_COLS;
        rt->rows = cfg->rows ? cfg->rows : PICOJS_RUNTIME_DEFAULT_ROWS;
        rt->frame_interval_ms = cfg->frame_interval_ms ? cfg->frame_interval_ms : kDefaultFrameIntervalMs;
    }
    if (rt->cols == 0 || rt->rows == 0 || static_cast<size_t>(rt->cols) * rt->rows > kMaxCells) {
        return ESP_ERR_INVALID_ARG;
    }

    rt->initialized = true;
    render_banner(rt.get());
    ESP_LOGI(kTag, "runtime initialized: %ux%u cells frame_interval=%ums",
             rt->cols, rt->rows, (unsigned)rt->frame_interval_ms);
    *out = rt.release();
    return ESP_OK;
}

void picojs_runtime_destroy(picojs_runtime_t *rt)
{
    delete rt;
}

esp_err_t picojs_runtime_get_status(picojs_runtime_t *rt, picojs_runtime_status_t *out)
{
    if (!rt || !out) return ESP_ERR_INVALID_ARG;
    *out = {};
    out->initialized = rt->initialized;
    out->cols = rt->cols;
    out->rows = rt->rows;
    out->frame_count = rt->frame_count;
    out->app_count = rt->app_count;
    out->mounted_app_count = rt->mounted_app_count;
    out->last_frame_ms = rt->last_frame_ms;
    out->last_error_count = rt->last_error_count;
    return ESP_OK;
}

esp_err_t picojs_runtime_frame(picojs_runtime_t *rt, uint32_t dt_ms)
{
    if (!rt || !rt->initialized) return ESP_ERR_INVALID_STATE;
    rt->last_frame_ms = dt_ms;
    ++rt->frame_count;
    render_banner(rt);
    return ESP_OK;
}

esp_err_t picojs_runtime_key(picojs_runtime_t *rt, const char *token)
{
    if (!rt || !rt->initialized) return ESP_ERR_INVALID_STATE;
    if (!token || token[0] == 0) return ESP_ERR_INVALID_ARG;
    std::snprintf(rt->last_key, sizeof(rt->last_key), "%s", token);
    render_banner(rt);
    return ESP_OK;
}

esp_err_t picojs_runtime_dump_text(picojs_runtime_t *rt, char *dst, size_t dst_len)
{
    if (!rt || !rt->initialized) return ESP_ERR_INVALID_STATE;
    if (!dst || dst_len == 0) return ESP_ERR_INVALID_ARG;

    const size_t row_dump_len = 5 + rt->cols + 1; // "[NN] " + cells + "\n"
    const size_t required = static_cast<size_t>(rt->rows) * row_dump_len + 1;
    if (dst_len < required) return ESP_ERR_INVALID_SIZE;

    char *out = dst;
    size_t remaining = dst_len;
    for (uint16_t row = 0; row < rt->rows; ++row) {
        const int written = std::snprintf(out, remaining, "[%02u] ", row);
        if (written < 0 || static_cast<size_t>(written) >= remaining) return ESP_ERR_INVALID_SIZE;
        out += written;
        remaining -= written;
        for (uint16_t col = 0; col < rt->cols; ++col) {
            if (remaining <= 1) return ESP_ERR_INVALID_SIZE;
            *out++ = rt->cells[row * rt->cols + col].ch;
            --remaining;
        }
        if (remaining <= 1) return ESP_ERR_INVALID_SIZE;
        *out++ = '\n';
        --remaining;
        *out = 0;
    }
    return ESP_OK;
}
