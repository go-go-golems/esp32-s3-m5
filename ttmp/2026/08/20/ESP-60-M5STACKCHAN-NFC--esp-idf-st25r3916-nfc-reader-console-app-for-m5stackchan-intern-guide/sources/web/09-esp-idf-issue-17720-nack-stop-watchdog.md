# I2C driver causes watchdog timer reset waiting for stop after nack (IDFGH-16612)

- **Canonical URL:** https://github.com/espressif/esp-idf/issues/17720
- **Repository:** `espressif/esp-idf`
- **Issue:** #17720
- **State at retrieval:** closed
- **Created:** 2025-10-08T23:48:08Z
- **Updated:** 2025-11-14T08:08:17Z
- **Retrieved:** 2026-08-21
- **Labels:** Type: Bug, Status: Done, Resolution: Done

> [!note] Source snapshot
> This file preserves an external issue discussion as research evidence. Claims in comments are reports from participants, not automatically verified facts. Consult the canonical issue for later updates.

## Issue body

### Answers checklist.

- [x] I have read the documentation [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/) and the issue is not addressed there.
- [x] I have updated my IDF branch (master or release) to the latest version and checked that the issue is present there.
- [x] I have searched the issue tracker for a similar issue and not found a similar issue.

### IDF version.

v5.4.2

### Espressif SoC revision.

ESP32 (revision 100)

### Operating System used.

macOS

### How did you build your project?

Command line with CMake

### If you are using Windows, please specify command line type.

None

### Development Kit.

LilyGo T-Relay

### Power Supply used.

External 3.3V

### What is the expected behavior?

The I2C driver should not cause a watchdog timer reset

### What is the actual behavior?

Occasionally, the ESP-IDF I2C driver hangs and triggers a watchdog timer reset while waiting for a STOP to occur after a NACK.  When a NACK occurs, `s_i2c_send_commands()` waits indefinitely for `i2c_ll_is_bus_busy()` to return false but the I2C FSM is stuck and it never clears so eventually the watchdog timer resets the microcontroller.

The ESP-IDF I2C driver can already recover from several types of I2C bus errors, it just doesn't handle this situation.  I suggest adding a timeout to exit the busy-wait loop when the bus is stuck and then call to `s_i2c_hw_fsm_reset()` and I have prepared a pull-request to illustrate.

Perhaps the root cause is noise on my I2C bus but I haven't been able to isolate it completely yet.  Regardless, I think the I2C driver should be able to recover from the transient.

### Steps to reproduce.

Sorry, I don't have specific steps to reproduce.  It might be possible to simulate the problem by glitching the I2C bus to trigger a NACK without a STOP.

### Debug Logs.

