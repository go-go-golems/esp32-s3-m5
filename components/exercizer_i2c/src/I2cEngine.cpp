#include "exercizer/I2cEngine.h"

#include <inttypes.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "exercizer_i2c";

bool I2cEngine::Start(size_t queue_len) {
  if (queue_) {
    return true;
  }
  if (queue_len == 0) {
    queue_len = 16;
  }
  queue_ = xQueueCreate(queue_len, sizeof(exercizer_i2c_txn_t));
  if (!queue_) {
    ESP_LOGE(TAG, "failed to create i2c txn queue");
    return false;
  }
  BaseType_t ok = xTaskCreate(&I2cEngine::TaskEntry, "i2c_engine", 4096, this, 5, &task_);
  if (ok != pdPASS) {
    ESP_LOGE(TAG, "failed to create i2c engine task");
    return false;
  }
  return true;
}

bool I2cEngine::ApplyConfig(const exercizer_i2c_config_t &cfg) {
  if (configured_) {
    ESP_LOGW(TAG, "i2c already configured; ignoring reconfigure");
    return false;
  }

  i2c_master_bus_config_t bus_cfg = {};
  bus_cfg.i2c_port = static_cast<i2c_port_t>(cfg.port);
  bus_cfg.sda_io_num = static_cast<gpio_num_t>(cfg.sda);
  bus_cfg.scl_io_num = static_cast<gpio_num_t>(cfg.scl);
  bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
  bus_cfg.glitch_ignore_cnt = 7;
  bus_cfg.intr_priority = 0;
  bus_cfg.trans_queue_depth = 8;
  bus_cfg.flags.enable_internal_pullup = true;

  esp_err_t err = i2c_new_master_bus(&bus_cfg, &bus_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
    return false;
  }

  i2c_device_config_t dev_cfg = {};
  dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  dev_cfg.device_address = cfg.addr;
  dev_cfg.scl_speed_hz = cfg.hz;

  err = i2c_master_bus_add_device(bus_, &dev_cfg, &dev_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(err));
    return false;
  }

  cfg_ = cfg;
  configured_ = true;
  ESP_LOGI(TAG, "i2c configured: port=%d addr=0x%02x sda=%d scl=%d hz=%" PRIu32,
           (int)cfg.port,
           (int)cfg.addr,
           (int)cfg.sda,
           (int)cfg.scl,
           cfg.hz);
  return true;
}

bool I2cEngine::EnqueueTxn(const exercizer_i2c_txn_t &txn) {
  if (!queue_) {
    return false;
  }
  return xQueueSend(queue_, &txn, 0) == pdTRUE;
}

void I2cEngine::Scan(const exercizer_i2c_scan_t &scan) {
  if (!bus_) {
    ESP_LOGW(TAG, "i2c scan skipped; bus not configured");
    return;
  }

  uint8_t start = scan.start_addr;
  uint8_t end = scan.end_addr;
  if (start == 0 && end == 0) {
    start = 0x08;
    end = 0x77;
  }
  if (start > 0x7f) {
    start = 0x7f;
  }
  if (end > 0x7f) {
    end = 0x7f;
  }
  if (start > end) {
    uint8_t tmp = start;
    start = end;
    end = tmp;
  }

  int timeout_ms = scan.timeout_ms;
  if (timeout_ms <= 0) {
    timeout_ms = 50;
  }

  int found = 0;
  for (uint8_t addr = start;; addr++) {
    esp_err_t err = i2c_master_probe(bus_, addr, timeout_ms);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "i2c scan hit: addr=0x%02x", addr);
      found++;
    }
    if (addr == end) {
      break;
    }
  }
  ESP_LOGI(TAG, "i2c scan done: start=0x%02x end=0x%02x found=%d", start, end, found);
}

void I2cEngine::HandleEvent(const exercizer_ctrl_event_t &ev) {
  if (ev.type == EXERCIZER_CTRL_I2C_CONFIG) {
    if (ev.data_len < sizeof(exercizer_i2c_config_t)) {
      ESP_LOGW(TAG, "i2c config too small (len=%u)", ev.data_len);
      return;
    }
    exercizer_i2c_config_t cfg = {};
    memcpy(&cfg, ev.data, sizeof(cfg));
    (void)ApplyConfig(cfg);
    return;
  }

  if (ev.type == EXERCIZER_CTRL_I2C_TXN) {
    if (ev.data_len < sizeof(exercizer_i2c_txn_t)) {
      ESP_LOGW(TAG, "i2c txn too small (len=%u)", ev.data_len);
      return;
    }
    exercizer_i2c_txn_t txn = {};
    memcpy(&txn, ev.data, sizeof(txn));
    if (!EnqueueTxn(txn)) {
      ESP_LOGW(TAG, "i2c txn queue full");
    }
  }

  if (ev.type == EXERCIZER_CTRL_I2C_SCAN) {
    if (ev.data_len < sizeof(exercizer_i2c_scan_t)) {
      ESP_LOGW(TAG, "i2c scan payload too small (len=%u)", ev.data_len);
      return;
    }
    exercizer_i2c_scan_t scan = {};
    memcpy(&scan, ev.data, sizeof(scan));
    Scan(scan);
  }
}

void I2cEngine::TaskEntry(void *arg) {
  auto *self = static_cast<I2cEngine *>(arg);
  if (self) {
    self->TaskLoop();
  }
  vTaskDelete(nullptr);
}

void I2cEngine::TaskLoop() {
  exercizer_i2c_txn_t txn = {};
  while (true) {
    if (xQueueReceive(queue_, &txn, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    if (!configured_ || !dev_) {
      ESP_LOGW(TAG, "i2c txn dropped; not configured");
      continue;
    }

    if (txn.write_len > 0) {
      esp_err_t err = i2c_master_transmit(dev_, txn.write, txn.write_len, 100);
      if (err != ESP_OK) {
        ESP_LOGW(TAG, "i2c tx failed: %s", esp_err_to_name(err));
        continue;
      }
    }

    if (txn.read_len > 0) {
      uint8_t buf[EXERCIZER_I2C_WRITE_MAX] = {0};
      size_t want = txn.read_len;
      if (want > sizeof(buf)) {
        want = sizeof(buf);
      }
      esp_err_t err = i2c_master_receive(dev_, buf, want, 100);
      if (err != ESP_OK) {
        ESP_LOGW(TAG, "i2c rx failed: %s", esp_err_to_name(err));
        continue;
      }
      ESP_LOGI(TAG, "i2c rx len=%u", (unsigned)want);
      for (size_t i = 0; i < want; i++) {
        ESP_LOGI(TAG, "i2c rx[%u]=0x%02x", (unsigned)i, buf[i]);
      }
    }
  }
}
