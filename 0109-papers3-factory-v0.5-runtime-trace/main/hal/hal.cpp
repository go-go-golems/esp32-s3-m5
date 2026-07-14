/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "hal.h"
#include <memory>
#include <mooncake_log.h>
#include <M5Unified.hpp>
#include <driver/gpio.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <driver/spi_master.h>
#include <driver/sdspi_host.h>
#include <driver/sdmmc_host.h>
#include <sdmmc_cmd.h>
#include <esp_vfs_fat.h>
#include <esp_wifi.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_err.h>
#include <algorithm>
#include <vector>
#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static std::unique_ptr<Hal> _hal_instance;
static const std::string _tag = "HAL";

Hal& GetHAL()
{
    if (!_hal_instance) {
        mclog::tagInfo(_tag, "creating hal instance");
        _hal_instance = std::make_unique<Hal>();
    }
    return *_hal_instance.get();
}

void Hal::init()
{
    mclog::tagInfo(_tag, "init");

    M5.begin();
    M5.Display.setRotation(1);

    rtc_init();
    power_init();
    sd_card_init();
    wifi_init();
    buzzer_init();
    ext_port_init();
}

void Hal::feedTheDog()
{
    vTaskDelay(5);
}

/* -------------------------------------------------------------------------- */
/*                                     RTC                                    */
/* -------------------------------------------------------------------------- */
void Hal::rtc_init()
{
    mclog::tagInfo(_tag, "rtc init");

    m5::rtc_date_t date;
    date.year    = 2077;
    date.month   = 1;
    date.date    = 1;
    date.weekDay = 1;

    m5::rtc_time_t time;
    time.hours   = 12;
    time.minutes = 0;
    time.seconds = 0;

    M5.Rtc.setDateTime(&date, &time);
}

/* -------------------------------------------------------------------------- */
/*                                    Power                                   */
/* -------------------------------------------------------------------------- */
// https://github.com/espressif/esp-idf/blob/v5.3.3/examples/peripherals/adc/oneshot_read/main/oneshot_read_main.c
#define PIN_CHG_STATE   GPIO_NUM_4  // 0: charge, 1: full
#define PIN_USB_DET     GPIO_NUM_5  // USB DET 1: USB-IN
#define PIN_ADC_BATTERY GPIO_NUM_3

static adc_oneshot_unit_handle_t _adc1_handle = NULL;

void Hal::power_init()
{
    mclog::tagInfo(_tag, "power init");

    gpio_reset_pin(PIN_CHG_STATE);
    gpio_set_direction(PIN_CHG_STATE, GPIO_MODE_INPUT);

    gpio_reset_pin(PIN_USB_DET);
    gpio_set_direction(PIN_USB_DET, GPIO_MODE_INPUT);

    gpio_reset_pin(PIN_ADC_BATTERY);

    //-------------ADC1 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &_adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config;
    config.atten    = ADC_ATTEN_DB_12;
    config.bitwidth = ADC_BITWIDTH_DEFAULT;
    ESP_ERROR_CHECK(adc_oneshot_config_channel(_adc1_handle, ADC_CHANNEL_2, &config));
}

int Hal::getChgState()
{
    return gpio_get_level(PIN_CHG_STATE);
}

bool Hal::isUsbConnected()
{
    return gpio_get_level(PIN_USB_DET) == 1;
}

float Hal::getBatteryVoltage()
{
    float voltage = 0;

    int adc_raw = 0;
    if (adc_oneshot_read(_adc1_handle, ADC_CHANNEL_2, &adc_raw) == ESP_OK) {
        voltage = (float)adc_raw * 3.5 / 4096 * 2;
    } else {
        mclog::tagError(_tag, "failed to read battery voltage");
    }

    return voltage;
}

void Hal::powerOff()
{
    mclog::tagInfo(_tag, "power off");

    M5.Display.sleep();
    M5.Display.waitDisplay();
    delay(200);

    M5.Power.powerOff();
    mclog::tagInfo(_tag, "power off done");
}

