#include <M5Unified.hpp>

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "esp_console.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#ifndef PAPERS3_MATRIX_CELL
#define PAPERS3_MATRIX_CELL "LOCAL"
#endif
#ifndef PAPERS3_M5GFX_SHA
#define PAPERS3_M5GFX_SHA "unrecorded"
#endif
#ifndef PAPERS3_M5UNIFIED_SHA
#define PAPERS3_M5UNIFIED_SHA "unrecorded"
#endif

namespace {

constexpr char kTag[] = "epd_qual";
constexpr uint32_t kPaperWhite = 0xFFFFFF;
constexpr uint32_t kInkBlack = 0x000000;
constexpr uint32_t kMidGray = 0x888888;
constexpr uint32_t kMaxSoakIterations = 10000;

SemaphoreHandle_t g_display_mutex = nullptr;
bool g_display_ready = false;
uint64_t g_update_count = 0;
uint64_t g_full_update_count = 0;
uint64_t g_partial_update_count = 0;
uint64_t g_total_update_us = 0;
uint64_t g_max_update_us = 0;

class DisplayGuard {
public:
    DisplayGuard()
    {
        if (g_display_mutex != nullptr) {
            locked_ = xSemaphoreTake(g_display_mutex, portMAX_DELAY) == pdTRUE;
        }
    }

    ~DisplayGuard()
    {
        if (locked_) {
            xSemaphoreGive(g_display_mutex);
        }
    }

