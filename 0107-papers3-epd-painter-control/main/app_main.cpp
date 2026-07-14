#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <inttypes.h>
#include <cstring>

#include "EPD_Painter.h"
#include "esp_console.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "mbedtls/sha256.h"

namespace {

constexpr char kTag[] = "epd_control";
constexpr char kUpstreamCommit[] = "753c521da8aef59756df07c1a4eb88f1c64c8227";
constexpr char kExpectedIdf[] = "v5.4.2";
constexpr int kWidth = 960;
constexpr int kHeight = 540;
constexpr size_t kPackedBytes = static_cast<size_t>(kWidth) * kHeight / 4;
constexpr uint32_t kDriverTimeoutMs = 110000;
constexpr uint32_t kCommandTimeoutMs = 120000;
constexpr uint32_t kPowerDownTimeoutMs = 5000;

extern const uint8_t kReaderPageStart[] asm("_binary_reader_page_bin_start");
extern const uint8_t kReaderPageEnd[] asm("_binary_reader_page_bin_end");

EPD_Painter g_display(EPD_PAINTER_PRESET);
uint8_t* g_fixture = nullptr;
SemaphoreHandle_t g_command_mutex = nullptr;
std::atomic<uint32_t> g_next_operation{1};

enum class ControlState {
    kInitFailed,
    kBootLocked,
    kWhiteKnown,
    kTargetKnown,
    kFault,
};

enum class TargetKind {
    kNone,
    kWhite,
    kFullBlack,
    kArea1,
    kArea10,
    kArea25,
    kArea50,
    kArea100,
    kCheckerA,
    kCheckerB,
    kPage,
};

enum class OperationKind {
    kCleanup,
    kPaint,
    kWait,
};

struct OperationRequest {
    OperationKind kind = OperationKind::kWait;
    TargetKind target = TargetKind::kNone;
    uint32_t id = 0;
    TaskHandle_t waiter = nullptr;
    char command[80] = {};
    char target_hash[65] = {};
    bool result = false;
    int64_t elapsed_ms = 0;
};

ControlState g_state = ControlState::kInitFailed;
TargetKind g_last_target = TargetKind::kNone;
bool g_preset_valid = false;
OperationRequest g_request;

const char* StateName(ControlState state)
{
    switch (state) {
        case ControlState::kBootLocked:
            return "BOOT_LOCKED";
        case ControlState::kWhiteKnown:
            return "WHITE_KNOWN";
        case ControlState::kTargetKnown:
            return "TARGET_KNOWN";
        case ControlState::kFault:
            return "FAULT";
        case ControlState::kInitFailed:
        default:
            return "INIT_FAILED";
    }
}

const char* TargetName(TargetKind target)
{
    switch (target) {
        case TargetKind::kWhite:
            return "white";
        case TargetKind::kFullBlack:
            return "full-black";
        case TargetKind::kArea1:
            return "area-1";
        case TargetKind::kArea10:
            return "area-10";
        case TargetKind::kArea25:
            return "area-25";
        case TargetKind::kArea50:
            return "area-50";
        case TargetKind::kArea100:
            return "area-100";
        case TargetKind::kCheckerA:
            return "checker-a";
        case TargetKind::kCheckerB:
            return "checker-b";
        case TargetKind::kPage:
            return "reader-page";
        case TargetKind::kNone:
        default:
            return "none";
    }
}

const char* CommandedOrigin()
{
    if (g_state == ControlState::kBootLocked) {
        return "unknown-commanded";
    }
    return TargetName(g_last_target);
}

bool ValidatePreset()
{
    const auto& cfg = g_display.getConfig();
    constexpr std::array<int8_t, 8> expected_data = {6, 14, 7, 12, 9, 11, 8, 10};
    const bool controls_ok = cfg.width == kWidth && cfg.height == kHeight && cfg.pin_pwr == 46 &&
                             cfg.pin_spv == 17 && cfg.pin_ckv == 18 && cfg.pin_sph == 13 &&
                             cfg.pin_oe == 45 && cfg.pin_le == 15 && cfg.pin_cl == 16;
    bool data_ok = true;
    for (size_t i = 0; i < expected_data.size(); ++i) {
        data_ok = data_ok && cfg.data_pins[i] == expected_data[i];
    }
    return controls_ok && data_ok;
}

void HashFixture(char output[65])
{
    unsigned char digest[32] = {};
    mbedtls_sha256(g_fixture, kPackedBytes, digest, 0);
    for (size_t i = 0; i < sizeof(digest); ++i) {
        std::snprintf(output + i * 2, 3, "%02x", digest[i]);
    }
    output[64] = '\0';
}

void SetPackedPixel(int x, int y, uint8_t value)
{
    const size_t offset = static_cast<size_t>(y) * (kWidth / 4) + x / 4;
    const int shift = (3 - (x & 3)) * 2;
    const uint8_t mask = static_cast<uint8_t>(0x3U << shift);
    g_fixture[offset] = static_cast<uint8_t>((g_fixture[offset] & ~mask) | ((value & 0x3U) << shift));
}

void FillRectangle(int x, int y, int width, int height)
{
    for (int row = y; row < y + height; ++row) {
        for (int column = x; column < x + width; ++column) {
            SetPackedPixel(column, row, 3);
        }
    }
}

bool PrepareFixture(TargetKind target)
{
    if (!g_fixture) {
        return false;
    }
    std::memset(g_fixture, 0, kPackedBytes);
    switch (target) {
        case TargetKind::kWhite:
            return true;
        case TargetKind::kFullBlack:
        case TargetKind::kArea100:
            std::memset(g_fixture, 0xFF, kPackedBytes);
            return true;
        case TargetKind::kArea1:
            FillRectangle((kWidth - 96) / 2, (kHeight - 54) / 2, 96, 54);
            return true;
        case TargetKind::kArea10:
            FillRectangle((kWidth - 304) / 2, (kHeight - 172) / 2, 304, 172);
            return true;
        case TargetKind::kArea25:
            FillRectangle((kWidth - 480) / 2, (kHeight - 270) / 2, 480, 270);
            return true;
        case TargetKind::kArea50:
            FillRectangle((kWidth - 680) / 2, (kHeight - 382) / 2, 680, 382);
            return true;
        case TargetKind::kCheckerA:
        case TargetKind::kCheckerB:
            for (int y = 0; y < kHeight; ++y) {
                for (int x = 0; x < kWidth; ++x) {
                    const bool cell = ((x / 32) + (y / 32)) & 1;
                    const bool black = target == TargetKind::kCheckerA ? cell : !cell;
                    if (black) {
                        SetPackedPixel(x, y, 3);
                    }
                }
            }
            return true;
        case TargetKind::kPage:
            if (static_cast<size_t>(kReaderPageEnd - kReaderPageStart) != kPackedBytes) {
                return false;
            }
            std::memcpy(g_fixture, kReaderPageStart, kPackedBytes);
            return true;
        case TargetKind::kNone:
        default:
            return false;
    }
}

void PrintHelp()
{
    std::printf("commands:\n");
    std::printf("  epd help\n");
    std::printf("  epd status\n");
    std::printf("  epd heap\n");
    std::printf("  epd cleanup CONFIRM\n");
    std::printf("  epd target full white|black\n");
    std::printf("  epd target area 1|10|25|50|100\n");
    std::printf("  epd target checker a|b\n");
    std::printf("  epd target page\n");
    std::printf("  epd wait\n");
    std::printf("fixed policy: quality=HIGH stages=2 timeout_ms=%" PRIu32 "\n", kCommandTimeoutMs);
}

void PrintHeap()
{
    std::printf("EPD_HEAP free_8bit=%u min_8bit=%u free_internal=%u free_psram=%u\n",
                static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_8BIT)),
                static_cast<unsigned>(heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT)),
                static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
                static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)));
}

