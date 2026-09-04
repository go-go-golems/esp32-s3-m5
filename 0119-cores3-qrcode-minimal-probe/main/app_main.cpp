// SPDX-License-Identifier: MIT
// Minimal CoreS3 + Module13.2 QRCode electrical/UART probe.
// No queues, worker tasks, console, scanner configuration, or protocol wrapper.

#include <M5Unified.h>
#include <utility/PI4IOE5V6408_Class.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <memory>

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
constexpr uart_port_t kUart = UART_NUM_1;
constexpr int kTx = 13;  // CoreS3 G13 -> QR_RX (M5-Bus pin 23)
constexpr int kRx = 14;  // CoreS3 G14 <- QR_TX (M5-Bus pin 26)
constexpr uint8_t kPowerEn = 0;
constexpr uint8_t kTrigger = 4;
constexpr uint8_t kFirmwareQuery[] = {0x43, 0x02, 0xC1};
constexpr char kTag[] = "qr_minimal";

std::unique_ptr<m5::PI4IOE5V6408_Class> g_io;
uint8_t g_frame[256];
size_t g_frame_len;
int64_t g_last_rx_us;

void draw_header(const char *state) {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(8, 8);
    M5.Display.println("QR MINIMAL PROBE");
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setTextSize(1);
    M5.Display.printf("UART1 115200  TX=G13  RX=G14\n");
    M5.Display.printf("PWR=exp0  TRIG=exp4  H2 removed\n");
    M5.Display.printf("%s\n", state);
    M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5.Display.println("Tap screen to pulse hardware TRIG");
}

void show_bytes(const char *label, const uint8_t *data, size_t len) {
    ESP_LOGI(kTag, "%s: %u bytes", label, static_cast<unsigned>(len));
    ESP_LOG_BUFFER_HEXDUMP(kTag, data, len, ESP_LOG_INFO);

    M5.Display.fillRect(0, 76, M5.Display.width(), M5.Display.height() - 76,
                        TFT_BLACK);
    M5.Display.setCursor(8, 80);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Display.printf("%s: %u bytes\n\nHEX:\n", label,
                      static_cast<unsigned>(len));
    const size_t shown = std::min<size_t>(len, 96);
    for (size_t i = 0; i < shown; ++i) {
        M5.Display.printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) M5.Display.println();
    }
    M5.Display.println("\nASCII:");
    for (size_t i = 0; i < shown; ++i) {
        M5.Display.print(std::isprint(static_cast<unsigned char>(data[i]))
                             ? static_cast<char>(data[i])
                             : '.');
    }
    M5.Display.println();
}

bool init_expander() {
    g_io.reset(new m5::PI4IOE5V6408_Class(0x43, 100000, &M5.In_I2C));
    if (!g_io || !g_io->begin()) return false;

    // Preload a safe latch state while the outputs are still high impedance:
    // engine off, hardware trigger idle high.
    g_io->digitalWrite(kPowerEn, false);
    g_io->digitalWrite(kTrigger, true);
    for (uint8_t channel : {kPowerEn, kTrigger}) {
        g_io->setDirection(channel, true);
        g_io->setHighImpedance(channel, false);
    }
    g_io->digitalWrite(kTrigger, true);
    g_io->digitalWrite(kPowerEn, true);
    return true;
}

bool init_uart() {
    const uart_config_t config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
        .flags = {},
    };
    return uart_param_config(kUart, &config) == ESP_OK &&
           uart_set_pin(kUart, kTx, kRx, UART_PIN_NO_CHANGE,
                        UART_PIN_NO_CHANGE) == ESP_OK &&
           uart_driver_install(kUart, 1024, 0, 0, nullptr, 0) == ESP_OK;
}

void pulse_trigger() {
    ESP_LOGI(kTag, "TRIG pulse: low 100 ms, then high");
    g_io->digitalWrite(kTrigger, false);
    vTaskDelay(pdMS_TO_TICKS(100));
    g_io->digitalWrite(kTrigger, true);
}

void collect_uart() {
    uint8_t bytes[64];
    const int count = uart_read_bytes(kUart, bytes, sizeof(bytes), 0);
    if (count <= 0) return;

    ESP_LOGI(kTag, "RX chunk: %d bytes", count);
    ESP_LOG_BUFFER_HEXDUMP(kTag, bytes, count, ESP_LOG_INFO);
    const size_t room = sizeof(g_frame) - g_frame_len;
    const size_t copy = std::min<size_t>(static_cast<size_t>(count), room);
    std::copy_n(bytes, copy, g_frame + g_frame_len);
    g_frame_len += copy;
    g_last_rx_us = esp_timer_get_time();
}

void emit_quiet_frame() {
    if (!g_frame_len || esp_timer_get_time() - g_last_rx_us < 50000) return;
    show_bytes("UART RX", g_frame, g_frame_len);
    g_frame_len = 0;
}
}  // namespace

extern "C" void app_main(void) {
    M5.begin();
    M5.Display.init();
    M5.Display.setRotation(1);
    M5.Display.setBrightness(80);
    draw_header("starting");

    if (!init_expander()) {
        ESP_LOGE(kTag, "PI4IOE5V6408 @0x43 not found");
        draw_header("ERROR: expander @0x43 not found");
        while (true) vTaskDelay(portMAX_DELAY);
    }
    ESP_LOGI(kTag, "expander ready; PWR latch=1 TRIG latch=1");
    draw_header("power enabled; waiting 1000 ms");
    vTaskDelay(pdMS_TO_TICKS(1000));

    if (!init_uart()) {
        ESP_LOGE(kTag, "UART1 initialization failed");
        draw_header("ERROR: UART1 initialization failed");
        while (true) vTaskDelay(portMAX_DELAY);
    }

    uart_flush_input(kUart);
    ESP_LOGI(kTag, "TX firmware query: 43 02 C1");
    uart_write_bytes(kUart, kFirmwareQuery, sizeof(kFirmwareQuery));
    draw_header("sent firmware query 43 02 C1");

    const int64_t query_deadline = esp_timer_get_time() + 1000000;
    while (esp_timer_get_time() < query_deadline) {
        collect_uart();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    if (!g_frame_len) {
        ESP_LOGW(kTag, "firmware query: zero RX bytes");
        draw_header("firmware query: ZERO RX BYTES");
    } else {
        show_bytes("FW REPLY", g_frame, g_frame_len);
        g_frame_len = 0;
    }

    while (true) {
        M5.update();
        if (M5.Touch.getDetail().wasClicked() || M5.BtnA.wasClicked()) {
            pulse_trigger();
        }
        collect_uart();
        emit_quiet_frame();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