    bool locked() const { return locked_; }

private:
    bool locked_ = false;
};

const char* ResetReasonName(esp_reset_reason_t reason)
{
    switch (reason) {
    case ESP_RST_POWERON: return "power-on";
    case ESP_RST_EXT: return "external";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "interrupt-watchdog";
    case ESP_RST_TASK_WDT: return "task-watchdog";
    case ESP_RST_WDT: return "watchdog";
    case ESP_RST_DEEPSLEEP: return "deep-sleep";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_SDIO: return "sdio";
    case ESP_RST_USB: return "usb";
    case ESP_RST_JTAG: return "jtag";
    case ESP_RST_EFUSE: return "efuse";
    case ESP_RST_PWR_GLITCH: return "power-glitch";
    case ESP_RST_CPU_LOCKUP: return "cpu-lockup";
    default: return "other";
    }
}

void RecordUpdate(bool full, uint64_t elapsed_us)
{
    ++g_update_count;
    if (full) {
        ++g_full_update_count;
    } else {
        ++g_partial_update_count;
    }
    g_total_update_us += elapsed_us;
    g_max_update_us = std::max(g_max_update_us, elapsed_us);
}

template <typename DrawFn>
bool DrawTransaction(epd_mode_t mode, bool full, DrawFn&& draw)
{
    if (!g_display_ready) {
        std::printf("error: display is not ready\n");
        return false;
    }

    const int64_t started = esp_timer_get_time();
    M5.Display.waitDisplay();
    M5.Display.setEpdMode(mode);
    M5.Display.startWrite();
    draw();
    M5.Display.endWrite();
    M5.Display.waitDisplay();
    const uint64_t elapsed = static_cast<uint64_t>(esp_timer_get_time() - started);
    RecordUpdate(full, elapsed);
    return true;
}

bool ExplicitDisplayRange(int32_t x, int32_t y, int32_t w, int32_t h)
{
    if (!g_display_ready) {
        return false;
    }
    const int64_t started = esp_timer_get_time();
    M5.Display.waitDisplay();
    M5.Display.display(x, y, w, h);
    M5.Display.waitDisplay();
    RecordUpdate(false, static_cast<uint64_t>(esp_timer_get_time() - started));
    return true;
}

void ConfigureTextDefaults(uint32_t foreground = kInkBlack, uint32_t background = kPaperWhite)
{
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextFont(2);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(foreground, background);
}

bool FillScreenWithMode(uint32_t color, epd_mode_t mode)
{
    M5.Display.setRotation(1);
    return DrawTransaction(mode, true, [color]() {
        M5.Display.fillScreen(color);
    });
}

bool FillScreen(uint32_t color)
{
    return FillScreenWithMode(color, epd_mode_t::epd_quality);
}

bool DrawGrayBarsWithMode(epd_mode_t mode)
{
    M5.Display.setRotation(1);
    return DrawTransaction(mode, true, []() {
        const int32_t width = M5.Display.width();
        const int32_t height = M5.Display.height();
        for (int32_t i = 0; i < 16; ++i) {
            const uint32_t value = static_cast<uint32_t>(i * 17);
            const uint32_t gray = (value << 16) | (value << 8) | value;
            const int32_t x1 = (width * i) / 16;
            const int32_t x2 = (width * (i + 1)) / 16;
            M5.Display.fillRect(x1, 0, x2 - x1, height, gray);
        }
    });
}

bool DrawGrayBars()
{
    return DrawGrayBarsWithMode(epd_mode_t::epd_quality);
}

bool ParseWaveform(const char* name, epd_mode_t* mode)
{
    if (std::strcmp(name, "quality") == 0) *mode = epd_mode_t::epd_quality;
    else if (std::strcmp(name, "text") == 0) *mode = epd_mode_t::epd_text;
    else if (std::strcmp(name, "fast") == 0) *mode = epd_mode_t::epd_fast;
    else if (std::strcmp(name, "fastest") == 0) *mode = epd_mode_t::epd_fastest;
    else return false;
    return true;
}

bool DrawWaveformScene(const char* waveform_name, const char* scene_name)
{
    epd_mode_t mode;
    if (!ParseWaveform(waveform_name, &mode)) {
        std::printf("error: waveform must be quality, text, fast, or fastest\n");
        return false;
    }
    std::printf("waveform.name=%s waveform.id=%d scene=%s\n", waveform_name, static_cast<int>(mode), scene_name);
    if (std::strcmp(scene_name, "black") == 0) return FillScreenWithMode(kInkBlack, mode);
    if (std::strcmp(scene_name, "white") == 0) return FillScreenWithMode(kPaperWhite, mode);
    if (std::strcmp(scene_name, "gray") == 0) return DrawGrayBarsWithMode(mode);
    std::printf("error: waveform scene must be black, white, or gray\n");
    return false;
}

bool DrawWaveformComparison()
{
    struct WaveformEntry {
        const char* name;
        epd_mode_t mode;
    };
    static constexpr WaveformEntry kWaveforms[] = {
        {"QUALITY", epd_mode_t::epd_quality},
        {"TEXT", epd_mode_t::epd_text},
        {"FAST", epd_mode_t::epd_fast},
        {"FASTEST", epd_mode_t::epd_fastest},
    };

    M5.Display.setRotation(1);
    if (!FillScreenWithMode(kPaperWhite, epd_mode_t::epd_quality)) {
        return false;
    }
    const int32_t width = M5.Display.width();
    const int32_t height = M5.Display.height();
    for (int32_t i = 0; i < 4; ++i) {
        const int32_t x1 = (width * i) / 4;
        const int32_t x2 = (width * (i + 1)) / 4;
        const WaveformEntry& waveform = kWaveforms[i];
        if (!DrawTransaction(waveform.mode, false, [=]() {
                ConfigureTextDefaults();
                M5.Display.setTextFont(2);
                M5.Display.drawString(waveform.name, x1 + 12, 16);
                M5.Display.fillRect(x1 + 4, 54, x2 - x1 - 8, height - 62, kInkBlack);
            })) {
            return false;
        }
        std::printf("waveform_compare.column=%" PRId32 " mode=%s result=pass\n", i, waveform.name);
    }
    return heap_caps_check_integrity_all(true);
}

bool DrawCheckerboard()
{
    M5.Display.setRotation(1);
    return DrawTransaction(epd_mode_t::epd_text, true, []() {
        constexpr int32_t cell = 30;
        const int32_t width = M5.Display.width();
        const int32_t height = M5.Display.height();
        M5.Display.fillScreen(kPaperWhite);
        for (int32_t y = 0; y < height; y += cell) {
            for (int32_t x = 0; x < width; x += cell) {
                if (((x / cell) + (y / cell)) & 1) {
                    M5.Display.fillRect(x, y, std::min(cell, width - x), std::min(cell, height - y), kInkBlack);
                }
            }
        }
    });
}

bool DrawTextScene(const char* heading = "PaperS3 EPD qualification")
{
    M5.Display.setRotation(1);
    return DrawTransaction(epd_mode_t::epd_quality, true, [heading]() {
        M5.Display.fillScreen(kPaperWhite);
        ConfigureTextDefaults();
        M5.Display.setTextFont(4);
        M5.Display.drawString(heading, 28, 24);
        M5.Display.setTextFont(2);
        M5.Display.drawString("Native framebuffer + Panel_EPD waveform test", 30, 72);
        M5.Display.drawFastHLine(30, 102, M5.Display.width() - 60, kInkBlack);

        char line[192];
        std::snprintf(line, sizeof(line), "cell=%s  ESP-IDF=%s", PAPERS3_MATRIX_CELL, esp_get_idf_version());
        M5.Display.drawString(line, 30, 126);
        std::snprintf(line, sizeof(line), "M5GFX=%s", PAPERS3_M5GFX_SHA);
        M5.Display.drawString(line, 30, 152);
        std::snprintf(line, sizeof(line), "M5Unified=%s", PAPERS3_M5UNIFIED_SHA);
        M5.Display.drawString(line, 30, 178);
        std::snprintf(line, sizeof(line), "logical=%" PRId32 "x%" PRId32 " rotation=%u board=%d",
                      M5.Display.width(), M5.Display.height(), static_cast<unsigned>(M5.Display.getRotation()),
                      static_cast<int>(M5.getBoard()));
        M5.Display.drawString(line, 30, 204);

        M5.Display.setTextFont(4);
        M5.Display.drawString("Aa Bb Cc 0123456789", 30, 256);
        M5.Display.setTextFont(2);
        M5.Display.drawString("The quick brown fox jumps over the lazy dog.", 30, 306);
        M5.Display.drawString("Corners and short lines expose rotation and range errors.", 30, 334);

        for (int32_t i = 0; i < 8; ++i) {
            const uint32_t value = static_cast<uint32_t>(i * 36);
            const uint32_t gray = (value << 16) | (value << 8) | value;
            M5.Display.fillRect(30 + i * 108, 390, 88, 56, gray);
        }
        M5.Display.drawRect(20, 14, M5.Display.width() - 40, M5.Display.height() - 28, kMidGray);
    });
}

bool DrawScene(const char* name)
{
    if (std::strcmp(name, "white") == 0) return FillScreen(kPaperWhite);
    if (std::strcmp(name, "black") == 0) return FillScreen(kInkBlack);
    if (std::strcmp(name, "gray") == 0) return DrawGrayBars();
    if (std::strcmp(name, "checker") == 0) return DrawCheckerboard();
    if (std::strcmp(name, "text") == 0) return DrawTextScene();
    std::printf("error: unknown scene '%s'\n", name);
    return false;
}

bool DrawTinyRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
    return DrawTransaction(epd_mode_t::epd_text, false, [=]() {
        M5.Display.fillRect(x, y, w, h, color);
    });
}