void PrintStatus()
{
    std::printf(
        "EPD_CONTROL_STATUS state=%s initialized=%s preset=%s pending=%d rails=%s "
        "last=%s idf=%s expected_idf=%s upstream=%s psram=%s heap_free=%u heap_min=%u\n",
        StateName(g_state), g_display.initialized() ? "yes" : "no", g_preset_valid ? "match" : "mismatch",
        g_display.pendingStages(), g_display.panelPowerActive() ? "active" : "idle", TargetName(g_last_target),
        esp_get_idf_version(), kExpectedIdf, kUpstreamCommit, esp_psram_is_initialized() ? "ready" : "missing",
        static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_8BIT)),
        static_cast<unsigned>(heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT)));
}

void OperationTask(void* argument)
{
    auto* request = static_cast<OperationRequest*>(argument);
    const int64_t started = esp_timer_get_time();
    bool idle = false;

    if (request->kind == OperationKind::kCleanup) {
        g_display.clear(nullptr, 0, EPD_Painter::ClearMode::HARD);
        idle = g_display.waitIdle(kDriverTimeoutMs);
    } else if (request->kind == OperationKind::kPaint) {
        g_display.paintPacked(g_fixture);
        idle = g_display.waitIdle(kDriverTimeoutMs);
    } else {
        idle = g_display.waitIdle(kDriverTimeoutMs);
    }

    const bool powered_down = idle && g_display.powerDown(kPowerDownTimeoutMs);
    request->result = idle && powered_down && g_display.pendingStages() == 0 && !g_display.panelPowerActive();
    request->elapsed_ms = (esp_timer_get_time() - started) / 1000;

    std::printf(
        "EPD_OP_END id=%" PRIu32 " result=%s elapsed_ms=%lld pending=%d rails=%s heap_free=%u heap_min=%u\n",
        request->id, request->result ? "ok" : "failed", static_cast<long long>(request->elapsed_ms),
        g_display.pendingStages(), g_display.panelPowerActive() ? "active" : "idle",
        static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_8BIT)),
        static_cast<unsigned>(heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT)));
    std::fflush(stdout);
    xTaskNotifyGive(request->waiter);
    vTaskDelete(nullptr);
}