```plain
[12:23:35]E (1665918) task_wdt: Task watchdog got triggered. The following tasks/users did not reset the watchdog in time:
[12:23:35]E (1665918) task_wdt:  - loopTask (CPU 0)
[12:23:35]E (1665918) task_wdt: Tasks currently running:
[12:23:35]E (1665918) task_wdt: CPU 0: loopTask
[12:23:35]E (1665918) task_wdt: CPU 1: IDLE1
[12:23:35]E (1665918) task_wdt: Aborting.
[12:23:35]E (1665918) task_wdt: Print CPU 0 (current core) backtrace
[12:23:35]Backtrace: 0x4010880d:0x3ffbfa80 0x401089ae:0x3ffbfab0 0x40108a76:0x3ffbfae0 0x401094a7:0x3ffbfb10 0x400df6a5:0x3ffbfbb0 0x4019db17:0x3ffbfc60 0x400df369:0x3ffbfc90 0x400da8a5:0x3ffbfcc0 0x400daac0:0x3ffbfcf0 0x400dea76:0x3ffbfd10 0x400dea86:0x3ffbfd30 0x4019e9a1:0x3ffbfd50 0x400ebcdd:0x3ffbfd70 0x400eb3e9:0x3ffbfd90 0x400eeb76:0x3ffbfdc0 0x400dd052:0x3ffbfde0

0x4010880d: i2c_ll_is_bus_busy at /data/cache/platformio/packages/framework-espidf/components/hal/esp32/include/hal/i2c_ll.h:463
0x4010880d: s_i2c_send_commands at /data/cache/platformio/packages/framework-espidf/components/esp_driver_i2c/i2c_master.c:543
0x401089ae: s_i2c_transaction_start at /data/cache/platformio/packages/framework-espidf/components/esp_driver_i2c/i2c_master.c:649
0x40108a76: s_i2c_synchronous_transaction at /data/cache/platformio/packages/framework-espidf/components/esp_driver_i2c/i2c_master.c:945
0x40108a76: s_i2c_synchronous_transaction at /data/cache/platformio/packages/framework-espidf/components/esp_driver_i2c/i2c_master.c:925
0x401094a7: i2c_master_execute_defined_operations at /data/cache/platformio/packages/framework-espidf/components/esp_driver_i2c/i2c_master.c:1366
0x400df6a5: esphome::i2c::IDFI2CBus::write_readv(unsigned char, unsigned char const*, unsigned int, unsigned char*, unsigned int) at /data/build/van-central/src/esphome/components/i2c/i2c_bus_esp_idf.cpp:173
0x4019db17: esphome::i2c::I2CDevice::read_register(unsigned char, unsigned char*, unsigned int) at /data/build/van-central/src/esphome/components/i2c/i2c.cpp:33
0x400df369: esphome::i2c::I2CRegister::get() const at /data/build/van-central/src/esphome/components/i2c/i2c.cpp:93
0x400da8a5: esphome::aw9523::AW9523Component::digital_read(unsigned char) at /data/build/van-central/src/esphome/components/aw9523/aw9523.cpp:211
0x400daac0: esphome::aw9523::AW9523GPIOPin::digital_read() at /data/build/van-central/src/esphome/components/aw9523/aw9523_gpio_pin.cpp:18
0x400dea76: esphome::gpio::GPIOBinarySensor::loop() at /data/build/van-central/src/esphome/components/gpio/binary_sensor/gpio_binary_sensor.cpp:94
0x400dea86: {virtual override thunk({offset(-48)}, esphome::gpio::GPIOBinarySensor::loop())} at /data/build/van-central/src/esphome/components/gpio/binary_sensor/gpio_binary_sensor.h:58
0x4019e9a1: esphome::Component::call_loop() at /data/build/van-central/src/esphome/core/component.cpp:129
0x400ebcdd: esphome::Component::call() at /data/build/van-central/src/esphome/core/component.cpp:174
0x400eb3e9: esphome::Application::loop() at /data/build/van-central/src/esphome/core/application.cpp:145
0x400eeb76: loop() at /config/esphome/van-central.yaml:947
0x400dd052: esphome::loop_task(void*) at /data/build/van-central/src/esphome/components/esp32/core.cpp:82
```

### Diagnostic report archive.

_No response_

### More Information.

_No response_

## Comments

### Comment 1: j9brown

- **Created:** 2025-10-08T23:48:59Z
- **Updated:** 2025-10-08T23:48:59Z
- **URL:** https://github.com/espressif/esp-idf/issues/17720#issuecomment-3383581168

I also tried ESP-IDF 5.5.1 and the problem still exists there.

### Comment 2: littleboot

- **Created:** 2025-11-14T08:07:08Z
- **Updated:** 2025-11-14T08:08:17Z
- **URL:** https://github.com/espressif/esp-idf/issues/17720#issuecomment-3531470673

> I also tried ESP-IDF 5.5.1 and the problem still exists there.

Thank you for the issue report. I will be upgrading my application to the latest idf version that includes the fix and continue testing.
I experienced the same issue, I can confirm the issue was present in **IDF 5.5.1**

<img width="702" height="277" alt="Image" src="https://github.com/user-attachments/assets/7df0e618-dba7-4b92-89df-74f0c45a7777" />

<img width="740" height="286" alt="Image" src="https://github.com/user-attachments/assets/ebea1e6b-75b9-44f6-97b2-f61f7a236e9b" />

