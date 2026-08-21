# ESP-IDF 5.5.4 ESP32-S3 I2C Programming Guide

- **Canonical URL:** https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/i2c.html
- **Retrieved:** 2026-08-21

## Inter-Integrated Circuit (I2C)

[\[中文\]](https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32s3/api-reference/peripherals/i2c.html)

## Introduction

I2C is a serial, synchronous, multi-device, half-duplex communication protocol that allows co-existence of multiple
masters and slaves on the same bus. I2C uses two bidirectional open-drain lines: serial data line (SDA) and serial
clock line (SCL), pulled up by resistors.

ESP32-S3 has 2 I2C controller(s) (also called port), responsible for handling communication on the I2C bus.

A single I2C controller can be a master or a slave.

Typically, an I2C slave device has a 7-bit address or 10-bit address. ESP32-S3 supports both I2C Standard-mode (Sm) and
Fast-mode (Fm) which can go up to 100 kHz and 400 kHz respectively.

> [!warning] Warning
> The clock frequency of SCL in master mode should not be larger than 400 kHz.

> [!note] Note
> The frequency of SCL is influenced by both the pull-up resistor and the wire capacitance. Therefore, it is strongly
recommended to choose appropriate pull-up resistors to make the frequency accurate. The recommended value for pull-up
resistors usually ranges from 1 kΩ to 10 kΩ.
>
> Keep in mind that the higher the frequency, the smaller the pull-up resistor should be (but not less than 1 kΩ).
Indeed, large resistors will decline the current, which will increase the clock switching time and reduce the
frequency. A range of 2 kΩ to 5 kΩ is recommended, but adjustments may also be necessary depending on their current
draw requirements.