bool RunBoundaryRotation(uint8_t rotation)
{
    M5.Display.setRotation(rotation);
    const int32_t width = M5.Display.width();
    const int32_t height = M5.Display.height();
    std::printf("boundary.rotation=%u logical=%" PRId32 "x%" PRId32 "\n", rotation, width, height);

    if (!DrawTransaction(epd_mode_t::epd_quality, true, []() { M5.Display.fillScreen(kPaperWhite); })) {
        return false;
    }

    for (int32_t extent = 1; extent <= 16; ++extent) {
        const int32_t y = 8 + ((height - 24) * (extent - 1)) / 16;
        const int32_t x = 8 + ((width - 24) * (extent - 1)) / 16;
        const uint32_t color = (extent & 1) ? kInkBlack : kMidGray;
        const int32_t horizontal_h = std::min<int32_t>(7, height);
        const int32_t vertical_w = std::min<int32_t>(7, width);

        if (!DrawTinyRect(0, y, std::min(extent, width), horizontal_h, color)) return false;
        if (!DrawTinyRect(std::max<int32_t>(0, width - extent), y, std::min(extent, width), horizontal_h, color)) return false;
        if (!DrawTinyRect(x, 0, vertical_w, std::min(extent, height), color)) return false;
        if (!DrawTinyRect(x, std::max<int32_t>(0, height - extent), vertical_w, std::min(extent, height), color)) return false;

        if (!heap_caps_check_integrity_all(true)) {
            std::printf("boundary.heap_integrity=failed rotation=%u extent=%" PRId32 "\n", rotation, extent);
            return false;
        }
    }

    // Exercise the explicit logical full-range path from M5GFX Issue 181.
    if (!ExplicitDisplayRange(0, 0, width, height)) {
        return false;
    }
    const bool integrity = heap_caps_check_integrity_all(true);
    std::printf("boundary.rotation=%u result=%s\n", rotation, integrity ? "pass" : "heap-failed");
    return integrity;
}