```c
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = PIN_I2C_SCL,
        .sda_io_num = PIN_I2C_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,  // external pullups present
        .intr_priority = 0, // default prio
        .trans_queue_depth = 0,
    };
    if (i2c_new_master_bus(&i2c_bus_config, &bus_handle) != ESP_OK)
    {
        return 1;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7, // unused custom send has been implemented
        .scl_wait_us = 0,  // The ADS111x does not perform clock-stretching, so need need to wait
        // .device_address = 0b1001000, // unused custom send function
        .device_address = I2C_DEVICE_ADDRESS_NOT_USED,
        .scl_speed_hz = 400E3,  // ESP32 supports max 400KHz ADS111x can go up to 3.4MHz (requires config see datasheet)
        .flags.disable_ack_check = false, // Optional ability to disable ACK check
    };
```

```
E (419685) task_wdt: Task watchdog got triggered. The following tasks/users did not reset the watchdog in time:
E (419685) task_wdt:  - IDLE1 (CPU 1)
E (419685) task_wdt: Tasks currently running:
E (419685) task_wdt: CPU 0: LVGL
E (419685) task_wdt: CPU 1: sensors_update_
E (419685) task_wdt: Print CPU 1 backtrace

Backtrace: 0x40086326:0x3FFB2750 0x400853B1:0x3FFB2770 0x401B3523:0x3FFD76E0 0x401B36E4:0x3FFD7710 0x401B37E6:0x3FFD7740 0x401B44E2:0x3FFD7770 0x4011C3CE:0x3FFD7820 0x4011C4DF:0x3FFD78E0 0x4011C89B:0x3FFD7910 0x4011BAE7:0x3FFD7940 0x4011AE6E:0x3FFD7980 0x400DB732:0x3FFD79E0 0x4008F199:0x3FFD7A00
--- 0x40086326: esp_crosscore_isr at C:/Users/admin/esp/v5.5.1/esp-idf/components/esp_system/crosscore_int.c:74
--- 0x400853b1: _xt_lowint1 at C:/Users/admin/esp/v5.5.1/esp-idf/components/xtensa/xtensa_vectors.S:1240
--- 0x401b3523: i2c_ll_is_bus_busy at C:/Users/admin/esp/v5.5.1/esp-idf/components/hal/esp32/include/hal/i2c_ll.h:463
--- (inlined by) s_i2c_send_commands at C:/Users/admin/esp/v5.5.1/esp-idf/components/esp_driver_i2c/i2c_master.c:543
--- 0x401b36e4: s_i2c_transaction_start at C:/Users/admin/esp/v5.5.1/esp-idf/components/esp_driver_i2c/i2c_master.c:649
--- 0x401b37e6: s_i2c_synchronous_transaction at C:/Users/admin/esp/v5.5.1/esp-idf/components/esp_driver_i2c/i2c_master.c:945
--- 0x401b44e2: i2c_master_execute_defined_operations at C:/Users/admin/esp/v5.5.1/esp-idf/components/esp_driver_i2c/i2c_master.c:1401
--- 0x4011c3ce: ads1115_interface_iic_read at C:/Users/admin/Desktop/repos/VN081122-V02/Firmware/VN081122/components/ads1115/ads1115_interface.c:146
--- 0x4011c4df: a_ads1115_iic_multiple_read at C:/Users/admin/Desktop/repos/VN081122-V02/Firmware/VN081122/components/ads1115/ads1115.c:83
--- 0x4011c89b: ads1115_single_read at C:/Users/admin/Desktop/repos/VN081122-V02/Firmware/VN081122/components/ads1115/ads1115.c:882
--- 0x4011bae7: ph_sensor_interface_adc_read at C:/Users/admin/Desktop/repos/VN081122-V02/Firmware/VN081122/components/ph_sensor/ph_sensor_interface.c:121
--- 0x4011ae6e: ph_sensor_update at C:/Users/admin/Desktop/repos/VN081122-V02/Firmware/VN081122/components/ph_sensor/ph_sensor.c:124
--- 0x400db732: sensors_update_task at C:/Users/admin/Desktop/repos/VN081122-V02/Firmware/VN081122/main/main.c:118
--- 0x4008f199: vPortTaskWrapper at C:/Users/admin/esp/v5.5.1/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:139
```

manged to make the error happen more often by giving the i2c sensor task a low priority and flooding the HTTPD webserver with requests and frequent reconnects.