void Hal::sleepAndWakeupTest()
{
    mclog::tagInfo(_tag, "sleep and wakeup test");

    M5.Rtc.clearIRQ();
    M5.Rtc.setAlarmIRQ(16);
    powerOff();
}

/* -------------------------------------------------------------------------- */
/*                                   SD Card                                  */
/* -------------------------------------------------------------------------- */
// https://github.com/espressif/esp-idf/blob/v5.3.3/examples/storage/sd_card/sdspi/main/sd_card_example_main.c
#define PIN_MISO GPIO_NUM_40
#define PIN_MOSI GPIO_NUM_38
#define PIN_SCLK GPIO_NUM_39
#define PIN_CS   GPIO_NUM_47

#define MOUNT_POINT "/sdcard"

static sdmmc_card_t* _sd_card    = nullptr;
static bool _spi_bus_initialized = false;

void Hal::sd_card_init()
{
    mclog::tagInfo(_tag, "sd card init");

    // If already mounted successfully, return
    if (_is_sd_card_mounted) {
        mclog::tagInfo(_tag, "sd card already mounted");
        return;
    }

    esp_err_t ret;

    // Options for mounting the filesystem
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false, .max_files = 5, .allocation_unit_size = 16 * 1024};

    const char mount_point[] = MOUNT_POINT;
    mclog::tagInfo(_tag, "initializing SD card");

    // Use SPI peripheral
    mclog::tagInfo(_tag, "using SPI peripheral");

    // Initialize SPI bus
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = PIN_MOSI,
        .miso_io_num     = PIN_MISO,
        .sclk_io_num     = PIN_SCLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 4000,
    };

    // Initialize SPI bus only if not already initialized
    if (!_spi_bus_initialized) {
        ret = spi_bus_initialize((spi_host_device_t)host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
        if (ret != ESP_OK) {
            mclog::tagError(_tag, "failed to initialize SPI bus");
            return;
        }
        _spi_bus_initialized = true;
        mclog::tagInfo(_tag, "spi bus initialized");
    } else {
        mclog::tagInfo(_tag, "spi bus already initialized, reusing");
    }

    // Initialize SD card slot
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs               = PIN_CS;
    slot_config.host_id               = (spi_host_device_t)host.slot;

    mclog::tagInfo(_tag, "mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &_sd_card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            mclog::tagError(_tag, "failed to mount filesystem");
        } else {
            mclog::tagError(_tag, "failed to initialize the card, make sure SD card lines have pull-up resistors");
        }

        // Don't clean up SPI bus on failure - leave it for retry
        mclog::tagInfo(_tag, "sd card init failed, but spi bus remains initialized for retry");
        return;
    }

    mclog::tagInfo(_tag, "filesystem mounted successfully");

    sdmmc_card_print_info(stdout, _sd_card);

    _is_sd_card_mounted = true;
}

void Hal::sdCardTest()
{
    if (!_is_sd_card_mounted) {
        sd_card_init();
        if (!_is_sd_card_mounted) {
            _sd_card_test_result.is_mounted = false;
            _sd_card_test_result.size       = "Not Found";
            return;
        }
    }

    _sd_card_test_result.is_mounted = true;

    // Try write to sd card
    FILE* fp = fopen(MOUNT_POINT "/test.txt", "w");
    if (fp) {
        fwrite("Hello, World!", 1, 13, fp);
        fclose(fp);

        _sd_card_test_result.size =
            fmt::format("Size: {:.1f} GB",
                        ((float)((uint64_t)_sd_card->csd.capacity) * _sd_card->csd.sector_size) / (1024 * 1024 * 1024));
    } else {
        _sd_card_test_result.size = "Write Failed";
    }

    _sd_card_test_result.type = "Type: ";
    if (_sd_card->is_sdio) {
        _sd_card_test_result.type += "SDIO";
    } else if (_sd_card->is_mmc) {
        _sd_card_test_result.type += "MMC";
    } else {
        _sd_card_test_result.type += (_sd_card->ocr & (1 << 30)) ? "SDHC/SDXC" : "SDSC";
    }

    _sd_card_test_result.name = fmt::format("Name: {}", std::string(_sd_card->cid.name));
}