bool RunBoundaries(const char* rotation_arg)
{
    if (std::strcmp(rotation_arg, "all") == 0) {
        for (uint8_t rotation = 0; rotation < 4; ++rotation) {
            if (!RunBoundaryRotation(rotation)) {
                return false;
            }
        }
        M5.Display.setRotation(1);
        return true;
    }

    char* end = nullptr;
    const long rotation = std::strtol(rotation_arg, &end, 10);
    if (end == rotation_arg || *end != '\0' || rotation < 0 || rotation > 3) {
        std::printf("error: rotation must be 0, 1, 2, 3, or all\n");
        return false;
    }
    return RunBoundaryRotation(static_cast<uint8_t>(rotation));
}

uint32_t NextRandom(uint32_t& state)
{
    state = state * 1664525u + 1013904223u;
    return state;
}

bool RunSoak(uint32_t iterations)
{
    if (iterations == 0 || iterations > kMaxSoakIterations) {
        std::printf("error: iterations must be 1..%" PRIu32 "\n", kMaxSoakIterations);
        return false;
    }

    M5.Display.setRotation(1);
    if (!FillScreen(kPaperWhite)) {
        return false;
    }

    uint32_t random = 0x503A3E5Du;
    const uint64_t start_updates = g_update_count;
    const int64_t started = esp_timer_get_time();
    for (uint32_t i = 0; i < iterations; ++i) {
        bool ok = false;
        if ((i + 1) % 128 == 0) {
            const uint32_t color = ((i / 128) & 1) ? kPaperWhite : kInkBlack;
            ok = FillScreen(color);
        } else {
            const int32_t width = 1 + static_cast<int32_t>(NextRandom(random) % 64);
            const int32_t height = 1 + static_cast<int32_t>(NextRandom(random) % 48);
            const int32_t max_x = std::max<int32_t>(1, M5.Display.width() - width + 1);
            const int32_t max_y = std::max<int32_t>(1, M5.Display.height() - height + 1);
            const int32_t x = static_cast<int32_t>(NextRandom(random) % static_cast<uint32_t>(max_x));
            const int32_t y = static_cast<int32_t>(NextRandom(random) % static_cast<uint32_t>(max_y));
            const uint32_t color = (NextRandom(random) & 1) ? kInkBlack : kPaperWhite;
            ok = DrawTransaction(epd_mode_t::epd_fast, false, [=]() {
                M5.Display.fillRect(x, y, width, height, color);
            });
        }
        if (!ok) {
            return false;
        }

        if ((i + 1) % 25 == 0 && !heap_caps_check_integrity_all(true)) {
            std::printf("soak.heap_integrity=failed iteration=%" PRIu32 "\n", i + 1);
            return false;
        }
        if ((i + 1) % 100 == 0 || i + 1 == iterations) {
            std::printf("soak.progress=%" PRIu32 "/%" PRIu32 " free_internal=%u free_spiram=%u largest_dma=%u\n",
                        i + 1, iterations,
                        static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
                        static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)),
                        static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_DMA)));
        }
        vTaskDelay(1);
    }

    const uint64_t elapsed_us = static_cast<uint64_t>(esp_timer_get_time() - started);
    std::printf("soak.result=pass iterations=%" PRIu32 " updates=%" PRIu64 " elapsed_ms=%" PRIu64 "\n",
                iterations, g_update_count - start_updates, elapsed_us / 1000);
    return true;
}

