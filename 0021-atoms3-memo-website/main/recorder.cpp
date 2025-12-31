/*
 * Ticket CLINTS-MEMO-WEBSITE: I2S -> WAV recorder.
 *
 * MVP goals:
 * - start/stop recording to SPIFFS
 * - maintain lightweight waveform ring for /api/v1/waveform
 */

#include "recorder.h"

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <vector>

#include <dirent.h>
#include <sys/stat.h>

#include <cerrno>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"

#include "driver/i2c.h"
#include "driver/i2s.h"

#include "sdkconfig.h"

#if CONFIG_CLINTS_MEMO_CODEC_ES8311_ENABLE
#include "es8311.h"
#endif

#include "storage_spiffs.h"

static const char *TAG = "atoms3_memo_recorder";

static constexpr uint16_t kWavAudioFormatPcm = 1;
static constexpr uint8_t kWavChannels = 1;
static constexpr uint8_t kWavBitsPerSample = 16;

struct WavHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t riff_size = 0;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmt_size = 16;
    uint16_t audio_format = kWavAudioFormatPcm;
    uint16_t num_channels = kWavChannels;
    uint32_t sample_rate = 0;
    uint32_t byte_rate = 0;
    uint16_t block_align = 0;
    uint16_t bits_per_sample = kWavBitsPerSample;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t data_size = 0;
};

static_assert(sizeof(WavHeader) == 44, "WAV header size must be 44 bytes");

static inline i2s_port_t i2s_port_from_config() {
#if CONFIG_CLINTS_MEMO_I2S_PORT_0
    return I2S_NUM_0;
#else
    return I2S_NUM_1;
#endif
}

class Recorder {
public:
    esp_err_t init() {
        if (initialized_) return ESP_OK;

        ESP_ERROR_CHECK(storage_spiffs_init());

#if CONFIG_CLINTS_MEMO_CODEC_ES8311_ENABLE
        ESP_ERROR_CHECK(init_es8311_());
#endif
        ESP_ERROR_CHECK(init_i2s_());

        const int window_samples = std::max(1, (CONFIG_CLINTS_MEMO_AUDIO_SAMPLE_RATE_HZ * CONFIG_CLINTS_MEMO_WAVEFORM_WINDOW_MS) / 1000);
        waveform_ring_.assign((size_t)window_samples, 0);
        waveform_write_idx_ = 0;
        waveform_filled_ = 0;

        initialized_ = true;
        return ESP_OK;
    }

    esp_err_t start(std::string &out_filename) {
        lock_();
        if (state_ != State::Idle) {
            unlock_();
            return ESP_ERR_INVALID_STATE;
        }
        state_ = State::Recording;
        unlock_();

        if (!i2s_installed_) {
            ESP_LOGE(TAG, "I2S not installed");
            set_error_state_();
            return ESP_FAIL;
        }

        std::string filename;
        esp_err_t err = create_new_recording_file_(filename);
        if (err != ESP_OK) {
            set_error_state_();
            return err;
        }

        lock_();
        current_filename_ = filename;
        bytes_written_ = 0;
        stop_requested_ = false;
        unlock_();

        if (!done_sem_) {
            done_sem_ = xSemaphoreCreateBinary();
        }
        if (!done_sem_) {
            set_error_state_();
            return ESP_ERR_NO_MEM;
        }

        BaseType_t ok = xTaskCreatePinnedToCore(
            &Recorder::capture_task_entry_,
            "memo_capture",
            8192,
            this,
            5,
            &capture_task_,
            1);
        if (ok != pdPASS) {
            ESP_LOGE(TAG, "failed to start capture task");
            set_error_state_();
            return ESP_FAIL;
        }

        out_filename = filename;
        return ESP_OK;
    }

    esp_err_t stop(std::string &out_filename) {
        lock_();
        if (state_ != State::Recording) {
            unlock_();
            return ESP_ERR_INVALID_STATE;
        }
        state_ = State::Stopping;
        stop_requested_ = true;
        out_filename = current_filename_;
        unlock_();

        if (!capture_task_) {
            set_error_state_();
            return ESP_FAIL;
        }

        // Wait for the capture task to finish and signal completion.
        if (xSemaphoreTake(done_sem_, pdMS_TO_TICKS(5000)) != pdTRUE) {
            ESP_LOGE(TAG, "timeout waiting for capture task to stop");
            set_error_state_();
            return ESP_ERR_TIMEOUT;
        }

        lock_();
        state_ = State::Idle;
        capture_task_ = nullptr;
        unlock_();

        return ESP_OK;
    }

    RecorderStatus status() {
        RecorderStatus st;
        lock_();
        st.recording = (state_ == State::Recording);
        st.filename = current_filename_;
        st.bytes_written = bytes_written_;
        st.sample_rate_hz = CONFIG_CLINTS_MEMO_AUDIO_SAMPLE_RATE_HZ;
        st.channels = kWavChannels;
        st.bits_per_sample = kWavBitsPerSample;
        unlock_();
        return st;
    }