bool RunOperation(OperationKind kind, TargetKind target, const char* command)
{
    if (!g_command_mutex || xSemaphoreTake(g_command_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        std::printf("error: operation mutex unavailable\n");
        return false;
    }

    if (g_display.pendingStages() != 0 || g_display.panelPowerActive()) {
        std::printf("error: pre-operation driver state is not idle\n");
        g_state = ControlState::kFault;
        xSemaphoreGive(g_command_mutex);
        return false;
    }

    g_request = {};
    g_request.kind = kind;
    g_request.target = target;
    g_request.id = g_next_operation.fetch_add(1);
    g_request.waiter = xTaskGetCurrentTaskHandle();
    std::snprintf(g_request.command, sizeof(g_request.command), "%s", command);
    if (kind == OperationKind::kWait) {
        std::snprintf(g_request.target_hash, sizeof(g_request.target_hash), "none");
    } else {
        HashFixture(g_request.target_hash);
    }

    std::printf(
        "EPD_OP_BEGIN id=%" PRIu32 " command=\"%s\" state=%s origin=%s target=%s quality=HIGH "
        "stages=2 target_sha256=%s heap_free=%u heap_min=%u\n",
        g_request.id, g_request.command, StateName(g_state), CommandedOrigin(), TargetName(target),
        g_request.target_hash, static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_8BIT)),
        static_cast<unsigned>(heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT)));
    std::fflush(stdout);

    TaskHandle_t worker = nullptr;
    if (xTaskCreatePinnedToCore(OperationTask, "epd_operation", 6144, &g_request, 8, &worker, 1) != pdPASS) {
        std::printf("EPD_OP_END id=%" PRIu32 " result=task-create-failed pending=%d rails=%s\n", g_request.id,
                    g_display.pendingStages(), g_display.panelPowerActive() ? "active" : "idle");
        g_state = ControlState::kFault;
        xSemaphoreGive(g_command_mutex);
        return false;
    }

    const bool notified = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(kCommandTimeoutMs)) == 1;
    if (!notified) {
        std::printf("EPD_OP_TIMEOUT id=%" PRIu32 " timeout_ms=%" PRIu32 " action=FAULT_NO_AUTOMATIC_CLEANUP\n",
                    g_request.id, kCommandTimeoutMs);
        g_state = ControlState::kFault;
        xSemaphoreGive(g_command_mutex);
        return false;
    }

    const bool result = g_request.result;
    if (!result) {
        g_state = ControlState::kFault;
    } else if (kind == OperationKind::kCleanup || target == TargetKind::kWhite) {
        g_state = ControlState::kWhiteKnown;
        g_last_target = TargetKind::kWhite;
    } else if (kind == OperationKind::kPaint) {
        g_state = ControlState::kTargetKnown;
        g_last_target = target;
    }
    xSemaphoreGive(g_command_mutex);
    return result;
}

