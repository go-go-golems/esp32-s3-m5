#pragma once

#include <string>

#include "esp_err.h"

struct RecorderStatus {
    bool recording = false;
    std::string filename;
    uint32_t bytes_written = 0;
    uint32_t sample_rate_hz = 0;
    uint8_t channels = 1;
    uint8_t bits_per_sample = 16;
};

esp_err_t recorder_init(void);

// Starts recording into a new WAV file. On success, returns the filename (basename only).
esp_err_t recorder_start(std::string &out_filename);

// Stops recording and finalizes the WAV header. On success, returns the last filename.
esp_err_t recorder_stop(std::string &out_filename);

RecorderStatus recorder_get_status(void);

// Returns JSON string for GET /api/v1/waveform (per ticket contract).
std::string recorder_get_waveform_json(void);