    std::string waveform_json() {
        const int n = CONFIG_CLINTS_MEMO_WAVEFORM_N_POINTS;
        std::vector<int16_t> ring_copy;
        size_t write_idx = 0;
        size_t filled = 0;

        lock_();
        ring_copy = waveform_ring_;
        write_idx = waveform_write_idx_;
        filled = waveform_filled_;
        unlock_();

        std::vector<int16_t> recent(ring_copy.size(), 0);
        if (filled == 0) {
            // all zeros
        } else if (filled < ring_copy.size()) {
            // Not full yet: copy the filled prefix to the end so "latest" is right-aligned.
            const size_t start = recent.size() - filled;
            std::copy_n(ring_copy.begin(), filled, recent.begin() + (ptrdiff_t)start);
        } else {
            // Full ring: chronological order starting at write_idx (oldest).
            for (size_t i = 0; i < recent.size(); i++) {
                recent[i] = ring_copy[(write_idx + i) % ring_copy.size()];
            }
        }

        std::vector<int16_t> points((size_t)n, 0);
        if (!recent.empty()) {
            for (int i = 0; i < n; i++) {
                const size_t start = (size_t)((uint64_t)i * (uint64_t)recent.size() / (uint64_t)n);
                const size_t end = (size_t)((uint64_t)(i + 1) * (uint64_t)recent.size() / (uint64_t)n);
                int16_t best = 0;
                int best_abs = -1;
                for (size_t j = start; j < std::max(end, start + 1); j++) {
                    const int16_t v = recent[std::min(j, recent.size() - 1)];
                    const int a = std::abs((int)v);
                    if (a > best_abs) {
                        best_abs = a;
                        best = v;
                    }
                }
                points[(size_t)i] = best;
            }
        }

        std::string json;
        json.reserve(2048);
        json += "{";
        json += "\"ok\":true,";
        json += "\"sample_rate_hz\":";
        json += std::to_string(CONFIG_CLINTS_MEMO_AUDIO_SAMPLE_RATE_HZ);
        json += ",";
        json += "\"window_ms\":";
        json += std::to_string(CONFIG_CLINTS_MEMO_WAVEFORM_WINDOW_MS);
        json += ",";
        json += "\"n\":";
        json += std::to_string(n);
        json += ",";
        json += "\"points_i16\":[";
        for (int i = 0; i < n; i++) {
            if (i) json += ",";
            json += std::to_string(points[(size_t)i]);
        }
        json += "]";
        json += "}";
        return json;
    }

private:
    enum class State { Idle, Recording, Stopping, Error };

    void lock_() { portENTER_CRITICAL(&mux_); }
    void unlock_() { portEXIT_CRITICAL(&mux_); }

    void set_error_state_() {
        lock_();
        state_ = State::Error;
        unlock_();
    }

    esp_err_t init_i2s_() {
        if (i2s_installed_) return ESP_OK;

        const i2s_port_t port = i2s_port_from_config();
        const int bclk = CONFIG_CLINTS_MEMO_I2S_BCLK_GPIO;
        const int ws = CONFIG_CLINTS_MEMO_I2S_WS_GPIO;
        const int dout = CONFIG_CLINTS_MEMO_I2S_DOUT_GPIO;
        const int din = CONFIG_CLINTS_MEMO_I2S_DIN_GPIO;
        const int mclk = CONFIG_CLINTS_MEMO_I2S_MCLK_GPIO;
        const int sample_rate = CONFIG_CLINTS_MEMO_AUDIO_SAMPLE_RATE_HZ;

        if (bclk < 0 || ws < 0 || din < 0) {
            ESP_LOGE(TAG, "invalid I2S pin config: bclk=%d ws=%d din=%d", bclk, ws, din);
            return ESP_ERR_INVALID_ARG;
        }

        i2s_mode_t mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
        if (dout >= 0) mode = (i2s_mode_t)(mode | I2S_MODE_TX);

        i2s_config_t cfg = {};
        cfg.mode = mode;
        cfg.sample_rate = sample_rate;
        cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
#if CONFIG_CLINTS_MEMO_I2S_CHANNEL_FMT_ALL_LEFT
        cfg.channel_format = I2S_CHANNEL_FMT_ALL_LEFT;
#else
        cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
#endif
        cfg.communication_format = I2S_COMM_FORMAT_I2S;
        cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
        cfg.dma_buf_count = 8;
        cfg.dma_buf_len = CONFIG_CLINTS_MEMO_AUDIO_FRAME_SAMPLES;
        cfg.use_apll = 1;
        cfg.tx_desc_auto_clear = true;

        esp_err_t err = i2s_driver_install(port, &cfg, 0, nullptr);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "i2s_driver_install failed: %s", esp_err_to_name(err));
            return err;
        }

        i2s_pin_config_t pins = {};
        pins.mck_io_num = mclk;
        pins.bck_io_num = bclk;
        pins.ws_io_num = ws;
        pins.data_out_num = (dout >= 0) ? dout : I2S_PIN_NO_CHANGE;
        pins.data_in_num = din;

        err = i2s_set_pin(port, &pins);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "i2s_set_pin failed: %s", esp_err_to_name(err));
            return err;
        }
        (void)i2s_zero_dma_buffer(port);

        ESP_LOGI(TAG, "I2S ready: port=%d sr=%d frame_samples=%d", (int)port, sample_rate, CONFIG_CLINTS_MEMO_AUDIO_FRAME_SAMPLES);
        i2s_installed_ = true;
        return ESP_OK;
    }