> [!note] Note
> We realized that our first version of the I2C slave driver had some problems and was not easy to use, so we have
prepared a second version of the I2C slave driver, which solves many of the problems with our current I2C slave and
which will be the focus of our maintenance. We encourage and recommend that you use the second version of the I2C slave
driver, which you can do by enabling
[CONFIG\_I2C\_ENABLE\_SLAVE\_DRIVER\_VERSION\_2](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-refer
ence/kconfig-reference.html#config-i2c-enable-slave-driver-version-2). This document focuses on the content of I2C
slave v2.0. If you still want to read programming guide of I2C slave v1.0, please refer to [I2C Slave
v1.0](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/i2c_slave_v1.html#i2c-slav
e-v1). The I2C slave v1.0 driver will be removed with the IDF v6.0 update.

## I2C Clock Configuration

- `i2c_clock_source_t::I2C_CLK_SRC_DEFAULT`: Default I2C source clock.
- `i2c_clock_source_t::I2C_CLK_SRC_XTAL`: External crystal for I2C clock source.
- `i2c_clock_source_t::I2C_CLK_SRC_RC_FAST`: Internal 20 MHz RC oscillator for I2C clock source.

## I2C File Structure

![I2C file structure](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/_images/i2c_code_structure.png)

I2C file structure 

**Public headers that need to be included in the I2C application**

- `i2c.h`: The header file of legacy I2C APIs (for apps using legacy driver).
- `i2c_master.h`: The header file that provides standard communication mode specific APIs (for apps using new driver
with master mode).
- `i2c_slave.h`: The header file that provides standard communication mode specific APIs (for apps using new driver
with slave mode).

> [!note] Note
> The legacy driver can't coexist with the new driver. Include `i2c.h` to use the legacy driver or the other two
headers to use the new driver. Please keep in mind that the legacy driver is now deprecated and will be removed in
future.

**Public headers that have been included in the headers above**

- `i2c_types_legacy.h`: The legacy public types that are only used in the legacy driver.
- `i2c_types.h`: The header file that provides public types.

## Functional Overview

The I2C driver offers following services:

- \- covers how to allocate I2C bus with properly set of configurations. It also covers how to recycle the resources
when they finished working.
- \- covers behavior of I2C master controller. Introduce data transmit, data receive, and data transmit and receive.
- \- covers behavior of I2C slave controller. Involve data transmit and data receive.
- \- describes how different source clock will affect power consumption.
- \- describes tips on how to make the I2C interrupt work better along with a disabled cache.
- \- lists which APIs are guaranteed to be thread safe by the driver.
- \- lists the supported Kconfig options that can bring different effects to the driver.

### Resource Allocation

The I2C master bus is represented by in the driver. The available ports are managed in a resource pool that allocates a
free port on request.

#### Install I2C master bus and device

The I2C master bus is designed based on bus-device model. So and are required separately to allocate the I2C master bus
instance and I2C device instance.

![I2C master bus-device
module](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/_images/i2c_master_module.png)

I2C master bus-device module 

I2C master bus requires the configuration that specified by:

- sets the I2C port used by the controller.
- sets the GPIO number for the serial data bus (SDA).
- sets the GPIO number for the serial clock bus (SCL).
- selects the source clock for I2C bus. The available clocks are listed in. For the effect on power consumption of
different clock source, please refer to section.
- sets the glitch period of master bus, if the glitch period on the line is less than this value, it can be filtered
out, typically value is 7.
- sets the priority of the interrupt. If set to `0`, then the driver will use a interrupt with low or medium priority
(priority level may be one of 1, 2 or 3), otherwise use the priority indicated by. Please use the number form (1, 2,
3), not the bitmask form ((1<<1), (1<<2), (1<<3)).
- sets the depth of internal transfer queue. Only valid in asynchronous transaction.
- enables internal pullups. Note: This is not strong enough to pullup buses under high-speed frequency. A suitable
external pullup is recommended.
- configures if the driver allows the system to power down the peripheral in light sleep mode. Before entering sleep,
the system will backup the I2C register context, which will be restored later when the system exit the sleep mode.
Powering down the peripheral can save more power, but at the cost of more memory consumed to save the register context.
It's a tradeoff between power consumption and memory consumption. This configuration option relies on specific hardware
feature, if you enable it on an unsupported chip, you will see error message like `not able to power down in light
sleep`.

If the configurations in is specified, then can be called to allocate and initialize an I2C master bus. This function
will return an I2C bus handle if it runs correctly. Specifically, when there are no more I2C port available, this
function will return
[`ESP_ERR_NOT_FOUND`](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#c.
ESP_ERR_NOT_FOUND "ESP_ERR_NOT_FOUND") error.

I2C master device requires the configuration that specified by:

- configure the address bit length of the slave device. It can be chosen from enumerator or (if supported).
- sets the I2C device raw address. Please parse the device address to this member directly. For example, the device
address is 0x28, then parse 0x28 to, don't carry a write or read bit.
- sets the SCL line frequency of this device.
- sets the SCL await time (in μs). Usually this value should not be very small because slave stretch will happen in
pretty long time (It's possible even stretch for 12 ms). Set `0` means use default register value.

Once the structure is populated with mandatory parameters, can be called to allocate an I2C device instance and mounted
to the master bus then. This function will return an I2C device handle if it runs correctly. Specifically, when the I2C
bus is not initialized properly, calling this function will result in a
[`ESP_ERR_INVALID_ARG`](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#
c.ESP_ERR_INVALID_ARG "ESP_ERR_INVALID_ARG") error.

```c
#include "driver/i2c_master.h"

i2c_master_bus_config_t i2c_mst_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = TEST_I2C_PORT,
    .scl_io_num = I2C_MASTER_SCL_IO,
    .sda_io_num = I2C_MASTER_SDA_IO,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
};

i2c_master_bus_handle_t bus_handle;
ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x58,
    .scl_speed_hz = 100000,
};

i2c_master_dev_handle_t dev_handle;
ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
```

#### Get I2C master handle via port

When the I2C master handle has been initialized in one module (e.g. the audio module), but it is not convenient to
acquire this handle in another module (e.g. the video module). You can use the helper function, to retrieve the
initialized handle via port. Ensure that the handle has already been initialized beforehand to avoid potential errors.

```c
// Source File 1
#include "driver/i2c_master.h"
i2c_master_bus_handle_t bus_handle;
i2c_master_bus_config_t i2c_mst_config = {
    ... // same as others
};
ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

// Source File 2
#include "driver/i2c_master.h"
i2c_master_bus_handle_t handle;
ESP_ERROR_CHECK(i2c_master_get_bus_handle(0, &handle));
```

#### Uninstall I2C master bus and device

If a previously installed I2C bus or device is no longer needed, it's recommended to recycle the resource by calling
or, so as to release the underlying hardware.

Please note that removing all devices attached to bus before delete the master bus.

#### Install I2C slave device

I2C slave requires the configuration specified by:

- sets the I2C port used by the controller.
- sets the GPIO number for serial data bus (SDA).
- sets the GPIO number for serial clock bus (SCL).
- selects the source clock for I2C bus. The available clocks are listed in. For the effect on power consumption of
different clock source, please refer to section.
- sets the sending software buffer length.
- `i2c_slave_config_t::receive_buf_depth` sets the receiving software buffer length.
- sets the priority of the interrupt. If set to `0`, then the driver will use a interrupt with low or medium priority
(priority level may be one of 1, 2 or 3), otherwise use the priority indicated by. Please use the number form (1, 2,
3), instead of the bitmask form ((1<<1), (1<<2), (1<<3)). Please pay attention that once the interrupt priority is set,
it cannot be changed until is called.
- Set this variable to `I2C_ADDR_BIT_LEN_10` if the slave should have a 10-bit address.
- If set, the driver will backup/restore the I2C registers before/after entering/exist sleep mode. By this approach,
the system can power off I2C's power domain. This can save power, but at the expense of more RAM being consumed.
- Set this to true to enable the slave broadcast. When the slave receives the general call address 0x00 from the master
and the R/W bit followed is 0, it responds to the master regardless of its own address.
- `i2c_slave_config_t::enable_internal_pullup` Set this to enable internal pull-up. Even though, an output pull-up
resistance is strongly recommended.

Once the structure is populated with mandatory parameters, can be called to allocate and initialize an I2C master bus.
This function will return an I2C bus handle if it runs correctly. Specifically, when there are no more I2C port
available, this function will return
[`ESP_ERR_NOT_FOUND`](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#c.
ESP_ERR_NOT_FOUND "ESP_ERR_NOT_FOUND") error.

```c
i2c_slave_config_t i2c_slv_config = {
    .i2c_port = I2C_SLAVE_NUM,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .scl_io_num = I2C_SLAVE_SCL_IO,
    .sda_io_num = I2C_SLAVE_SDA_IO,
    .slave_addr = ESP_SLAVE_ADDR,
    .send_buf_depth = 100,
    .receive_buf_depth = 100,
};

i2c_slave_dev_handle_t slave_handle;
ESP_ERROR_CHECK(i2c_new_slave_device(&i2c_slv_config, &slave_handle));
```

#### Uninstall I2C slave device

If a previously installed I2C bus is no longer needed, it's recommended to recycle the resource by calling, so that to
release the underlying hardware.

### I2C Master Controller

After installing the I2C master driver by, ESP32-S3 is ready to communicate with other I2C devices. I2C APIs allow the
standard transactions. Like the wave as follows:

#### I2C Master Write

After installing I2C master bus successfully, you can simply call to write data to the slave device. The principle of
this function can be explained by following chart.

In order to organize the process, the driver uses a command link, that should be populated with a sequence of commands
and then passed to I2C controller for execution.

![I2C master write to
slave](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/_images/i2c_master_write_slave.png)

I2C master write to slave 

Simple example for writing data to slave:

```c
#define DATA_LENGTH 100
i2c_master_bus_config_t i2c_mst_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_PORT_NUM_0,
    .scl_io_num = I2C_MASTER_SCL_IO,
    .sda_io_num = I2C_MASTER_SDA_IO,
    .glitch_ignore_cnt = 7,
};
i2c_master_bus_handle_t bus_handle;

ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x58,
    .scl_speed_hz = 100000,
};

i2c_master_dev_handle_t dev_handle;
ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, data_wr, DATA_LENGTH, -1));
```

I2C master write also supports transmit multi-buffer in one transaction. Take following transaction as a simple example:

```c
uint8_t control_phase_byte = 0;
size_t control_phase_size = 0;
if (/*condition*/) {
    control_phase_byte = 1;
    control_phase_size = 1;
}

uint8_t *cmd_buffer = NULL;
size_t cmd_buffer_size = 0;
if (/*condition*/) {
    uint8_t cmds[4] = {BYTESHIFT(lcd_cmd, 3), BYTESHIFT(lcd_cmd, 2), BYTESHIFT(lcd_cmd, 1), BYTESHIFT(lcd_cmd, 0)};
    cmd_buffer = cmds;
    cmd_buffer_size = 4;
}

uint8_t *lcd_buffer = NULL;
size_t lcd_buffer_size = 0;
if (buffer) {
    lcd_buffer = (uint8_t*)buffer;
    lcd_buffer_size = buffer_size;
}

i2c_master_transmit_multi_buffer_info_t lcd_i2c_buffer[3] = {
    {.write_buffer = &control_phase_byte, .buffer_size = control_phase_size},
    {.write_buffer = cmd_buffer, .buffer_size = cmd_buffer_size},
    {.write_buffer = lcd_buffer, .buffer_size = lcd_buffer_size},
};

i2c_master_multi_buffer_transmit(handle, lcd_i2c_buffer, sizeof(lcd_i2c_buffer) /
sizeof(i2c_master_transmit_multi_buffer_info_t), -1);
```

#### I2C Master Read

After installing I2C master bus successfully, you can simply call to read data from the slave device. The principle of
this function can be explained by following chart.

![I2C master read from
slave](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/_images/i2c_master_read_slave.png)

I2C master read from slave 

Simple example for reading data from slave:

```c
#define DATA_LENGTH 100
i2c_master_bus_config_t i2c_mst_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_PORT_NUM_0,
    .scl_io_num = I2C_MASTER_SCL_IO,
    .sda_io_num = I2C_MASTER_SDA_IO,
    .glitch_ignore_cnt = 7,
};
i2c_master_bus_handle_t bus_handle;

ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x58,
    .scl_speed_hz = 100000,
};

i2c_master_dev_handle_t dev_handle;
ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

i2c_master_receive(dev_handle, data_rd, DATA_LENGTH, -1);
```

#### I2C Master Write and Read

Some I2C device needs write configurations before reading data from it. Therefore, an interface called can help. The
principle of this function can be explained by following chart.

![I2C master write to slave and read from
slave](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/_images/i2c_master_write_read_slave.png)

I2C master write to slave and read from slave 

Please note that no STOP condition bit is inserted between the write and read operations; therefore, this function is
suited to read a register from an I2C device. A simple example for writing and reading from a slave device:

```c
i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x58,
    .scl_speed_hz = 100000,
};

i2c_master_dev_handle_t dev_handle;
ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
uint8_t buf[20] = {0x20};
uint8_t buffer[2];
ESP_ERROR_CHECK(i2c_master_transmit_receive(dev_handle, buf, sizeof(buf), buffer, 2, -1));
```

#### I2C Master Probe

I2C driver can use to detect whether the specific device has been connected on I2C bus. If this function return
`ESP_OK`, that means the device has been detected.

> [!important] Important
> Pull-ups must be connected to the SCL and SDA pins when this function is called. If you get ESP\_ERR\_TIMEOUT while
xfer\_timeout\_ms was parsed correctly, you should check the pull-up resistors. If you do not have proper resistors
nearby, setting flags.enable\_internal\_pullup as true is also acceptable.

![I2C master probe](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/_images/i2c_master_probe.png)

I2C master probe 

Simple example for probing an I2C device:

```c
i2c_master_bus_config_t i2c_mst_config_1 = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = TEST_I2C_PORT,
    .scl_io_num = I2C_MASTER_SCL_IO,
    .sda_io_num = I2C_MASTER_SDA_IO,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
};
i2c_master_bus_handle_t bus_handle;

ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config_1, &bus_handle));
ESP_ERROR_CHECK(i2c_master_probe(bus_handle, 0x22, -1));
ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
```

#### I2C Master Execute Customized Transactions

Not all I2C devices strictly adhere to the standard I2C protocol, as different manufacturers may implement custom
variations. For example, some devices require the address to be shifted, while others do not. Similarly, certain
devices mandate acknowledgment (ACK) checks for specific operations, whereas others might not. To accommodate these
variations, function allow developers to define and execute fully customized I2C transactions. This flexibility ensures
seamless communication with non-standard devices by tailoring the transaction sequence, addressing, and acknowledgment
behavior to the device's specific requirements.

> [!note] Note
> If you want to define your address in, please set as `I2C_DEVICE_ADDRESS_NOT_USED` to skip internal address
configuration in driver.

For address configuration of user defined transactions, given that the device address is `0x20`, there are two
situations. See following example:

```c
i2c_device_config_t i2c_device = {
    .device_address = I2C_DEVICE_ADDRESS_NOT_USED,
    .scl_speed_hz = 100 * 1000,
    .scl_wait_us = 20000,
};

i2c_master_dev_handle_t dev_handle;

i2c_master_bus_add_device(bus_handle, &i2c_device, &dev_handle);

// Situation one: The device does not allow device address shift
uint8_t address1 = 0x20;
i2c_operation_job_t i2c_ops1[] = {
    { .command = I2C_MASTER_CMD_START },
    { .command = I2C_MASTER_CMD_WRITE, .write = { .ack_check = false, .data = (uint8_t *) &address1, .total_bytes = 1 }
},
    { .command = I2C_MASTER_CMD_STOP },
};

// Situation one: The device address should be left shifted by one byte to include a write bit or a read bit (official
protocol)
uint8_t address2 = (0x20 << 1 | 0); // (0x20 << 1 | 1)
i2c_operation_job_t i2c_ops2[] = {
    { .command = I2C_MASTER_CMD_START },
    { .command = I2C_MASTER_CMD_WRITE, .write = { .ack_check = false, .data = (uint8_t *) &address2, .total_bytes = 1 }
},
    { .command = I2C_MASTER_CMD_STOP },
};
```

Some devices do not require an address, and allow direct transaction with data:

```c
uint8_t data[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

i2c_operation_job_t i2c_ops[] = {
    { .command = I2C_MASTER_CMD_START },
    { .command = I2C_MASTER_CMD_WRITE, .write = { .ack_check = false, .data = (uint8_t *)data, .total_bytes = 8 } },
    { .command = I2C_MASTER_CMD_STOP },
};

i2c_master_execute_defined_operations(dev_handle, i2c_ops, sizeof(i2c_ops) / sizeof(i2c_operation_job_t), -1);
```

The principle of read operations is the same as that of write operations. Note to always ensure the last byte read
before the stop condition is a `NACK`. An example is as follows:

```c
uint8_t address = (0x20 << 1 | 1);
uint8_t rcv_data[10] = {};

i2c_operation_job_t i2c_ops[] = {
    { .command = I2C_MASTER_CMD_START },
    { .command = I2C_MASTER_CMD_WRITE, .write = { .ack_check = false, .data = (uint8_t *) &address, .total_bytes = 1 }
},
    { .command = I2C_MASTER_CMD_READ, .read = { .ack_value = I2C_ACK_VAL, .data = (uint8_t *)rcv_data, .total_bytes = 9
} },
    { .command = I2C_MASTER_CMD_READ, .read = { .ack_value = I2C_NACK_VAL, .data = (uint8_t *)(rcv_data + 9),
.total_bytes = 1 } }, // This must be NACK
    { .command = I2C_MASTER_CMD_STOP },
};

i2c_master_execute_defined_operations(dev_handle, i2c_ops, sizeof(i2c_ops) / sizeof(i2c_operation_job_t), -1);
```

### I2C Slave Controller

After installing the I2C slave driver by, ESP32-S3 is ready to communicate with other I2C masters as a slave.

The I2C slave is not as active as the I2C master, which knows when to send data and when to receive it. The I2C slave
is very passive in most cases, meaning the I2C slave's ability to send and receive data is largely dependent on the
master's actions. Therefore, we implement two callback functions in the driver to handle read and write requests from
the I2C master.

#### I2C Slave Write

You can get I2C slave write event by registering `i2c_slave_event_callbacks_t::on_request` callback. Then, in a task
where the request event is triggered, you can call `i2c_slave_write` to send data.

A simple example for transmitting data:

```c
// Prepare a callback function
static bool i2c_slave_request_cb(i2c_slave_dev_handle_t i2c_slave, const i2c_slave_request_event_data_t *evt_data, void
*arg)
{
    i2c_slave_event_t evt = I2C_SLAVE_EVT_TX;
    BaseType_t xTaskWoken = 0;
    xQueueSendFromISR(context->event_queue, &evt, &xTaskWoken);
    return xTaskWoken;
}

// Register callback in a task
i2c_slave_event_callbacks_t cbs = {
    .on_request = i2c_slave_request_cb,
};
ESP_ERROR_CHECK(i2c_slave_register_event_callbacks(context.handle, &cbs, &context));

// Wait for request event and send data in a task
static void i2c_slave_task(void *arg)
{
    uint8_t buffer_size = 64;
    uint32_t write_len;
    uint8_t *data_buffer;

    while (true) {
        i2c_slave_event_t evt;
        if (xQueueReceive(context->event_queue, &evt, 10) == pdTRUE) {
            ESP_ERROR_CHECK(i2c_slave_write(handle, data_buffer, buffer_size, &write_len, 1000));
        }
    }
    vTaskDelete(NULL);
}
```

#### I2C Slave Read

Same as write event, you can get I2C slave read event by registering `i2c_slave_event_callbacks_t::on_receive`
callback. Then, in a task where the request event is triggered, you can save the data and do what you want.

A simple example for receiving data:

```c
// Prepare a callback function
static bool i2c_slave_receive_cb(i2c_slave_dev_handle_t i2c_slave, const i2c_slave_rx_done_event_data_t *evt_data, void
*arg)
{
    i2c_slave_event_t evt = I2C_SLAVE_EVT_RX;
    BaseType_t xTaskWoken = 0;
    // You can get data and length via i2c_slave_rx_done_event_data_t
    xQueueSendFromISR(context->event_queue, &evt, &xTaskWoken);
    return xTaskWoken;
}

// Register callback in a task
i2c_slave_event_callbacks_t cbs = {
    .on_receive = i2c_slave_receive_cb,
};
ESP_ERROR_CHECK(i2c_slave_register_event_callbacks(context.handle, &cbs, &context));
```

### Power Management

If the controller clock source is selected to
[`I2C_CLK_SRC_XTAL`](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/clk_tree.ht
ml#_CPPv4N24soc_periph_i2c_clk_src_t16I2C_CLK_SRC_XTALE "I2C_CLK_SRC_XTAL"), then the driver won't install power
management lock for it, which is more suitable for a low power application as long as the source clock can still
provide sufficient resolution.

### IRAM Safe

By default, the I2C interrupt will be deferred when the cache is disabled for reasons like writing or erasing flash.
Thus the event callback functions will not get executed in time, which is not expected in a real-time application.

There's a Kconfig option
[CONFIG\_I2C\_ISR\_IRAM\_SAFE](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/kconfig-refer
ence.html#config-i2c-isr-iram-safe) that will:

1. Enable the interrupt being serviced even when cache is disabled.
2. Place all functions that used by the ISR into IRAM.
3. Place driver object into DRAM (in case it's mapped to PSRAM by accident).

This will allow the interrupt to run while the cache is disabled but will come at the cost of increased IRAM
consumption.

### Thread Safety

The factory function and are guaranteed to be thread safe by the driver, which means that the functions can be called
from different RTOS tasks without protection by extra locks.

I2C master operation functions are also guaranteed to be thread safe by bus operation semaphore.

I2C slave operation functions are also guaranteed to be thread safe by bus operation semaphore.

- `i2c_slave_write()`

Other functions are not guaranteed to be thread-safe. Thus, you should avoid calling them in different tasks without
mutex protection.

### Kconfig Options

-
[CONFIG\_I2C\_ISR\_IRAM\_SAFE](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/kconfig-refer
ence.html#config-i2c-isr-iram-safe) controls whether the default ISR handler can work when cache is disabled, see also
for more information.
-
[CONFIG\_I2C\_ENABLE\_DEBUG\_LOG](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/kconfig-re
ference.html#config-i2c-enable-debug-log) is used to enable the debug log at the cost of increased firmware binary size.
-
[CONFIG\_I2C\_ENABLE\_SLAVE\_DRIVER\_VERSION\_2](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-refer
ence/kconfig-reference.html#config-i2c-enable-slave-driver-version-2) is used to enable the I2C slave driver v2.0.

## Application Examples

- [peripherals/i2c/i2c\_basic](https://github.com/espressif/esp-idf/tree/v5.5.4/examples/peripherals/i2c/i2c_basic)
demonstrates the basic steps to initialize the I2C master driver and read data from a MPU9250 sensor.
- [peripherals/i2c/i2c\_eeprom](https://github.com/espressif/esp-idf/tree/v5.5.4/examples/peripherals/i2c/i2c_eeprom)
demonstrates how to use the I2C master mode to read and write data from a connected EEPROM.
- [peripherals/i2c/i2c\_tools](https://github.com/espressif/esp-idf/tree/v5.5.4/examples/peripherals/i2c/i2c_tools)
demonstrates how to use the I2C Tools for developing I2C related applications, providing command-line tools for
configuring the I2C bus, scanning for devices, reading and setting registers, and examining registers.
-
[peripherals/i2c/i2c\_slave\_network\_sensor](https://github.com/espressif/esp-idf/tree/v5.5.4/examples/peripherals/i2c/
i2c_slave_network_sensor) demonstrates how to use the I2C slave for developing I2C related applications, providing how
I2C slave can behave as a network sensor, and use event callbacks to receive and send data.

## API Reference

### Header File

-
[components/esp\_driver\_i2c/include/driver/i2c\_master.h](https://github.com/espressif/esp-idf/blob/v5.5.4/components/e
sp_driver_i2c/include/driver/i2c_master.h)
- This header file can be included with:
	> ```c
	> #include "driver/i2c_master.h"
	> ```
- This header file is a part of the API provided by the `esp_driver_i2c` component. To declare that your component
depends on `esp_driver_i2c`, add the following to your CMakeLists.txt:
	> ```
	> REQUIRES esp_driver_i2c
	> ```
	>
	> or
	>
	> ```
	> PRIV_REQUIRES esp_driver_i2c
	> ```

### Functions

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_new\_master\_bus(const \*bus\_config, \*ret\_bus\_handle)
[](#_CPPv418i2c_new_master_busPK23i2c_master_bus_config_tP23i2c_master_bus_handle_t "Permalink to this definition")

Allocate an I2C master bus.

Parameters:

- **bus\_config** -- **\[in\]** I2C master bus configuration.
- **ret\_bus\_handle** -- **\[out\]** I2C bus handle

Returns:

- ESP\_OK: I2C master bus initialized successfully.
- ESP\_ERR\_INVALID\_ARG: I2C bus initialization failed because of invalid argument.
- ESP\_ERR\_NO\_MEM: Create I2C bus failed because of out of memory.
- ESP\_ERR\_NOT\_FOUND: No more free bus.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_master\_bus\_add\_device( bus\_handle, const \*dev\_config, \*ret\_handle)
[](#_CPPv425i2c_master_bus_add_device23i2c_master_bus_handle_tPK19i2c_device_config_tP23i2c_master_dev_handle_t
"Permalink to this definition")

Add I2C master BUS device.

Parameters:

- **bus\_handle** -- **\[in\]** I2C bus handle.
- **dev\_config** -- **\[in\]** device config.
- **ret\_handle** -- **\[out\]** device handle.

Returns:

- ESP\_OK: Create I2C master device successfully.
- ESP\_ERR\_INVALID\_ARG: I2C bus initialization failed because of invalid argument.
- ESP\_ERR\_NO\_MEM: Create I2C bus failed because of out of memory.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_del\_master\_bus( bus\_handle) [](#_CPPv418i2c_del_master_bus23i2c_master_bus_handle_t
"Permalink to this definition")

Deinitialize the I2C master bus and delete the handle.

Parameters:

**bus\_handle** -- **\[in\]** I2C bus handle.

Returns:

- ESP\_OK: Delete I2C bus success, otherwise, failed.
- Otherwise: Some module delete failed.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_master\_bus\_rm\_device( handle)
[](#_CPPv424i2c_master_bus_rm_device23i2c_master_dev_handle_t "Permalink to this definition")

I2C master bus delete device.

Parameters:

**handle** -- i2c device handle

Returns:

- ESP\_OK: If device is successfully deleted.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_master\_transmit( i2c\_dev, const uint8\_t \*write\_buffer, size\_t write\_size, int
xfer\_timeout\_ms) [](#_CPPv419i2c_master_transmit23i2c_master_dev_handle_tPK7uint8_t6size_ti "Permalink to this
definition")

Perform a write transaction on the I2C bus. The transaction will be undergoing until it finishes or it reaches the
timeout provided.

> [!note] Note
> If a callback was registered with `i2c_master_register_event_callbacks`, the transaction will be asynchronous, and
thus, this function will return directly, without blocking. You will get finish information from callback. Besides,
data buffer should always be completely prepared when callback is registered, otherwise, the data will get corrupt.

Parameters:

- **i2c\_dev** -- **\[in\]** I2C master device handle that created by `i2c_master_bus_add_device`.
- **write\_buffer** -- **\[in\]** Data bytes to send on the I2C bus.
- **write\_size** -- **\[in\]** Size, in bytes, of the write buffer.
- **xfer\_timeout\_ms** -- **\[in\]** Wait timeout, in ms. Note: -1 means wait forever.

Returns:

- ESP\_OK: I2C master transmit success
- ESP\_ERR\_INVALID\_ARG: I2C master transmit parameter invalid.
- ESP\_ERR\_TIMEOUT: Operation timeout(larger than xfer\_timeout\_ms) because the bus is busy or hardware crash.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_master\_multi\_buffer\_transmit( i2c\_dev, \*buffer\_info\_array, size\_t array\_size, int
xfer\_timeout\_ms)
[](#_CPPv432i2c_master_multi_buffer_transmit23i2c_master_dev_handle_tP39i2c_master_transmit_multi_buffer_info_t6size_
ti "Permalink to this definition")

Transmit multiple buffers of data over an I2C bus.

This function transmits multiple buffers of data over an I2C bus using the specified I2C master device handle. It takes
in an array of buffer information structures along with the size of the array and a transfer timeout value in
milliseconds.

Parameters:

- **i2c\_dev** -- I2C master device handle that created by `i2c_master_bus_add_device`.
- **buffer\_info\_array** -- Pointer to buffer information array.
- **array\_size** -- size of buffer information array.
- **xfer\_timeout\_ms** -- Wait timeout, in ms. Note: -1 means wait forever.

Returns:

- ESP\_OK: I2C master transmit success
- ESP\_ERR\_INVALID\_ARG: I2C master transmit parameter invalid.
- ESP\_ERR\_TIMEOUT: Operation timeout(larger than xfer\_timeout\_ms) because the bus is busy or hardware crash.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_master\_transmit\_receive( i2c\_dev, const uint8\_t \*write\_buffer, size\_t write\_size,
uint8\_t \*read\_buffer, size\_t read\_size, int xfer\_timeout\_ms)
[](#_CPPv427i2c_master_transmit_receive23i2c_master_dev_handle_tPK7uint8_t6size_tP7uint8_t6size_ti "Permalink to
this definition")

Perform a write-read transaction on the I2C bus. The transaction will be undergoing until it finishes or it reaches the
timeout provided.

> [!note] Note
> If a callback was registered with `i2c_master_register_event_callbacks`, the transaction will be asynchronous, and
thus, this function will return directly, without blocking. You will get finish information from callback. Besides,
data buffer should always be completely prepared when callback is registered, otherwise, the data will get corrupt.

Parameters:

- **i2c\_dev** -- **\[in\]** I2C master device handle that created by `i2c_master_bus_add_device`.
- **write\_buffer** -- **\[in\]** Data bytes to send on the I2C bus.
- **write\_size** -- **\[in\]** Size, in bytes, of the write buffer.
- **read\_buffer** -- **\[out\]** Data bytes received from i2c bus.
- **read\_size** -- **\[in\]** Size, in bytes, of the read buffer.
- **xfer\_timeout\_ms** -- **\[in\]** Wait timeout, in ms. Note: -1 means wait forever.

Returns:

- ESP\_OK: I2C master transmit-receive success
- ESP\_ERR\_INVALID\_ARG: I2C master transmit parameter invalid.
- ESP\_ERR\_TIMEOUT: Operation timeout(larger than xfer\_timeout\_ms) because the bus is busy or hardware crash.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_master\_receive( i2c\_dev, uint8\_t \*read\_buffer, size\_t read\_size, int xfer\_timeout\_ms)
[](#_CPPv418i2c_master_receive23i2c_master_dev_handle_tP7uint8_t6size_ti "Permalink to this definition")

Perform a read transaction on the I2C bus. The transaction will be undergoing until it finishes or it reaches the
timeout provided.

> [!note] Note
> If a callback was registered with `i2c_master_register_event_callbacks`, the transaction will be asynchronous, and
thus, this function will return directly, without blocking. You will get finish information from callback. Besides,
data buffer should always be completely prepared when callback is registered, otherwise, the data will get corrupt.

Parameters:

- **i2c\_dev** -- **\[in\]** I2C master device handle that created by `i2c_master_bus_add_device`.
- **read\_buffer** -- **\[out\]** Data bytes received from i2c bus.
- **read\_size** -- **\[in\]** Size, in bytes, of the read buffer.
- **xfer\_timeout\_ms** -- **\[in\]** Wait timeout, in ms. Note: -1 means wait forever.

Returns:

- ESP\_OK: I2C master receive success
- ESP\_ERR\_INVALID\_ARG: I2C master receive parameter invalid.
- ESP\_ERR\_TIMEOUT: Operation timeout(larger than xfer\_timeout\_ms) because the bus is busy or hardware crash.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_master\_probe( bus\_handle, uint16\_t address, int xfer\_timeout\_ms)
[](#_CPPv416i2c_master_probe23i2c_master_bus_handle_t8uint16_ti "Permalink to this definition")

Probe I2C address, if address is correct and ACK is received, this function will return ESP\_OK.

**Attention**

Pull-ups must be connected to the SCL and SDA pins when this function is called. If you get `ESP_ERR_TIMEOUT while`
xfer\_timeout\_ms `was parsed correctly, you should check the pull-up resistors. If you do not have proper resistors
nearby. `flags.enable\_internal\_pullup\` is also acceptable.

> [!note] Note
> The principle of this function is to sent device address with a write command. If the device on your I2C bus, there
would be an ACK signal and function returns `ESP_OK`. If the device is not on your I2C bus, there would be a NACK
signal and function returns `ESP_ERR_NOT_FOUND`. `ESP_ERR_TIMEOUT` is not an expected failure, which indicated that the
i2c probe not works properly, usually caused by pull-up resistors not be connected properly. Suggestion check data on
SDA/SCL line to see whether there is ACK/NACK signal is on line when i2c probe function fails.

> [!note] Note
> There are lots of I2C devices all over the world, we assume that not all I2C device support the behavior like
`device_address+nack/ack`. So, if the on line data is strange and no ack/nack got respond. Please check the device
datasheet.

Parameters:

- **bus\_handle** -- **\[in\]** I2C master device handle that created by `i2c_master_bus_add_device`.
- **address** -- **\[in\]** I2C device address that you want to probe.
- **xfer\_timeout\_ms** -- **\[in\]** Wait timeout, in ms. Note: -1 means wait forever (Not recommended in this
function).

Returns:

- ESP\_OK: I2C device probe successfully
- ESP\_ERR\_NOT\_FOUND: I2C probe failed, doesn't find the device with specific address you gave.
- ESP\_ERR\_TIMEOUT: Operation timeout(larger than xfer\_timeout\_ms) because the bus is busy or hardware crash.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_master\_execute\_defined\_operations( i2c\_dev, \*i2c\_operation, size\_t
operation\_list\_num, int xfer\_timeout\_ms)
[](#_CPPv437i2c_master_execute_defined_operations23i2c_master_dev_handle_tP19i2c_operation_job_t6size_ti "Permalink
to this definition")

Execute a series of pre-defined I2C operations.

This function processes a list of I2C operations, such as start, write, read, and stop, according to the user-defined
`i2c_operation_job_t` array. It performs these operations sequentially on the specified I2C master device.

> [!note] Note
> The `ack_value` field in the READ operation must be set to `I2C_NACK_VAL` if the next operation is a STOP command.

Parameters:

- **i2c\_dev** -- **\[in\]** Handle to the I2C master device.
- **i2c\_operation** -- **\[in\]** Pointer to an array of user-defined I2C operation jobs. Each job specifies a command
and associated parameters.
- **operation\_list\_num** -- **\[in\]** The number of operations in the `i2c_operation` array.
- **xfer\_timeout\_ms** -- **\[in\]** Timeout for the transaction, in milliseconds.

Returns:

- ESP\_OK: Transaction completed successfully.
- ESP\_ERR\_INVALID\_ARG: One or more arguments are invalid.
- ESP\_ERR\_TIMEOUT: Transaction timed out.
- ESP\_FAIL: Other error during transaction.

Register I2C transaction callbacks for a master device.

> [!note] Note
> User can deregister a previously registered callback by calling this function and setting the callback member in the
`cbs` structure to NULL.

> [!note] Note
> When CONFIG\_I2C\_ISR\_IRAM\_SAFE is enabled, the callback itself and functions called by it should be placed in
IRAM. The variables used in the function should be in the SRAM as well. The `user_data` should also reside in SRAM.

> [!note] Note
> If the callback is used for helping asynchronous transaction. On the same bus, only one device can be used for
performing asynchronous operation.

Parameters:

- **i2c\_dev** -- **\[in\]** I2C master device handle that created by `i2c_master_bus_add_device`.
- **cbs** -- **\[in\]** Group of callback functions
- **user\_data** -- **\[in\]** User data, which will be passed to callback functions directly

Returns:

- ESP\_OK: Set I2C transaction callbacks successfully
- ESP\_ERR\_INVALID\_ARG: Set I2C transaction callbacks failed because of invalid argument
- ESP\_FAIL: Set I2C transaction callbacks failed because of other error

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_master\_bus\_reset( bus\_handle) [](#_CPPv420i2c_master_bus_reset23i2c_master_bus_handle_t
"Permalink to this definition")

Reset the I2C master bus.

Parameters:

**bus\_handle** -- I2C bus handle.

Returns:

- ESP\_OK: Reset succeed.
- ESP\_ERR\_INVALID\_ARG: I2C master bus handle is not initialized.
- Otherwise: Reset failed.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_master\_device\_change\_address( i2c\_dev, uint16\_t new\_device\_address, int timeout\_ms)
[](#_CPPv432i2c_master_device_change_address23i2c_master_dev_handle_t8uint16_ti "Permalink to this definition")

Change the I2C device address at runtime.

This function updates the device address of an existing I2C device handle. It is useful for devices that support
dynamic address assignment or when switching communication to a device with a different address on the same bus.

> [!note] Note
> - This function does not send commands to the I2C device. It only updates the address used in subsequent transactions
through the I2C handle.
> - Ensure that the new address is valid and does not conflict with other devices on the bus.

Parameters:

- **i2c\_dev** -- **\[in\]** I2C device handle.
- **new\_device\_address** -- **\[in\]** The new device address.
- **timeout\_ms** -- **\[in\]** Timeout for the address change operation, in milliseconds.

Returns:

- ESP\_OK: Address successfully changed.
- ESP\_ERR\_INVALID\_ARG: Invalid arguments (e.g., NULL handle or invalid address).
- ESP\_ERR\_TIMEOUT: Operation timed out.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_master\_bus\_wait\_all\_done( bus\_handle, int timeout\_ms)
[](#_CPPv428i2c_master_bus_wait_all_done23i2c_master_bus_handle_ti "Permalink to this definition")

Wait for all pending I2C transactions done.

Parameters:

- **bus\_handle** -- **\[in\]** I2C bus handle
- **timeout\_ms** -- **\[in\]** Wait timeout, in ms. Specially, -1 means to wait forever.

Returns:

- ESP\_OK: Flush transactions successfully
- ESP\_ERR\_INVALID\_ARG: Flush transactions failed because of invalid argument
- ESP\_ERR\_TIMEOUT: Flush transactions failed because of timeout
- ESP\_FAIL: Flush transactions failed because of other error

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_master\_get\_bus\_handle( port\_num, \*ret\_handle)
[](#_CPPv425i2c_master_get_bus_handle14i2c_port_num_tP23i2c_master_bus_handle_t "Permalink to this definition")

Retrieves the I2C master bus handle for a specified I2C port number.

This function retrieves the I2C master bus handle for the given I2C port number. Please make sure the handle has
already been initialized, and this function would simply returns the existing handle. Note that the returned handle
still can't be used concurrently

Parameters:

- **port\_num** -- I2C port number for which the handle is to be retrieved.
- **ret\_handle** -- Pointer to a variable where the retrieved handle will be stored.

Returns:

- ESP\_OK: Success. The handle is retrieved successfully.
- ESP\_ERR\_INVALID\_ARG: Invalid argument, such as invalid port number
- ESP\_ERR\_INVALID\_STATE: Invalid state, such as the I2C port is not initialized.

### Structures

struct i2c\_master\_bus\_config\_t [](#_CPPv423i2c_master_bus_config_t "Permalink to this definition")

I2C master bus specific configurations.

Public Members

i2c\_port [](#_CPPv4N23i2c_master_bus_config_t8i2c_portE "Permalink to this definition")

I2C port number, `-1` for auto selecting, (not include LP I2C instance)

gpio\_num\_t sda\_io\_num [](#_CPPv4N23i2c_master_bus_config_t10sda_io_numE "Permalink to this definition")

GPIO number of I2C SDA signal, pulled-up internally

gpio\_num\_t scl\_io\_num [](#_CPPv4N23i2c_master_bus_config_t10scl_io_numE "Permalink to this definition")

GPIO number of I2C SCL signal, pulled-up internally

clk\_source [](#_CPPv4N23i2c_master_bus_config_t10clk_sourceE "Permalink to this definition")

Clock source of I2C master bus

uint8\_t glitch\_ignore\_cnt [](#_CPPv4N23i2c_master_bus_config_t17glitch_ignore_cntE "Permalink to this definition")

If the glitch period on the line is less than this value, it can be filtered out, typically value is 7 (unit: I2C
module clock cycle)

int intr\_priority [](#_CPPv4N23i2c_master_bus_config_t13intr_priorityE "Permalink to this definition")

I2C interrupt priority, if set to 0, driver will select the default priority (1,2,3).

size\_t trans\_queue\_depth [](#_CPPv4N23i2c_master_bus_config_t17trans_queue_depthE "Permalink to this definition")

Depth of internal transfer queue, increase this value can support more transfers pending in the background, only valid
in asynchronous transaction. (Typically max\_device\_num \* per\_transaction)

uint32\_t enable\_internal\_pullup [](#_CPPv4N23i2c_master_bus_config_t22enable_internal_pullupE "Permalink to this
definition")

Enable internal pullups. Note: This is not strong enough to pullup buses under high-speed frequency. Recommend proper
external pull-up if possible

uint32\_t allow\_pd [](#_CPPv4N23i2c_master_bus_config_t8allow_pdE "Permalink to this definition")

If set, the driver will backup/restore the I2C registers before/after entering/exist sleep mode. By this approach, the
system can power off I2C's power domain. This can save power, but at the expense of more RAM being consumed

struct flags [](#_CPPv4N23i2c_master_bus_config_t5flagsE "Permalink to this definition")

I2C master config flags

struct i2c\_device\_config\_t [](#_CPPv419i2c_device_config_t "Permalink to this definition")

I2C device configuration.

Public Members

dev\_addr\_length [](#_CPPv4N19i2c_device_config_t15dev_addr_lengthE "Permalink to this definition")

Select the address length of the slave device.

uint16\_t device\_address [](#_CPPv4N19i2c_device_config_t14device_addressE "Permalink to this definition")

I2C device raw address. (The 7/10 bit address without read/write bit). Macro I2C\_DEVICE\_ADDRESS\_NOT\_USED (0xFFFF)
stands for skip the address config inside driver.

uint32\_t scl\_speed\_hz [](#_CPPv4N19i2c_device_config_t12scl_speed_hzE "Permalink to this definition")

I2C SCL line frequency.

uint32\_t scl\_wait\_us [](#_CPPv4N19i2c_device_config_t11scl_wait_usE "Permalink to this definition")

Timeout value. (unit: us). Please note this value should not be so small that it can handle stretch/disturbance
properly. If 0 is set, that means use the default reg value

uint32\_t disable\_ack\_check [](#_CPPv4N19i2c_device_config_t17disable_ack_checkE "Permalink to this definition")

Disable ACK check. If this is set false, that means ack check is enabled, the transaction will be stopped and API
returns error when nack is detected.

struct flags [](#_CPPv4N19i2c_device_config_t5flagsE "Permalink to this definition")

I2C device config flags

struct i2c\_operation\_job\_t [](#_CPPv419i2c_operation_job_t "Permalink to this definition")

Structure representing an I2C operation job.

This structure is used to define individual I2C operations (write or read) within a sequence of I2C master transactions.

Public Members

command [](#_CPPv4N19i2c_operation_job_t7commandE "Permalink to this definition")

I2C command indicating the type of operation (START, WRITE, READ, or STOP)

bool ack\_check [](#_CPPv4N19i2c_operation_job_t9ack_checkE "Permalink to this definition")

Whether to enable ACK check during WRITE operation

uint8\_t \*data [](#_CPPv4N19i2c_operation_job_t4dataE "Permalink to this definition")

Pointer to the data to be written

Pointer to the buffer for storing the data read from the bus

size\_t total\_bytes [](#_CPPv4N19i2c_operation_job_t11total_bytesE "Permalink to this definition")

Total number of bytes to write

Total number of bytes to read

struct write [](#_CPPv4N19i2c_operation_job_t5writeE "Permalink to this definition")

Structure for WRITE command.

Used when the `command` is set to `I2C_MASTER_CMD_WRITE`.

ack\_value [](#_CPPv4N19i2c_operation_job_t9ack_valueE "Permalink to this definition")

ACK value to send after the read (ACK or NACK)

struct read [](#_CPPv4N19i2c_operation_job_t4readE "Permalink to this definition")

Structure for READ command.

Used when the `command` is set to `I2C_MASTER_CMD_READ`.

struct i2c\_master\_transmit\_multi\_buffer\_info\_t [](#_CPPv439i2c_master_transmit_multi_buffer_info_t "Permalink
to this definition")

I2C master transmit buffer information structure.

Public Members

const uint8\_t \*write\_buffer [](#_CPPv4N39i2c_master_transmit_multi_buffer_info_t12write_bufferE "Permalink to
this definition")

Pointer to buffer to be written.

size\_t buffer\_size [](#_CPPv4N39i2c_master_transmit_multi_buffer_info_t11buffer_sizeE "Permalink to this
definition")

Size of data to be written.

struct i2c\_master\_event\_callbacks\_t [](#_CPPv428i2c_master_event_callbacks_t "Permalink to this definition")

Group of I2C master callbacks, can be used to get status during transaction or doing other small things. But take care
potential concurrency issues.

> [!note] Note
> The callbacks are all running under ISR context

> [!note] Note
> When CONFIG\_I2C\_ISR\_IRAM\_SAFE is enabled, the callback itself and functions called by it should be placed in
IRAM. The variables used in the function should be in the SRAM as well.

Public Members

on\_trans\_done [](#_CPPv4N28i2c_master_event_callbacks_t13on_trans_doneE "Permalink to this definition")

I2C master transaction finish callback

### Macros

I2C\_DEVICE\_ADDRESS\_NOT\_USED [](#c.I2C_DEVICE_ADDRESS_NOT_USED "Permalink to this definition")

Skip carry address bit in driver transmit and receive

### Header File

-
[components/esp\_driver\_i2c/include/driver/i2c\_slave.h](https://github.com/espressif/esp-idf/blob/v5.5.4/components/es
p_driver_i2c/include/driver/i2c_slave.h)
- This header file can be included with:
	> ```c
	> #include "driver/i2c_slave.h"
	> ```
- This header file is a part of the API provided by the `esp_driver_i2c` component. To declare that your component
depends on `esp_driver_i2c`, add the following to your CMakeLists.txt:
	> ```
	> REQUIRES esp_driver_i2c
	> ```
	>
	> or
	>
	> ```
	> PRIV_REQUIRES esp_driver_i2c
	> ```

### Functions

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_slave\_receive( i2c\_slave, uint8\_t \*data, size\_t buffer\_size)
[](#_CPPv417i2c_slave_receive22i2c_slave_dev_handle_tP7uint8_t6size_t "Permalink to this definition")

Read bytes from I2C internal buffer. Start a job to receive I2C data.

> [!note] Note
> This function is non-blocking, it initiates a new receive job and then returns. User should check the received data
from the `on_recv_done` callback that registered by `i2c_slave_register_event_callbacks()`.

Parameters:

- **i2c\_slave** -- **\[in\]** I2C slave device handle that created by `i2c_new_slave_device`.
- **data** -- **\[out\]** Buffer to store data from I2C fifo. Should be valid until `on_recv_done` is triggered.
- **buffer\_size** -- **\[in\]** Buffer size of data that provided by users.

Returns:

- ESP\_OK: I2C slave receive success.
- ESP\_ERR\_INVALID\_ARG: I2C slave receive parameter invalid.
- ESP\_ERR\_NOT\_SUPPORTED: This function should be work in fifo mode, but I2C\_SLAVE\_NONFIFO mode is configured

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_slave\_transmit( i2c\_slave, const uint8\_t \*data, int size, int xfer\_timeout\_ms)
[](#_CPPv418i2c_slave_transmit22i2c_slave_dev_handle_tPK7uint8_tii "Permalink to this definition")

Write bytes to internal ringbuffer of the I2C slave data. When the TX fifo empty, the ISR will fill the hardware FIFO
with the internal ringbuffer's data.

> [!note] Note
> If you connect this slave device to some master device, the data transaction direction is from slave device to master
device.

Parameters:

- **i2c\_slave** -- **\[in\]** I2C slave device handle that created by `i2c_new_slave_device`.
- **data** -- **\[in\]** Buffer to write to slave fifo, can pickup by master. Can be freed after this function returns.
Equal or larger than `size`.
- **size** -- **\[in\]** In bytes, of `data` buffer.
- **xfer\_timeout\_ms** -- **\[in\]** Wait timeout, in ms. Note: -1 means wait forever.

Returns:

- ESP\_OK: I2C slave transmit success.
- ESP\_ERR\_INVALID\_ARG: I2C slave transmit parameter invalid.
- ESP\_ERR\_TIMEOUT: Operation timeout(larger than xfer\_timeout\_ms) because the device is busy or hardware crash.
- ESP\_ERR\_NOT\_SUPPORTED: This function should be work in fifo mode, but I2C\_SLAVE\_NONFIFO mode is configured

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_slave\_read\_ram( i2c\_slave, uint8\_t ram\_address, uint8\_t \*data, size\_t receive\_size)
[](#_CPPv418i2c_slave_read_ram22i2c_slave_dev_handle_t7uint8_tP7uint8_t6size_t "Permalink to this definition")

Read bytes from I2C internal ram. This can be only used when `access_ram_en` in configuration structure set to true.

Parameters:

- **i2c\_slave** -- **\[in\]** I2C slave device handle that created by `i2c_new_slave_device`.
- **ram\_address** -- **\[in\]** The offset of RAM (Cannot larger than I2C RAM memory)
- **data** -- **\[out\]** Buffer to store data read from I2C ram.
- **receive\_size** -- **\[in\]** Received size from RAM.

Returns:

- ESP\_OK: I2C slave transmit success.
- ESP\_ERR\_INVALID\_ARG: I2C slave transmit parameter invalid.
- ESP\_ERR\_NOT\_SUPPORTED: This function should be work in non-fifo mode, but I2C\_SLAVE\_FIFO mode is configured

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_slave\_write\_ram( i2c\_slave, uint8\_t ram\_address, const uint8\_t \*data, size\_t size)
[](#_CPPv419i2c_slave_write_ram22i2c_slave_dev_handle_t7uint8_tPK7uint8_t6size_t "Permalink to this definition")

Write bytes to I2C internal ram. This can be only used when `access_ram_en` in configuration structure set to true.

Parameters:

- **i2c\_slave** -- **\[in\]** I2C slave device handle that created by `i2c_new_slave_device`.
- **ram\_address** -- **\[in\]** The offset of RAM (Cannot larger than I2C RAM memory)
- **data** -- **\[in\]** Buffer to fill.
- **size** -- **\[in\]** Received size from RAM.

Returns:

- ESP\_OK: I2C slave transmit success.
- ESP\_ERR\_INVALID\_ARG: I2C slave transmit parameter invalid.
- ESP\_ERR\_INVALID\_SIZE: Write size is larger than
- ESP\_ERR\_NOT\_SUPPORTED: This function should be work in non-fifo mode, but I2C\_SLAVE\_FIFO mode is configured

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_new\_slave\_device(const \*slave\_config, \*ret\_handle)
[](#_CPPv420i2c_new_slave_devicePK18i2c_slave_config_tP22i2c_slave_dev_handle_t "Permalink to this definition")

Initialize an I2C slave device.

Parameters:

- **slave\_config** -- **\[in\]** I2C slave device configurations
- **ret\_handle** -- **\[out\]** Return a generic I2C device handle

Returns:

- ESP\_OK: I2C slave device initialized successfully
- ESP\_ERR\_INVALID\_ARG: I2C device initialization failed because of invalid argument.
- ESP\_ERR\_NO\_MEM: Create I2C device failed because of out of memory.

Set I2C slave event callbacks for I2C slave channel.

> [!note] Note
> User can deregister a previously registered callback by calling this function and setting the callback member in the
`cbs` structure to NULL.

> [!note] Note
> When CONFIG\_I2C\_ISR\_IRAM\_SAFE is enabled, the callback itself and functions called by it should be placed in
IRAM. The variables used in the function should be in the SRAM as well. The `user_data` should also reside in SRAM.

Parameters:

- **i2c\_slave** -- **\[in\]** I2C slave device handle that created by `i2c_new_slave_device`.
- **cbs** -- **\[in\]** Group of callback functions
- **user\_data** -- **\[in\]** User data, which will be passed to callback functions directly

Returns:

- ESP\_OK: Set I2C transaction callbacks successfully
- ESP\_ERR\_INVALID\_ARG: Set I2C transaction callbacks failed because of invalid argument
- ESP\_FAIL: Set I2C transaction callbacks failed because of other error

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/esp_err.html#_CPPv49esp
_err_t "esp_err_t") i2c\_del\_slave\_device( i2c\_slave) [](#_CPPv420i2c_del_slave_device22i2c_slave_dev_handle_t
"Permalink to this definition")

Deinitialize the I2C slave device.

Parameters:

**i2c\_slave** -- **\[in\]** I2C slave device handle that created by `i2c_new_slave_device`.

Returns:

- ESP\_OK: Delete I2C device successfully.
- ESP\_ERR\_INVALID\_ARG: I2C device initialization failed because of invalid argument.

### Structures

struct i2c\_slave\_config\_t [](#_CPPv418i2c_slave_config_t "Permalink to this definition")

I2C slave specific configurations.

Public Members

i2c\_port [](#_CPPv4N18i2c_slave_config_t8i2c_portE "Permalink to this definition")

I2C port number, `-1` for auto selecting

gpio\_num\_t sda\_io\_num [](#_CPPv4N18i2c_slave_config_t10sda_io_numE "Permalink to this definition")

SDA IO number used by I2C bus

gpio\_num\_t scl\_io\_num [](#_CPPv4N18i2c_slave_config_t10scl_io_numE "Permalink to this definition")

SCL IO number used by I2C bus

clk\_source [](#_CPPv4N18i2c_slave_config_t10clk_sourceE "Permalink to this definition")

Clock source of I2C bus.

uint32\_t send\_buf\_depth [](#_CPPv4N18i2c_slave_config_t14send_buf_depthE "Permalink to this definition")

Depth of internal transfer ringbuffer, increase this value can support more transfers pending in the background

uint16\_t slave\_addr [](#_CPPv4N18i2c_slave_config_t10slave_addrE "Permalink to this definition")

I2C slave address

addr\_bit\_len [](#_CPPv4N18i2c_slave_config_t12addr_bit_lenE "Permalink to this definition")

I2C slave address in bit length

int intr\_priority [](#_CPPv4N18i2c_slave_config_t13intr_priorityE "Permalink to this definition")

I2C interrupt priority, if set to 0, driver will select the default priority (1,2,3).

uint32\_t stretch\_en [](#_CPPv4N18i2c_slave_config_t10stretch_enE "Permalink to this definition")

Enable slave stretch

uint32\_t broadcast\_en [](#_CPPv4N18i2c_slave_config_t12broadcast_enE "Permalink to this definition")

I2C slave enable broadcast

uint32\_t access\_ram\_en [](#_CPPv4N18i2c_slave_config_t13access_ram_enE "Permalink to this definition")

Can get access to I2C RAM directly

uint32\_t allow\_pd [](#_CPPv4N18i2c_slave_config_t8allow_pdE "Permalink to this definition")

If set, the driver will backup/restore the I2C registers before/after entering/exist sleep mode. By this approach, the
system can power off I2C's power domain. This can save power, but at the expense of more RAM being consumed

struct flags [](#_CPPv4N18i2c_slave_config_t5flagsE "Permalink to this definition")

I2C slave config flags

struct i2c\_slave\_event\_callbacks\_t [](#_CPPv427i2c_slave_event_callbacks_t "Permalink to this definition")

Group of I2C slave callbacks (e.g. get i2c slave stretch cause). But take care of potential concurrency issues.

> [!note] Note
> The callbacks are all running under ISR context

> [!note] Note
> When CONFIG\_I2C\_ISR\_IRAM\_SAFE is enabled, the callback itself and functions called by it should be placed in
IRAM. The variables used in the function should be in the SRAM as well.

Public Members

on\_stretch\_occur [](#_CPPv4N27i2c_slave_event_callbacks_t16on_stretch_occurE "Permalink to this definition")

I2C slave stretched callback

on\_recv\_done [](#_CPPv4N27i2c_slave_event_callbacks_t12on_recv_doneE "Permalink to this definition")

I2C slave receive done callback

### Header File

-
[components/esp\_driver\_i2c/include/driver/i2c\_types.h](https://github.com/espressif/esp-idf/blob/v5.5.4/components/es
p_driver_i2c/include/driver/i2c_types.h)
- This header file can be included with:
	> ```c
	> #include "driver/i2c_types.h"
	> ```
- This header file is a part of the API provided by the `esp_driver_i2c` component. To declare that your component
depends on `esp_driver_i2c`, add the following to your CMakeLists.txt:
	> ```
	> REQUIRES esp_driver_i2c
	> ```
	>
	> or
	>
	> ```
	> PRIV_REQUIRES esp_driver_i2c
	> ```

### Structures

struct i2c\_master\_event\_data\_t [](#_CPPv423i2c_master_event_data_t "Permalink to this definition")

Data type used in I2C event callback.

Public Members

event [](#_CPPv4N23i2c_master_event_data_t5eventE "Permalink to this definition")

The I2C hardware event that I2C callback is called.

struct i2c\_slave\_rx\_done\_event\_data\_t [](#_CPPv430i2c_slave_rx_done_event_data_t "Permalink to this
definition")

Event structure used in I2C slave.

Public Members

uint8\_t \*buffer [](#_CPPv4N30i2c_slave_rx_done_event_data_t6bufferE "Permalink to this definition")

Pointer for buffer received in callback.

struct i2c\_slave\_stretch\_event\_data\_t [](#_CPPv430i2c_slave_stretch_event_data_t "Permalink to this definition")

Stretch cause event structure used in I2C slave.

Public Members

stretch\_cause [](#_CPPv4N30i2c_slave_stretch_event_data_t13stretch_causeE "Permalink to this definition")

Stretch cause can be got in callback

struct i2c\_slave\_request\_event\_data\_t [](#_CPPv430i2c_slave_request_event_data_t "Permalink to this definition")

Event structure used in I2C slave request.

### Type Definitions

typedef int i2c\_port\_num\_t [](#_CPPv414i2c_port_num_t "Permalink to this definition")

I2C port number.

typedef struct i2c\_master\_bus\_t \*i2c\_master\_bus\_handle\_t [](#_CPPv423i2c_master_bus_handle_t "Permalink to
this definition")

Type of I2C master bus handle.

typedef struct i2c\_master\_dev\_t \*i2c\_master\_dev\_handle\_t [](#_CPPv423i2c_master_dev_handle_t "Permalink to
this definition")

Type of I2C master bus device handle.

typedef struct i2c\_slave\_dev\_t \*i2c\_slave\_dev\_handle\_t [](#_CPPv422i2c_slave_dev_handle_t "Permalink to this
definition")

Type of I2C slave device handle.

typedef bool (\*i2c\_master\_callback\_t)( i2c\_dev, const \*evt\_data, void \*arg)
[](#_CPPv421i2c_master_callback_t "Permalink to this definition")

An callback for I2C transaction.

Param i2c\_dev:

**\[in\]** Handle for I2C device.

Param evt\_data:

**\[out\]** I2C capture event data, fed by driver

Param arg:

**\[in\]** User data, set in `i2c_master_register_event_callbacks()`

Return:

Whether a high priority task has been waken up by this function

typedef bool (\*i2c\_slave\_received\_callback\_t)( i2c\_slave, const \*evt\_data, void \*arg)
[](#_CPPv429i2c_slave_received_callback_t "Permalink to this definition")

Callback signature for I2C slave.

Param i2c\_slave:

**\[in\]** Handle for I2C slave.

Param evt\_data:

**\[out\]** I2C capture event data, fed by driver

Param arg:

**\[in\]** User data, set in `i2c_slave_register_event_callbacks()`

Return:

Whether a high priority task has been waken up by this function

typedef bool (\*i2c\_slave\_stretch\_callback\_t)( i2c\_slave, const \*evt\_cause, void \*arg)
[](#_CPPv428i2c_slave_stretch_callback_t "Permalink to this definition")

Callback signature for I2C slave stretch.

Param i2c\_slave:

**\[in\]** Handle for I2C slave.

Param evt\_cause:

**\[out\]** I2C capture event cause, fed by driver

Param arg:

**\[in\]** User data, set in `i2c_slave_register_event_callbacks()`

Return:

Whether a high priority task has been waken up by this function

typedef bool (\*i2c\_slave\_request\_callback\_t)( i2c\_slave, const \*evt\_data, void \*arg)
[](#_CPPv428i2c_slave_request_callback_t "Permalink to this definition")

Callback signature for I2C slave request event. When this callback is triggered that means master want to read data
from slave while there is no data in slave fifo. So user should write data to fifo via `i2c_slave_write`

Param i2c\_slave:

**\[in\]** Handle for I2C slave.

Param evt\_data:

**\[out\]** I2C receive event data, fed by driver

Param arg:

**\[in\]** User data, set in `i2c_slave_register_event_callbacks()`

Return:

Whether a high priority task has been waken up by this function

### Enumerations

enum i2c\_master\_status\_t [](#_CPPv419i2c_master_status_t "Permalink to this definition")

Enumeration for I2C fsm status.

*Values:*

enumerator I2C\_STATUS\_READ [](#_CPPv4N19i2c_master_status_t15I2C_STATUS_READE "Permalink to this definition")

read status for current master command, but just partial read, not all data is read is this status

enumerator I2C\_STATUS\_READ\_ALL [](#_CPPv4N19i2c_master_status_t19I2C_STATUS_READ_ALLE "Permalink to this
definition")

read status for current master command, all data is read is this status

enumerator I2C\_STATUS\_WRITE [](#_CPPv4N19i2c_master_status_t16I2C_STATUS_WRITEE "Permalink to this definition")

write status for current master command

enumerator I2C\_STATUS\_START [](#_CPPv4N19i2c_master_status_t16I2C_STATUS_STARTE "Permalink to this definition")

Start status for current master command

enumerator I2C\_STATUS\_STOP [](#_CPPv4N19i2c_master_status_t15I2C_STATUS_STOPE "Permalink to this definition")

stop status for current master command

enumerator I2C\_STATUS\_IDLE [](#_CPPv4N19i2c_master_status_t15I2C_STATUS_IDLEE "Permalink to this definition")

idle status for current master command

enumerator I2C\_STATUS\_ACK\_ERROR [](#_CPPv4N19i2c_master_status_t20I2C_STATUS_ACK_ERRORE "Permalink to this
definition")

ack error status for current master command

enumerator I2C\_STATUS\_DONE [](#_CPPv4N19i2c_master_status_t15I2C_STATUS_DONEE "Permalink to this definition")

I2C command done

enumerator I2C\_STATUS\_TIMEOUT [](#_CPPv4N19i2c_master_status_t18I2C_STATUS_TIMEOUTE "Permalink to this definition")

I2C bus status error, and operation timeout

enum i2c\_master\_event\_t [](#_CPPv418i2c_master_event_t "Permalink to this definition")

Enumeration for I2C event.

*Values:*

enumerator I2C\_EVENT\_ALIVE [](#_CPPv4N18i2c_master_event_t15I2C_EVENT_ALIVEE "Permalink to this definition")

i2c bus in alive status.

enumerator I2C\_EVENT\_DONE [](#_CPPv4N18i2c_master_event_t14I2C_EVENT_DONEE "Permalink to this definition")

i2c bus transaction done

enumerator I2C\_EVENT\_NACK [](#_CPPv4N18i2c_master_event_t14I2C_EVENT_NACKE "Permalink to this definition")

i2c bus nack

enumerator I2C\_EVENT\_TIMEOUT [](#_CPPv4N18i2c_master_event_t17I2C_EVENT_TIMEOUTE "Permalink to this definition")

i2c bus timeout

enum i2c\_master\_command\_t [](#_CPPv420i2c_master_command_t "Permalink to this definition")

Enum for I2C master commands.

These commands are used to define the I2C master operations. They correspond to hardware-level commands supported by
the I2C peripheral.

*Values:*

enumerator I2C\_MASTER\_CMD\_START [](#_CPPv4N20i2c_master_command_t20I2C_MASTER_CMD_STARTE "Permalink to this
definition")

Start or Restart condition

enumerator I2C\_MASTER\_CMD\_WRITE [](#_CPPv4N20i2c_master_command_t20I2C_MASTER_CMD_WRITEE "Permalink to this
definition")

Write operation

enumerator I2C\_MASTER\_CMD\_READ [](#_CPPv4N20i2c_master_command_t19I2C_MASTER_CMD_READE "Permalink to this
definition")

Read operation

enumerator I2C\_MASTER\_CMD\_STOP [](#_CPPv4N20i2c_master_command_t19I2C_MASTER_CMD_STOPE "Permalink to this
definition")

Stop condition

enum i2c\_ack\_value\_t [](#_CPPv415i2c_ack_value_t "Permalink to this definition")

Enum for I2C master ACK values.

These values define the acknowledgment (ACK) behavior during read operations.

*Values:*

enumerator I2C\_ACK\_VAL [](#_CPPv4N15i2c_ack_value_t11I2C_ACK_VALE "Permalink to this definition")

Acknowledge (ACK) signal

enumerator I2C\_NACK\_VAL [](#_CPPv4N15i2c_ack_value_t12I2C_NACK_VALE "Permalink to this definition")

Not Acknowledge (NACK) signal

### Header File

-
[components/hal/include/hal/i2c\_types.h](https://github.com/espressif/esp-idf/blob/v5.5.4/components/hal/include/hal/i2
c_types.h)
- This header file can be included with:
	> ```c
	> #include "hal/i2c_types.h"
	> ```

### Structures

struct i2c\_hal\_clk\_config\_t [](#_CPPv420i2c_hal_clk_config_t "Permalink to this definition")

Data structure for calculating I2C bus timing.

Public Members

uint16\_t clkm\_div [](#_CPPv4N20i2c_hal_clk_config_t8clkm_divE "Permalink to this definition")

I2C core clock divider

uint16\_t scl\_low [](#_CPPv4N20i2c_hal_clk_config_t7scl_lowE "Permalink to this definition")

I2C scl low period

uint16\_t scl\_high [](#_CPPv4N20i2c_hal_clk_config_t8scl_highE "Permalink to this definition")

I2C scl high period

uint16\_t scl\_wait\_high [](#_CPPv4N20i2c_hal_clk_config_t13scl_wait_highE "Permalink to this definition")

I2C scl wait\_high period

uint16\_t sda\_hold [](#_CPPv4N20i2c_hal_clk_config_t8sda_holdE "Permalink to this definition")

I2C scl low period

uint16\_t sda\_sample [](#_CPPv4N20i2c_hal_clk_config_t10sda_sampleE "Permalink to this definition")

I2C sda sample time

uint16\_t setup [](#_CPPv4N20i2c_hal_clk_config_t5setupE "Permalink to this definition")

I2C start and stop condition setup period

uint16\_t hold [](#_CPPv4N20i2c_hal_clk_config_t4holdE "Permalink to this definition")

I2C start and stop condition hold period

uint16\_t tout [](#_CPPv4N20i2c_hal_clk_config_t4toutE "Permalink to this definition")

I2C bus timeout period

### Type Definitions

typedef
[soc\_periph\_i2c\_clk\_src\_t](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/
clk_tree.html#_CPPv424soc_periph_i2c_clk_src_t "soc_periph_i2c_clk_src_t") i2c\_clock\_source\_t
[](#_CPPv418i2c_clock_source_t "Permalink to this definition")

I2C group clock source.

### Enumerations

enum i2c\_port\_t [](#_CPPv410i2c_port_t "Permalink to this definition")

I2C port number, can be I2C\_NUM\_0 ~ (I2C\_NUM\_MAX-1).

*Values:*

enumerator I2C\_NUM\_0 [](#_CPPv4N10i2c_port_t9I2C_NUM_0E "Permalink to this definition")

I2C port 0

enumerator I2C\_NUM\_1 [](#_CPPv4N10i2c_port_t9I2C_NUM_1E "Permalink to this definition")

I2C port 1

enumerator I2C\_NUM\_MAX [](#_CPPv4N10i2c_port_t11I2C_NUM_MAXE "Permalink to this definition")

I2C port max

enum i2c\_addr\_bit\_len\_t [](#_CPPv418i2c_addr_bit_len_t "Permalink to this definition")

Enumeration for I2C device address bit length.

*Values:*

enumerator I2C\_ADDR\_BIT\_LEN\_7 [](#_CPPv4N18i2c_addr_bit_len_t18I2C_ADDR_BIT_LEN_7E "Permalink to this
definition")

i2c address bit length 7

enumerator I2C\_ADDR\_BIT\_LEN\_10 [](#_CPPv4N18i2c_addr_bit_len_t19I2C_ADDR_BIT_LEN_10E "Permalink to this
definition")

i2c address bit length 10

enum i2c\_mode\_t [](#_CPPv410i2c_mode_t "Permalink to this definition")

*Values:*

enumerator I2C\_MODE\_SLAVE [](#_CPPv4N10i2c_mode_t14I2C_MODE_SLAVEE "Permalink to this definition")

I2C slave mode

enumerator I2C\_MODE\_MASTER [](#_CPPv4N10i2c_mode_t15I2C_MODE_MASTERE "Permalink to this definition")

I2C master mode

enumerator I2C\_MODE\_MAX [](#_CPPv4N10i2c_mode_t12I2C_MODE_MAXE "Permalink to this definition")

enum i2c\_rw\_t [](#_CPPv48i2c_rw_t "Permalink to this definition")

*Values:*

enumerator I2C\_MASTER\_WRITE [](#_CPPv4N8i2c_rw_t16I2C_MASTER_WRITEE "Permalink to this definition")

I2C write data

enumerator I2C\_MASTER\_READ [](#_CPPv4N8i2c_rw_t15I2C_MASTER_READE "Permalink to this definition")

I2C read data

enum i2c\_trans\_mode\_t [](#_CPPv416i2c_trans_mode_t "Permalink to this definition")

*Values:*

enumerator I2C\_DATA\_MODE\_MSB\_FIRST [](#_CPPv4N16i2c_trans_mode_t23I2C_DATA_MODE_MSB_FIRSTE "Permalink to this
definition")

I2C data msb first

enumerator I2C\_DATA\_MODE\_LSB\_FIRST [](#_CPPv4N16i2c_trans_mode_t23I2C_DATA_MODE_LSB_FIRSTE "Permalink to this
definition")

I2C data lsb first

enumerator I2C\_DATA\_MODE\_MAX [](#_CPPv4N16i2c_trans_mode_t17I2C_DATA_MODE_MAXE "Permalink to this definition")

enum i2c\_addr\_mode\_t [](#_CPPv415i2c_addr_mode_t "Permalink to this definition")

*Values:*

enumerator I2C\_ADDR\_BIT\_7 [](#_CPPv4N15i2c_addr_mode_t14I2C_ADDR_BIT_7E "Permalink to this definition")

I2C 7bit address for slave mode

enumerator I2C\_ADDR\_BIT\_10 [](#_CPPv4N15i2c_addr_mode_t15I2C_ADDR_BIT_10E "Permalink to this definition")

I2C 10bit address for slave mode

enumerator I2C\_ADDR\_BIT\_MAX [](#_CPPv4N15i2c_addr_mode_t16I2C_ADDR_BIT_MAXE "Permalink to this definition")

enum i2c\_ack\_type\_t [](#_CPPv414i2c_ack_type_t "Permalink to this definition")

*Values:*

enumerator I2C\_MASTER\_ACK [](#_CPPv4N14i2c_ack_type_t14I2C_MASTER_ACKE "Permalink to this definition")

I2C ack for each byte read

enumerator I2C\_MASTER\_NACK [](#_CPPv4N14i2c_ack_type_t15I2C_MASTER_NACKE "Permalink to this definition")

I2C nack for each byte read

enumerator I2C\_MASTER\_LAST\_NACK [](#_CPPv4N14i2c_ack_type_t20I2C_MASTER_LAST_NACKE "Permalink to this definition")

I2C nack for the last byte

enumerator I2C\_MASTER\_ACK\_MAX [](#_CPPv4N14i2c_ack_type_t18I2C_MASTER_ACK_MAXE "Permalink to this definition")

enum i2c\_slave\_stretch\_cause\_t [](#_CPPv425i2c_slave_stretch_cause_t "Permalink to this definition")

Enum for I2C slave stretch causes.

*Values:*

enumerator I2C\_SLAVE\_STRETCH\_CAUSE\_ADDRESS\_MATCH
[](#_CPPv4N25i2c_slave_stretch_cause_t37I2C_SLAVE_STRETCH_CAUSE_ADDRESS_MATCHE "Permalink to this definition")

Stretching SCL low when the slave is read by the master and the address just matched

enumerator I2C\_SLAVE\_STRETCH\_CAUSE\_TX\_EMPTY
[](#_CPPv4N25i2c_slave_stretch_cause_t32I2C_SLAVE_STRETCH_CAUSE_TX_EMPTYE "Permalink to this definition")

Stretching SCL low when TX FIFO is empty in slave mode

enumerator I2C\_SLAVE\_STRETCH\_CAUSE\_RX\_FULL
[](#_CPPv4N25i2c_slave_stretch_cause_t31I2C_SLAVE_STRETCH_CAUSE_RX_FULLE "Permalink to this definition")

Stretching SCL low when RX FIFO is full in slave mode

enum i2c\_bus\_mode\_t [](#_CPPv414i2c_bus_mode_t "Permalink to this definition")

Enum for i2c working modes.

*Values:*

enumerator I2C\_BUS\_MODE\_MASTER [](#_CPPv4N14i2c_bus_mode_t19I2C_BUS_MODE_MASTERE "Permalink to this definition")

I2C works under master mode
