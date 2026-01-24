#include "cardputer_kb/scanner.h"

#include "esp_log.h"

static const char *TAG = "cardputer_kb";

esp_err_t cardputer_kb::UnifiedScanner::init(const UnifiedScannerConfig &cfg) {
    if (inited_) {
        return ESP_OK;
    }

    switch (cfg.backend) {
    case ScannerBackend::Auto:
        // TODO: runtime probe TCA8418 first, fall back to GPIO matrix.
        backend_ = ScannerBackend::GpioMatrix;
        break;
    case ScannerBackend::GpioMatrix:
        backend_ = ScannerBackend::GpioMatrix;
        break;
    case ScannerBackend::Tca8418:
        ESP_LOGW(TAG, "TCA8418 backend not implemented yet");
        return ESP_ERR_NOT_SUPPORTED;
    default:
        return ESP_ERR_INVALID_ARG;
    }

    matrix_.init();
    inited_ = true;
    return ESP_OK;
}

cardputer_kb::ScanSnapshot cardputer_kb::UnifiedScanner::scan() {
    if (!inited_) {
        return ScanSnapshot{};
    }
    return matrix_.scan();
}