/* -------------------------------------------------------------------------- */
/*                                    WiFi                                    */
/* -------------------------------------------------------------------------- */
// https://github.com/espressif/esp-idf/blob/v5.3.3/examples/wifi/scan/main/scan.c

#define DEFAULT_SCAN_LIST_SIZE 16
static wifi_ap_record_t _ap_info[DEFAULT_SCAN_LIST_SIZE];

void Hal::wifi_init()
{
    mclog::tagInfo(_tag, "wifi init");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void Hal::wifiScan()
{
    mclog::tagInfo(_tag, "wifi scan");

    // Clear previous scan results
    _wifi_scan_result.ap_list.clear();
    _wifi_scan_result.bestSsid.clear();
    _wifi_scan_result.bestRssi = -100;  // Initialize to a very weak signal

    uint16_t number   = DEFAULT_SCAN_LIST_SIZE;
    uint16_t ap_count = 0;
    memset(_ap_info, 0, sizeof(_ap_info));

    // Start WiFi scan
    esp_err_t ret = esp_wifi_scan_start(NULL, true);
    if (ret != ESP_OK) {
        mclog::tagError(_tag, "failed to start wifi scan: {}", esp_err_to_name(ret));
        return;
    }

    ret = esp_wifi_scan_get_ap_num(&ap_count);
    if (ret != ESP_OK) {
        mclog::tagError(_tag, "failed to get AP number: {}", esp_err_to_name(ret));
        return;
    }

    ret = esp_wifi_scan_get_ap_records(&number, _ap_info);
    if (ret != ESP_OK) {
        mclog::tagError(_tag, "failed to get AP records: {}", esp_err_to_name(ret));
        return;
    }

    // Process scan results
    for (int i = 0; i < number; i++) {
        std::string ssid = (char*)_ap_info[i].ssid;
        int rssi         = _ap_info[i].rssi;

        // Skip empty SSID
        if (ssid.empty()) {
            continue;
        }

        // Add to ap_list
        _wifi_scan_result.ap_list.push_back(std::make_pair(rssi, ssid));

        // Find the best (strongest) signal
        if (rssi > _wifi_scan_result.bestRssi) {
            _wifi_scan_result.bestRssi = rssi;
            _wifi_scan_result.bestSsid = ssid;
        }
    }

    // Sort ap_list by RSSI (from strongest to weakest)
    std::sort(_wifi_scan_result.ap_list.begin(), _wifi_scan_result.ap_list.end(),
              [](const std::pair<int, std::string>& a, const std::pair<int, std::string>& b) {
                  return a.first > b.first;  // Higher RSSI first
              });

    mclog::tagInfo(_tag, "wifi scan completed, found {} APs", _wifi_scan_result.ap_list.size());
    if (!_wifi_scan_result.bestSsid.empty()) {
        mclog::tagInfo(_tag, "best AP: {} (RSSI: {})", _wifi_scan_result.bestSsid, _wifi_scan_result.bestRssi);
    }
}

/* -------------------------------------------------------------------------- */
/*                                   Buzzer                                   */
/* -------------------------------------------------------------------------- */
// https://github.com/espressif/esp-idf/blob/v5.3.3/examples/peripherals/ledc/ledc_basic/main/ledc_basic_example_main.c

#define PIN_BUZZER GPIO_NUM_21

#define LEDC_TIMER_BUZZER    LEDC_TIMER_0
#define LEDC_MODE_BUZZER     LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL_BUZZER  LEDC_CHANNEL_0
#define LEDC_DUTY_RES_BUZZER LEDC_TIMER_13_BIT  // Set duty resolution to 13 bits
#define LEDC_DUTY_BUZZER     (4096)             // Set duty to 50%. (2 ** 13) * 50% = 4096

static bool _buzzer_initialized = false;

void Hal::buzzer_init()
{
    mclog::tagInfo(_tag, "buzzer init");

    if (_buzzer_initialized) {
        mclog::tagInfo(_tag, "buzzer already initialized");
        return;
    }

    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode          = LEDC_MODE_BUZZER;
    ledc_timer.duty_resolution     = LEDC_DUTY_RES_BUZZER;
    ledc_timer.timer_num           = LEDC_TIMER_BUZZER;
    ledc_timer.freq_hz             = 1000;  // Default frequency, will be changed in tone()
    ledc_timer.clk_cfg             = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {};
    ledc_channel.gpio_num              = PIN_BUZZER;
    ledc_channel.speed_mode            = LEDC_MODE_BUZZER;
    ledc_channel.channel               = LEDC_CHANNEL_BUZZER;
    ledc_channel.timer_sel             = LEDC_TIMER_BUZZER;
    ledc_channel.intr_type             = LEDC_INTR_DISABLE;
    ledc_channel.duty                  = 0;  // Set duty to 0% initially (silent)
    ledc_channel.hpoint                = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    _buzzer_initialized = true;
    mclog::tagInfo(_tag, "buzzer initialized successfully");
}

void Hal::tone(int frequency, int duration)
{
    // mclog::tagInfo(_tag, "tone: {} Hz, {} ms", frequency, duration);

    if (!_buzzer_initialized) {
        mclog::tagError(_tag, "buzzer not initialized");
        return;
    }

    if (frequency <= 0) {
        mclog::tagError(_tag, "invalid frequency: {}", frequency);
        return;
    }

    // Update the timer frequency
    ESP_ERROR_CHECK(ledc_set_freq(LEDC_MODE_BUZZER, LEDC_TIMER_BUZZER, frequency));

    // Set duty to 50% to generate the tone
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE_BUZZER, LEDC_CHANNEL_BUZZER, LEDC_DUTY_BUZZER));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE_BUZZER, LEDC_CHANNEL_BUZZER));

    // If duration is specified, stop the tone after the duration
    if (duration > 0) {
        delay(duration);
        noTone();
    }
}