#if CONFIG_CLINTS_MEMO_CODEC_ES8311_ENABLE
    esp_err_t init_es8311_() {
        if (es8311_initialized_) return ESP_OK;

        const int sample_rate = CONFIG_CLINTS_MEMO_AUDIO_SAMPLE_RATE_HZ;
        const i2c_port_t port = (CONFIG_CLINTS_MEMO_CODEC_I2C_PORT == 0) ? I2C_NUM_0 : I2C_NUM_1;

        i2c_config_t conf = {};
        conf.mode = I2C_MODE_MASTER;
        conf.sda_io_num = CONFIG_CLINTS_MEMO_CODEC_I2C_SDA_GPIO;
        conf.scl_io_num = CONFIG_CLINTS_MEMO_CODEC_I2C_SCL_GPIO;
        conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
        conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
        conf.master.clk_speed = CONFIG_CLINTS_MEMO_CODEC_I2C_FREQ_HZ;

        ESP_ERROR_CHECK(i2c_param_config(port, &conf));
        ESP_ERROR_CHECK(i2c_driver_install(port, conf.mode, 0, 0, 0));

        es8311_handle_ = es8311_create(port, ES8311_ADDRRES_0);
        if (!es8311_handle_) {
            ESP_LOGE(TAG, "es8311_create failed");
            return ESP_FAIL;
        }

        es8311_clock_config_t clk_cfg = {};
        clk_cfg.mclk_inverted = false;
        clk_cfg.sclk_inverted = false;
        clk_cfg.mclk_from_mclk_pin = false;
        clk_cfg.mclk_frequency = 0;
        clk_cfg.sample_frequency = sample_rate;

        ESP_ERROR_CHECK(es8311_init(es8311_handle_, &clk_cfg, ES8311_RESOLUTION_32, ES8311_RESOLUTION_32));
        (void)es8311_voice_volume_set(es8311_handle_, 80, nullptr);
        ESP_ERROR_CHECK(es8311_microphone_config(es8311_handle_, false));

        ESP_LOGI(TAG, "ES8311 initialized over I2C port=%d sr=%d", (int)port, sample_rate);
        es8311_initialized_ = true;
        return ESP_OK;
    }