bool RunTextSoak(uint32_t iterations)
{
    if (iterations == 0 || iterations > kMaxSoakIterations) {
        std::printf("error: iterations must be 1..%" PRIu32 "\n", kMaxSoakIterations);
        return false;
    }

    M5.Display.setRotation(1);
    if (!DrawTransaction(epd_mode_t::epd_quality, true, []() {
            M5.Display.fillScreen(kPaperWhite);
            ConfigureTextDefaults();
            M5.Display.setTextFont(4);
            M5.Display.drawString("Text update soak", 40, 40);
        })) {
        return false;
    }

    const int64_t started = esp_timer_get_time();
    for (uint32_t i = 0; i < iterations; ++i) {
        const uint32_t update_number = i + 1;
        const bool ok = DrawTransaction(epd_mode_t::epd_fast, false, [update_number]() {
            char line[96];
            std::snprintf(line, sizeof(line), "update %06" PRIu32, update_number);
            M5.Display.fillRect(40, 108, 440, 64, kPaperWhite);
            ConfigureTextDefaults();
            M5.Display.setTextFont(4);
            M5.Display.drawString(line, 40, 120);
        });
        if (!ok) {
            return false;
        }
        if (update_number % 25 == 0 && !heap_caps_check_integrity_all(true)) {
            std::printf("text_soak.heap_integrity=failed iteration=%" PRIu32 "\n", update_number);
            return false;
        }
        if (update_number % 100 == 0 || update_number == iterations) {
            std::printf("text_soak.progress=%" PRIu32 "/%" PRIu32 " free_internal=%u free_spiram=%u\n",
                        update_number, iterations,
                        static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
                        static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)));
        }
        vTaskDelay(1);
    }

    std::printf("text_soak.result=pass iterations=%" PRIu32 " elapsed_ms=%" PRIu64 "\n", iterations,
                static_cast<uint64_t>(esp_timer_get_time() - started) / 1000);
    return true;
}

bool CycleDisplaySleep(uint32_t milliseconds)
{
    if (milliseconds > 60000) {
        std::printf("error: cycle-sleep maximum is 60000 ms\n");
        return false;
    }
    M5.Display.waitDisplay();
    const int64_t started = esp_timer_get_time();
    M5.Display.sleep();
    vTaskDelay(pdMS_TO_TICKS(milliseconds));
    M5.Display.wakeup();
    const uint64_t elapsed = static_cast<uint64_t>(esp_timer_get_time() - started);
    std::printf("display_sleep_cycle_ms=%" PRIu64 "\n", elapsed / 1000);
    return DrawTextScene("PaperS3 woke from display sleep");
}