bool StateAllowsOperations()
{
    return g_state != ControlState::kInitFailed && g_state != ControlState::kFault;
}

bool ParseArea(const char* value, TargetKind& target)
{
    if (std::strcmp(value, "1") == 0) target = TargetKind::kArea1;
    else if (std::strcmp(value, "10") == 0) target = TargetKind::kArea10;
    else if (std::strcmp(value, "25") == 0) target = TargetKind::kArea25;
    else if (std::strcmp(value, "50") == 0) target = TargetKind::kArea50;
    else if (std::strcmp(value, "100") == 0) target = TargetKind::kArea100;
    else return false;
    return true;
}

int CommandEpd(int argc, char** argv)
{
    if (argc < 2 || std::strcmp(argv[1], "help") == 0) {
        PrintHelp();
        return 0;
    }
    if (std::strcmp(argv[1], "status") == 0) {
        PrintStatus();
        return 0;
    }
    if (std::strcmp(argv[1], "heap") == 0) {
        PrintHeap();
        return 0;
    }
    if (!StateAllowsOperations()) {
        std::printf("error: panel operation refused in state=%s\n", StateName(g_state));
        return 1;
    }
    if (std::strcmp(argv[1], "cleanup") == 0) {
        if (argc != 3 || std::strcmp(argv[2], "CONFIRM") != 0) {
            std::printf("error: use exactly: epd cleanup CONFIRM\n");
            return 1;
        }
        PrepareFixture(TargetKind::kWhite);
        return RunOperation(OperationKind::kCleanup, TargetKind::kWhite, "cleanup CONFIRM") ? 0 : 1;
    }
    if (std::strcmp(argv[1], "wait") == 0) {
        return RunOperation(OperationKind::kWait, TargetKind::kNone, "wait") ? 0 : 1;
    }
    if (std::strcmp(argv[1], "target") != 0 || argc < 3) {
        std::printf("error: unknown or incomplete epd command\n");
        PrintHelp();
        return 1;
    }

    TargetKind target = TargetKind::kNone;
    char command[80] = {};
    if (std::strcmp(argv[2], "full") == 0 && argc == 4) {
        if (std::strcmp(argv[3], "white") == 0) {
            target = TargetKind::kWhite;
            if (g_state == ControlState::kBootLocked) {
                std::printf("error: first panel operation must be cleanup CONFIRM\n");
                return 1;
            }
        } else if (std::strcmp(argv[3], "black") == 0) {
            target = TargetKind::kFullBlack;
            if (!(g_state == ControlState::kWhiteKnown || g_last_target == TargetKind::kFullBlack)) {
                std::printf("error: full black requires white or full-black commanded origin\n");
                return 1;
            }
        } else {
            return 1;
        }
        std::snprintf(command, sizeof(command), "target full %s", argv[3]);
    } else if (std::strcmp(argv[2], "area") == 0 && argc == 4) {
        if (g_state != ControlState::kWhiteKnown || !ParseArea(argv[3], target)) {
            std::printf("error: area requires WHITE_KNOWN and fraction 1|10|25|50|100\n");
            return 1;
        }
        std::snprintf(command, sizeof(command), "target area %s", argv[3]);
    } else if (std::strcmp(argv[2], "checker") == 0 && argc == 4) {
        if (std::strcmp(argv[3], "a") == 0 && g_state == ControlState::kWhiteKnown) {
            target = TargetKind::kCheckerA;
        } else if (std::strcmp(argv[3], "b") == 0 && g_last_target == TargetKind::kCheckerA) {
            target = TargetKind::kCheckerB;
        } else {
            std::printf("error: checker a requires white; checker b requires checker a\n");
            return 1;
        }
        std::snprintf(command, sizeof(command), "target checker %s", argv[3]);
    } else if (std::strcmp(argv[2], "page") == 0 && argc == 3) {
        if (!(g_state == ControlState::kWhiteKnown || g_last_target == TargetKind::kPage)) {
            std::printf("error: page requires white or repeated page commanded origin\n");
            return 1;
        }
        target = TargetKind::kPage;
        std::snprintf(command, sizeof(command), "target page");
    } else {
        std::printf("error: unsupported target command\n");
        return 1;
    }

    if (!PrepareFixture(target)) {
        std::printf("error: fixture preparation failed for %s\n", TargetName(target));
        g_state = ControlState::kFault;
        return 1;
    }
    return RunOperation(OperationKind::kPaint, target, command) ? 0 : 1;
}