void Hal::noTone()
{
    // mclog::tagInfo(_tag, "no tone");

    if (!_buzzer_initialized) {
        mclog::tagError(_tag, "buzzer not initialized");
        return;
    }

    // Set duty to 0% to stop the tone
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE_BUZZER, LEDC_CHANNEL_BUZZER, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE_BUZZER, LEDC_CHANNEL_BUZZER));
}

/* -------------------------------------------------------------------------- */
/*                                     EXT                                    */
/* -------------------------------------------------------------------------- */
static std::vector<gpio_num_t> _ext_port_test_pins = {GPIO_NUM_0, GPIO_NUM_1, GPIO_NUM_2};

void Hal::ext_port_init()
{
    mclog::tagInfo(_tag, "ext port init");

    for (auto pin : _ext_port_test_pins) {
        gpio_reset_pin(pin);
        gpio_set_direction(pin, GPIO_MODE_INPUT_OUTPUT);
        gpio_set_level(pin, 0);
    }

    // Keep ext port led blinking
    xTaskCreate(
        [](void* arg) {
            // Beeper
            for (int i = 0; i < 5; i++) {
                GetHAL().tone(4000, 100);
                GetHAL().delay(100);
            }

            bool level = false;
            while (1) {
                for (auto pin : _ext_port_test_pins) {
                    gpio_set_level(pin, level);
                    GetHAL().delay(500);
                }
                level = !level;
            }
        },
        "ext", 1024 * 4, NULL, 5, NULL);
}

/* -------------------------------------------------------------------------- */
/*                                    Touch                                   */
/* -------------------------------------------------------------------------- */
bool Hal::wasTouchClickedArea(int x, int y, int w, int h)
{
    if (GetHAL().isTouchPressed()) {
        auto& t = getTouchDetail();
        if (t.wasClicked()) {
            if (t.x >= x && t.x <= x + w && t.y >= y && t.y <= y + h) {
                return true;
            }
        }
    }
    return false;
}
