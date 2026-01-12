#pragma once

#include <stdint.h>

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "exercizer/ControlPlaneTypes.h"

class I2cEngine {
 public:
  bool Start(size_t queue_len = 16);
  void HandleEvent(const exercizer_ctrl_event_t &ev);

 private:
  void TaskLoop();
  bool ApplyConfig(const exercizer_i2c_config_t &cfg);
  bool EnqueueTxn(const exercizer_i2c_txn_t &txn);
  void Scan(const exercizer_i2c_scan_t &scan);
  void Deconfig();

  static void TaskEntry(void *arg);

  QueueHandle_t queue_ = nullptr;
  TaskHandle_t task_ = nullptr;

  i2c_master_bus_handle_t bus_ = nullptr;
  i2c_master_dev_handle_t dev_ = nullptr;
  exercizer_i2c_config_t cfg_ = {};
  bool configured_ = false;
};