void PrintStatus()
{
    const uint64_t average_us = g_update_count == 0 ? 0 : g_total_update_us / g_update_count;
    std::printf("qualification.cell=%s\n", PAPERS3_MATRIX_CELL);
    std::printf("qualification.m5gfx_sha=%s\n", PAPERS3_M5GFX_SHA);
    std::printf("qualification.m5unified_sha=%s\n", PAPERS3_M5UNIFIED_SHA);
    std::printf("idf.version=%s\n", esp_get_idf_version());
    std::printf("reset.reason=%s(%d)\n", ResetReasonName(esp_reset_reason()), static_cast<int>(esp_reset_reason()));
    std::printf("board.id=%d\n", static_cast<int>(M5.getBoard()));
    std::printf("display.count=%u\n", static_cast<unsigned>(M5.getDisplayCount()));
    std::printf("display.ready=%s\n", g_display_ready ? "yes" : "no");
    if (g_display_ready) {
        const auto& panel_config = M5.Display.getPanel()->config();
        std::printf("display.logical=%" PRId32 "x%" PRId32 "\n", M5.Display.width(), M5.Display.height());
        std::printf("display.physical=%ux%u\n", static_cast<unsigned>(panel_config.panel_width),
                    static_cast<unsigned>(panel_config.panel_height));
        std::printf("display.rotation=%u\n", static_cast<unsigned>(M5.Display.getRotation()));
        std::printf("display.epd=%s\n", M5.Display.isEPD() ? "yes" : "no");
        std::printf("display.epd_mode=%d\n", static_cast<int>(M5.Display.getEpdMode()));
    }
    std::printf("psram.bytes=%u\n", static_cast<unsigned>(esp_psram_get_size()));
    std::printf("heap.internal.free=%u\n",
                static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)));
    std::printf("heap.internal.largest=%u\n",
                static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)));
    std::printf("heap.dma.free=%u\n", static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_DMA)));
    std::printf("heap.dma.largest=%u\n", static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_DMA)));
    std::printf("heap.spiram.free=%u\n",
                static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)));
    std::printf("heap.spiram.largest=%u\n",
                static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)));
    std::printf("heap.integrity=%s\n", heap_caps_check_integrity_all(true) ? "pass" : "failed");
    std::printf("updates.total=%" PRIu64 "\n", g_update_count);
    std::printf("updates.full=%" PRIu64 "\n", g_full_update_count);
    std::printf("updates.partial=%" PRIu64 "\n", g_partial_update_count);
    std::printf("updates.average_ms=%" PRIu64 "\n", average_us / 1000);
    std::printf("updates.max_ms=%" PRIu64 "\n", g_max_update_us / 1000);
}

void PrintHelp()
{
    std::printf("usage:\n");
    std::printf("  epd help\n");
    std::printf("  epd status\n");
    std::printf("  epd scene white|black|gray|checker|text\n");
    std::printf("  epd waveform quality|text|fast|fastest black|white|gray\n");
    std::printf("  epd waveform-compare\n");
    std::printf("  epd boundary [0|1|2|3|all]\n");
    std::printf("  epd soak [iterations:1..10000]\n");
    std::printf("  epd text-soak [iterations:1..10000]\n");
    std::printf("  epd cycle-sleep [milliseconds:0..60000]\n");
    std::printf("  epd poweroff CONFIRM\n");
}