#endif

    esp_err_t create_new_recording_file_(std::string &out_basename) {
        const char *dir = storage_recordings_dir();
        struct stat st = {};

        for (int attempt = 0; attempt < 1000; attempt++) {
            const uint32_t n = ++file_counter_;
            char name[64];
            snprintf(name, sizeof(name), "rec_%05" PRIu32 ".wav", n);
            const std::string basename(name);

            const std::string path = std::string(dir) + "/" + name;
            if (stat(path.c_str(), &st) == 0) {
                continue; // exists
            }

            FILE *fp = fopen(path.c_str(), "wb");
            if (!fp) {
                // SPIFFS root may not report as a directory via stat(), but file creation can still work.
                // Log errno to help diagnose mount/path issues.
                ESP_LOGE(TAG, "failed to open for write: %s errno=%d", path.c_str(), errno);
                return ESP_FAIL;
            }

            WavHeader hdr = {};
            hdr.sample_rate = CONFIG_CLINTS_MEMO_AUDIO_SAMPLE_RATE_HZ;
            hdr.byte_rate = hdr.sample_rate * hdr.num_channels * (hdr.bits_per_sample / 8);
            hdr.block_align = hdr.num_channels * (hdr.bits_per_sample / 8);
            hdr.data_size = 0;
            hdr.riff_size = 36 + hdr.data_size;

            const size_t wrote = fwrite(&hdr, 1, sizeof(hdr), fp);
            if (wrote != sizeof(hdr)) {
                fclose(fp);
                unlink(path.c_str());
                ESP_LOGE(TAG, "failed writing WAV header: %s", path.c_str());
                return ESP_FAIL;
            }
            fflush(fp);

            lock_();
            file_path_ = path;
            file_ = fp;
            unlock_();

            out_basename = basename;
            return ESP_OK;
        }

        return ESP_ERR_NO_MEM;
    }

    void finalize_and_close_file_(uint32_t data_bytes) {
        FILE *fp = nullptr;
        std::string path;

        lock_();
        fp = file_;
        path = file_path_;
        file_ = nullptr;
        file_path_.clear();
        unlock_();

        if (!fp) return;

        WavHeader hdr = {};
        hdr.sample_rate = CONFIG_CLINTS_MEMO_AUDIO_SAMPLE_RATE_HZ;
        hdr.byte_rate = hdr.sample_rate * hdr.num_channels * (hdr.bits_per_sample / 8);
        hdr.block_align = hdr.num_channels * (hdr.bits_per_sample / 8);
        hdr.data_size = data_bytes;
        hdr.riff_size = 36 + hdr.data_size;

        (void)fseek(fp, 0, SEEK_SET);
        (void)fwrite(&hdr, 1, sizeof(hdr), fp);
        fflush(fp);
        fclose(fp);

        ESP_LOGI(TAG, "finalized wav: %s bytes=%" PRIu32, path.c_str(), data_bytes);
    }

    void capture_loop_() {
        const i2s_port_t port = i2s_port_from_config();
        const size_t frame_bytes = (size_t)CONFIG_CLINTS_MEMO_AUDIO_FRAME_SAMPLES * sizeof(int16_t);
        std::vector<int16_t> buf((size_t)CONFIG_CLINTS_MEMO_AUDIO_FRAME_SAMPLES);

        while (true) {
            bool stop = false;
            lock_();
            stop = stop_requested_;
            unlock_();
            if (stop) break;

            size_t bytes_read = 0;
            const esp_err_t err = i2s_read(port, buf.data(), frame_bytes, &bytes_read, portMAX_DELAY);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "i2s_read failed: %s", esp_err_to_name(err));
                break;
            }
            if (bytes_read == 0) continue;

            // Feed waveform ring.
            const size_t samples = bytes_read / sizeof(int16_t);
            lock_();
            for (size_t i = 0; i < samples; i++) {
                waveform_ring_[waveform_write_idx_] = buf[i];
                waveform_write_idx_ = (waveform_write_idx_ + 1) % waveform_ring_.size();
                waveform_filled_ = std::min(waveform_filled_ + 1, waveform_ring_.size());
            }
            unlock_();

            // Append to file.
            FILE *fp = nullptr;
            lock_();
            fp = file_;
            unlock_();
            if (!fp) {
                ESP_LOGE(TAG, "file closed unexpectedly");
                break;
            }

            const size_t wrote = fwrite(buf.data(), 1, bytes_read, fp);
            if (wrote != bytes_read) {
                ESP_LOGE(TAG, "fwrite failed wrote=%u expected=%u", (unsigned)wrote, (unsigned)bytes_read);
                break;
            }

            lock_();
            bytes_written_ += (uint32_t)wrote;
            unlock_();
        }

        uint32_t bytes = 0;
        lock_();
        bytes = bytes_written_;
        unlock_();
        finalize_and_close_file_(bytes);
    }

    static void capture_task_entry_(void *arg) {
        auto *self = static_cast<Recorder *>(arg);
        self->capture_loop_();
        xSemaphoreGive(self->done_sem_);
        vTaskDelete(nullptr);
    }

    bool initialized_ = false;

    portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
    State state_ = State::Idle;

    bool i2s_installed_ = false;
    TaskHandle_t capture_task_ = nullptr;
    SemaphoreHandle_t done_sem_ = nullptr;

    bool stop_requested_ = false;
    uint32_t bytes_written_ = 0;
    uint32_t file_counter_ = 0;
    std::string current_filename_;

    FILE *file_ = nullptr;
    std::string file_path_;

    std::vector<int16_t> waveform_ring_;
    size_t waveform_write_idx_ = 0;
    size_t waveform_filled_ = 0;

#if CONFIG_CLINTS_MEMO_CODEC_ES8311_ENABLE
    bool es8311_initialized_ = false;
    es8311_handle_t es8311_handle_ = nullptr;
#endif
};

static Recorder &recorder_singleton() {
    static Recorder r;
    return r;
}

esp_err_t recorder_init(void) {
    return recorder_singleton().init();
}

esp_err_t recorder_start(std::string &out_filename) {
    return recorder_singleton().start(out_filename);
}

esp_err_t recorder_stop(std::string &out_filename) {
    return recorder_singleton().stop(out_filename);
}

RecorderStatus recorder_get_status(void) {
    return recorder_singleton().status();
}

std::string recorder_get_waveform_json(void) {
    return recorder_singleton().waveform_json();
}