void StartConsole()
{
    const esp_console_cmd_t command = {
        .command = "epd",
        .help = "PaperS3 bounded independent EPD control",
        .hint = nullptr,
        .func = CommandEpd,
        .argtable = nullptr,
        .func_w_context = nullptr,
        .context = nullptr,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&command));

    esp_console_repl_t* repl = nullptr;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "epd-control> ";
    repl_config.max_cmdline_length = 256;
    const esp_console_dev_usb_serial_jtag_config_t hardware_config =
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hardware_config, &repl_config, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

}  // namespace

extern "C" void app_main(void)
{
    std::printf("\nPaperS3 independent EPD control boot\n");
    std::printf("control.version=p0.16 upstream=%s idf=%s expected_idf=%s\n", kUpstreamCommit,
                esp_get_idf_version(), kExpectedIdf);

    g_preset_valid = ValidatePreset();
    if (!g_preset_valid) {
        ESP_LOGE(kTag, "PaperS3 preset mismatch; driver initialization refused");
    } else if (!esp_psram_is_initialized()) {
        ESP_LOGE(kTag, "octal PSRAM unavailable; driver initialization refused");
    } else {
        g_display.setAutoShutdown(false);
        g_display.setInterlaceMode(false);
        g_display.setQuality(EPD_Painter::Quality::QUALITY_HIGH);
        if (!g_display.begin()) {
            ESP_LOGE(kTag, "EPD_Painter begin failed; panel commands remain unavailable");
        } else {
            g_fixture = static_cast<uint8_t*>(
                heap_caps_aligned_alloc(16, kPackedBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
            g_command_mutex = xSemaphoreCreateMutex();
            if (!g_fixture || !g_command_mutex) {
                ESP_LOGE(kTag, "fixture/mutex allocation failed; panel commands remain unavailable");
            } else {
                std::memset(g_fixture, 0, kPackedBytes);
                g_state = ControlState::kBootLocked;
            }
        }
    }

    PrintStatus();
    StartConsole();
    ESP_LOGI(kTag, "bounded console ready; no panel operation executed at boot");
}