int CommandEpd(int argc, char** argv)
{
    if (argc < 2 || std::strcmp(argv[1], "help") == 0) {
        PrintHelp();
        return argc < 2 ? 1 : 0;
    }

    if (std::strcmp(argv[1], "status") == 0) {
        DisplayGuard guard;
        PrintStatus();
        return 0;
    }

    DisplayGuard guard;
    if (!guard.locked()) {
        std::printf("error: display mutex is unavailable\n");
        return 1;
    }
    if (!g_display_ready) {
        std::printf("error: display initialization failed; use 'epd status' for diagnostics\n");
        return 1;
    }

    bool ok = false;
    if (std::strcmp(argv[1], "scene") == 0) {
        if (argc < 3) {
            std::printf("usage: epd scene white|black|gray|checker|text\n");
            return 1;
        }
        ok = DrawScene(argv[2]);
    } else if (std::strcmp(argv[1], "waveform") == 0) {
        if (argc < 4) {
            std::printf("usage: epd waveform quality|text|fast|fastest black|white|gray\n");
            return 1;
        }
        ok = DrawWaveformScene(argv[2], argv[3]);
    } else if (std::strcmp(argv[1], "waveform-compare") == 0) {
        ok = DrawWaveformComparison();
    } else if (std::strcmp(argv[1], "boundary") == 0) {
        ok = RunBoundaries(argc >= 3 ? argv[2] : "all");
    } else if (std::strcmp(argv[1], "soak") == 0) {
        const uint32_t iterations = argc >= 3 ? static_cast<uint32_t>(std::strtoul(argv[2], nullptr, 10)) : 1000;
        ok = RunSoak(iterations);
    } else if (std::strcmp(argv[1], "text-soak") == 0) {
        const uint32_t iterations = argc >= 3 ? static_cast<uint32_t>(std::strtoul(argv[2], nullptr, 10)) : 1000;
        ok = RunTextSoak(iterations);
    } else if (std::strcmp(argv[1], "cycle-sleep") == 0) {
        const uint32_t milliseconds = argc >= 3 ? static_cast<uint32_t>(std::strtoul(argv[2], nullptr, 10)) : 2000;
        ok = CycleDisplaySleep(milliseconds);
    } else if (std::strcmp(argv[1], "poweroff") == 0) {
        if (argc < 3 || std::strcmp(argv[2], "CONFIRM") != 0) {
            std::printf("refusing power off; use: epd poweroff CONFIRM\n");
            return 1;
        }
        M5.Display.waitDisplay();
        M5.Display.sleep();
        std::printf("poweroff.requested=yes\n");
        std::fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(100));
        M5.Power.powerOff();
        return 0;
    } else {
        std::printf("error: unknown epd subcommand '%s'\n", argv[1]);
        PrintHelp();
        return 1;
    }

    std::printf("command.result=%s\n", ok ? "pass" : "failed");
    return ok ? 0 : 1;
}

void StartConsole()
{
    const esp_console_cmd_t command = {
        .command = "epd",
        .help = "PaperS3 EPD qualification commands",
        .hint = nullptr,
        .func = CommandEpd,
        .argtable = nullptr,
        .func_w_context = nullptr,
        .context = nullptr,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&command));

    esp_console_repl_t* repl = nullptr;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "epd-qual> ";
    repl_config.max_cmdline_length = 256;
    esp_console_dev_usb_serial_jtag_config_t hardware_config =
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hardware_config, &repl_config, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

}  // namespace

extern "C" void app_main(void)
{
    std::printf("\nPaperS3 EPD qualification boot\n");
    std::printf("cell=%s idf=%s m5gfx=%s m5unified=%s\n", PAPERS3_MATRIX_CELL, esp_get_idf_version(),
                PAPERS3_M5GFX_SHA, PAPERS3_M5UNIFIED_SHA);

    g_display_mutex = xSemaphoreCreateMutex();
    if (g_display_mutex == nullptr) {
        ESP_LOGE(kTag, "failed to create display mutex");
        return;
    }

    auto config = M5.config();
    // Avoid an implicit clear before the harness sets and records rotation.
    config.clear_display = false;
    M5.begin(config);
    g_display_ready = M5.getDisplayCount() > 0;

    if (g_display_ready) {
        M5.Display.setRotation(1);
        ConfigureTextDefaults();
        DrawTextScene();
    } else {
        ESP_LOGE(kTag, "display initialization failed; console remains available for status diagnostics");
    }

    PrintStatus();
    StartConsole();
    ESP_LOGI(kTag, "qualification console ready");
}
