---
Title: Source - esp32-p4-i2s
Ticket: ESP32-P4-PICOCALC-AUDIO
Status: active
Topics:
    - esp32-p4
    - picocalc
DocType: source
Intent: reference
Summary: "Downloaded reference material for ESP32-P4-PICOCALC-AUDIO"
---

## Inter-IC Sound (I2S)

[\[中文\]](https://docs.espressif.com/projects/esp-idf/zh_CN/v6.0.1/esp32p4/api-reference/peripherals/i2s.html)

## Introduction

I2S (Inter-IC Sound) is a synchronous serial communication protocol usually used for transmitting audio data between two digital audio devices.

Note

For LP I2S documentation, see [Low Power Inter-IC Sound](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lp_i2s.html).

ESP32-P4 contains three I2S peripheral(s). These peripherals can be configured to input and output sample data via the I2S driver.

An I2S bus that communicates in standard and TDM mode consists of the following lines:

- **MCLK:** Master clock line. It is an optional signal depending on the slave side, mainly used for offering a reference clock to the I2S slave device.
- **BCLK:** Bit clock line. The bit clock for data line.
- **WS:** Word (Slot) select line. It is usually used to identify the vocal tract except PDM mode.
- **DIN/DOUT:** Serial data input/output line. Data will loopback internally if DIN and DOUT are set to a same GPIO.

An I2S bus that communicates in PDM mode consists of the following lines:

- **CLK:** PDM clock line.
- **DIN/DOUT:** Serial data input/output line.

Each I2S controller has the following features that can be configured by the I2S driver:

- Operation as system master or slave
- Capable of acting as transmitter or receiver
- DMA controller that allows stream sampling of data without requiring the CPU to copy each data sample

Each controller has separate RX and TX channels. That means they are able to work under different clocks and slot configurations with separate GPIO pins. Note that although the internal MCLKs of TX channel and RX channel are separate on a controller, the output MCLK signal can only be attached to one channel. If independent MCLK output is required for each channel, they must be allocated on different I2S controllers.

## I2S Clock

### Clock Source

- `i2s_clock_src_t::I2S_CLK_SRC_DEFAULT`: Default PLL clock.
- `i2s_clock_src_t::I2S_CLK_SRC_APLL`: Audio PLL clock, which is more precise than `I2S_CLK_SRC_PLL_160M` in high sample rate applications. Its frequency is configurable according to the sample rate. However, if APLL has been occupied by EMAC or other channels, the APLL frequency cannot be changed, and the driver will try to work under this APLL frequency. If this frequency cannot meet the requirements of I2S, the clock configuration will fail.

### Clock Terminology

- **Sample rate**: The number of sampled data in one second per slot.
- **SCLK**: Source clock frequency. It is the frequency of the clock source.
- **MCLK**: Master clock frequency. BCLK is generated from this clock. The MCLK signal usually serves as a reference clock and is mostly needed to synchronize BCLK and WS between I2S master and slave roles.
- **BCLK**: Bit clock frequency. Every tick of this clock stands for one data bit on data pin. The slot bit width configured in is equal to the number of BCLK ticks, which means there will be 8/16/24/32 BCLK ticks in one slot.
- **LRCK** / **WS**: Left/right clock or word select clock. For non-PDM mode, its frequency is equal to the sample rate.

Note

Normally, MCLK should be the multiple of `sample rate` and BCLK at the same time. The field indicates the multiple of MCLK to the `sample rate`. In most cases, `I2S_MCLK_MULTIPLE_256` should be enough. However, if `slot_bit_width` is set to `I2S_SLOT_BIT_WIDTH_24BIT`, to keep MCLK a multiple to the BCLK, should be set to multiples that are divisible by 3 such as `I2S_MCLK_MULTIPLE_384`. Otherwise, WS will be inaccurate.

## I2S Communication Mode

### Overview of All Modes

| Target | Standard | PCM-to-PDM | PDM-to-PCM | PDM | TDM | ADC/DAC | LCD/Camera |
| --- | --- | --- | --- | --- | --- | --- | --- |
| ESP32 | I2S 0/1 | I2S 0 | I2S 0 | I2S 0/1 | none | I2S 0 | I2S 0 |
| ESP32-S2 | I2S 0 | none | none | none | none | none | I2S 0 |
| ESP32-C3 | I2S 0 | I2S 0 | none | I2S 0 | I2S 0 | none | none |
| ESP32-C6 | I2S 0 | I2S 0 | none | I2S 0 | I2S 0 | none | none |
| ESP32-S3 | I2S 0/1 | I2S 0 | I2S 0 | I2S 0/1 | I2S 0/1 | none | none |
| ESP32-H2 | I2S 0 | I2S 0 | none | I2S 0 | I2S 0 | none | none |
| ESP32-P4 | I2S 0~2 | I2S 0 | I2S 0 | I2S 0~2 | I2S 0~2 | none | none |
| ESP32-C5 | I2S 0 | I2S 0 | none | I2S 0 | I2S 0 | none | none |
| ESP32-C61 | I2S 0 | I2S 0 | none | I2S 0 | I2S 0 | none | none |

Note

If you are using PDM mode, note that not all I2S ports support conversion between raw PDM and PCM formats, because these ports do not have PCM-to-PDM data format converter in TX direction, or PDM-to-PCM data format converter in RX direction. Ports without the converter can only read/write raw PDM data. To read/write PCM format data on these ports, you may need an extra software filter for PDM-to-PCM conversion.

### Standard Mode

In standard mode, there are always two sound channels, i.e., the left and right channels, which are called "slots". These slots support 8/16/24/32-bit width sample data. The communication format for the slots mainly includes the following:

- **Philips Format**: Data signal has one-bit shift comparing to the WS signal, and the duty of WS signal is 50%.
- **MSB Format**: Basically the same as Philips format, but without data shift.
- **PCM Short Format**: Data has one-bit shift and meanwhile the WS signal becomes a pulse lasting for one BCLK cycle.

### PDM Mode

PDM (Pulse-density Modulation) digitalizes the analog signal by oversampling with 1-bit resolution. It represents the analog signal by the pulse density, the higher the pulse density, the larger the corresponding analog quantity. The PDM timing diagram is shown as follow:

The PDM format data can be transferred into PCM format by the following steps:

1. Low-pass filtering: To restore the analog wave. It is usually a FIR filter;
2. Down-sampling: To reduce the PDM sample rate to the expected PCM sample rate. Normally we decimate one sample every specific number of samples;
3. High-pass filtering: To remove the DC offset of the analog wave;
4. Amplifying: To adjust the final gain of the converted PCM format data. It can be done by simply amplifying a coefficient.

For I2S ports with a `PCM-to-PDM` converter, the hardware can convert PCM format data to PDM format when sending the data. For I2S ports with a `PDM-to-PCM` converter, the hardware can convert PDM format data to PCM format when receiving the data. If the hardware does not have the converters above, then the PDM mode can only read/write raw PDM format data. You need to realize a software filter to convert the raw PDM data into PCM format.

Note

In PDM mode, regardless of whether you are using raw PDM or PCM format, the data unit width is always 16 bits. For example, if you are sending data in raw PDM format, the data in the buffer is supposed to be arranged as follows: CH0 0x1234, CH1 0x5678, CH0 0x9abc, CH1 0xdef0. Same in the RX direction.

#### PDM TX Mode in Raw PDM Format

To use the PDM TX mode in raw PDM format, set to. Be cautious when setting, as the PDM sample rate is normally in the MHz range, typically between 1.024 MHz and 6.144 MHz. Adjust it according to your needs.

As for the slot configuration of raw PDM format, you can use the helper macros like or.

#### PDM TX Mode in PCM Format (with PCM-to-PDM Converter)

ESP32-P4 supports PCM-to-PDM converter on `I2S0`. To send PCM format data in the PDM TX mode, you need to set to. And then please take care when setting the, the PCM sample rate is normally below 100KHz, typically, it ranges from 16KHz to 48KHz, you can set it according to your needs.

And the up-sampling parameters can be set for the PCM-to-PDM converter, i.e., and. The up-sampling rate can be calculated by `up_sample_rate = i2s_pdm_tx_clk_config_t::up_sample_fp / i2s_pdm_tx_clk_config_t::up_sample_fs`. There are two up-sampling modes for PCM-to-PDM converter. The relation of the PDM clock on CLK pin and the PCM sample rate that set in the driver are shown as follow:

- **Fixed Clock Frequency**: In this mode, the up-sampling rate changes according to the sample rate. Setting `fp = 960` and `fs = (PCM)sample_rate / 100`, then the PDM clock frequency on the CLK pin will be fixed to `128 * 48 KHz = 6.144 MHz`.
- **Fixed Up-sampling Rate**: In this mode, the up-sampling rate is fixed to 2. Setting `fp = 960` and `fs = 480`, then the PDM clock frequency on CLK pin will be `128 * (PCM)sample_rate`.

As for the slot configuration of PCM format, you can use the helper macros like or.

#### PDM RX Mode in Raw PDM Format

To use the PDM RX mode in raw PDM format, you need to set to. And then please take care when setting the, the PDM sample rate is normally several MHz, typically, it ranges from 1.024MHz to 6.144MHz, you can set it according to your needs.

As for the slot configuration of raw PDM format, you can use the helper macro.

#### PDM RX Mode in PCM Format (with PDM-to-PCM Converter)

ESP32-P4 supports PDM-to-PCM converter on `I2S0`. To receive PCM format data in the PDM RX mode, you need to set to. And then please take care when setting the, the PCM sample rate is normally below 100KHz, typically, it ranges from 16KHz to 48KHz, you can set it according to your needs.

The down-sampling parameter can be set to the PDM-to-PCM converter, which is. There are two down-sampling modes for PDM-to-PCM converter, the relation of the PDM clock on CLK pin and the PCM sample rate that set in the driver are shown as follow:

- : In this mode, the PDM clock frequency on the CLK pin is `(PCM) sample_rate * 64`.
- : In this mode, the PDM clock frequency on the CLK pin is `(PCM) sample_rate * 128`.

As for the slot configuration of PCM format, you can use the helper macro like

### TDM Mode

TDM (Time Division Multiplexing) mode supports up to 16 slots. These slots can be enabled by.

But due to the hardware limitation, only up to 4 slots are supported while the slot is set to 32 bit-width, and 8 slots for 16 bit-width, 16 slots for 8 bit-width. The slot communication format of TDM is almost the same as the standard mode, yet with some small differences.

- **Philips Format**: Data signal has one-bit shift comparing to the WS signal. And no matter how many slots are contained in one frame, the duty of WS signal always keeps 50%.
- **MSB Format**: Basically the same as the Philips format, but without data shift.
- **PCM Short Format**: Data has one-bit shift and the WS signal becomes a pulse lasting one BCLK cycle for every frame.
- **PCM Long Format**: Data has one-bit shift and the WS signal lasts one-slot bit width for every frame. For example, the duty of WS will be 25% if there are four slots enabled, and 20% if there are five slots.

## Functional Overview

The I2S driver offers the following services:

### Resource Management

There are three levels of resources in the I2S driver:

- `platform level`: Resources of all I2S controllers in the current target.
- `controller level`: Resources in one I2S controller.
- `channel level`: Resources of TX or RX channel in one I2S controller.

The public APIs are all channel-level APIs. The channel handle can help users to manage the resources under a specific channel without considering the other two levels. The other two upper levels' resources are private and are managed by the driver automatically. Users can call to allocate a channel handle and call to delete it.

### Power Management

When the power management is enabled (i.e., [CONFIG\_PM\_ENABLE](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/kconfig-reference.html#config-pm-enable) is on), the system will adjust or stop the source clock of I2S before entering Light-sleep, thus potentially changing the I2S signals and leading to transmitting or receiving invalid data.

The I2S driver can prevent the system from changing or stopping the source clock by acquiring a power management lock. When the source clock is generated from APB, the lock type will be set to [`esp_pm_lock_type_t::ESP_PM_APB_FREQ_MAX`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/power_management.html#_CPPv4N18esp_pm_lock_type_t19ESP_PM_APB_FREQ_MAXE "esp_pm_lock_type_t::ESP_PM_APB_FREQ_MAX") and when the source clock is APLL (if supported), it will be set to [`esp_pm_lock_type_t::ESP_PM_NO_LIGHT_SLEEP`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/power_management.html#_CPPv4N18esp_pm_lock_type_t21ESP_PM_NO_LIGHT_SLEEPE "esp_pm_lock_type_t::ESP_PM_NO_LIGHT_SLEEP"). Whenever the user is reading or writing via I2S (i.e., calling or ), the driver guarantees that the power management lock is acquired. Likewise, the driver releases the lock after the reading or writing finishes.

### Finite State Machine

There are three states for an I2S channel, namely, `registered`, `ready`, and `running`. Their relationship is shown in the following diagram:

![I2S Finite State Machine](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/_images/i2s_state_machine.png)

I2S Finite State Machine 

The `<mode>` in the diagram can be replaced by corresponding I2S communication modes, e.g., `std` for standard two-slot mode. For more information about communication modes, please refer to the section.

### Data Transport

The data transport of the I2S peripheral, including sending and receiving, is realized by DMA. Before transporting data, please call to enable the specific channel. When the sent or received data reaches the size of one DMA buffer, the `I2S_OUT_EOF` or `I2S_IN_SUC_EOF` interrupt will be triggered. Note that the DMA buffer size is not equal to. One frame here refers to all the sampled data in one WS circle. Therefore, `dma_buffer_size = dma_frame_num * slot_num * slot_bit_width / 8`. For the data transmitting, users can input the data by calling. This function helps users to copy the data from the source buffer to the DMA TX buffer and wait for the transmission to finish. Then it will repeat until the sent bytes reach the given size. For the data receiving, the function waits to receive the message queue which contains the DMA buffer address. It helps users copy the data from the DMA RX buffer to the destination buffer.

Both and are blocking functions. They keeps waiting until the whole source buffer is sent or the whole destination buffer is loaded, unless they exceed the max blocking time, where the error code `ESP_ERR_TIMEOUT` returns. To send or receive data asynchronously, callbacks can be registered by. Users are able to access the DMA buffer directly in the callback function instead of transmitting or receiving by the two blocking functions. However, please be aware that it is an interrupt callback, so do not add complex logic, run floating operation, or call non-reentrant functions in the callback.

### Configuration

Users can initialize a channel by calling corresponding functions (i.e., `i2s_channel_init_std_mode()`, `i2s_channel_init_pdm_rx_mode()`, `i2s_channel_init_pdm_tx_mode()`, or `i2s_channel_init_tdm_mode()`) to a specific mode. If the configurations need to be updated after initialization, users have to first call to ensure that the channel has stopped, and then call corresponding `reconfig` functions, like,, and.

### Advanced API

To satisfy the high quality audio requirement, following advanced APIs are provided:

- : Preloading audio data into the I2S internal cache, enabling the TX channel to immediately send data upon activation, thereby reducing the initial audio output delay.
- : Dynamically fine-tuning the audio rate at runtime to match the speed of the audio data producer and consumer, thereby preventing the accumulation or shortage of intermediate buffered data that caused by rate mismatches.

### IRAM Safe

By default, the I2S interrupt will be deferred when the cache is disabled for reasons like writing/erasing flash. Thus the EOF interrupt will not get executed in time.

To avoid such case in real-time applications, you can enable the Kconfig option [CONFIG\_I2S\_ISR\_IRAM\_SAFE](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/kconfig-reference.html#config-i2s-isr-iram-safe) that:

1. Keeps the interrupt being serviced even when the cache is disabled.
2. Places driver object into DRAM (in case it is linked to PSRAM by accident).

This allows the interrupt to run while the cache is disabled, but comes at the cost of increased IRAM consumption.

### Thread Safety

All the public I2S APIs are guaranteed to be thread safe by the driver, which means users can call them from different RTOS tasks without protection by extra locks. Notice that the I2S driver uses mutex lock to ensure the thread safety, thus these APIs are not allowed to be used in ISR.

### Kconfig Options

- [CONFIG\_I2S\_ISR\_IRAM\_SAFE](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/kconfig-reference.html#config-i2s-isr-iram-safe) controls whether the default ISR handler can work when the cache is disabled. See for more information.
- [CONFIG\_I2S\_ENABLE\_DEBUG\_LOG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/kconfig-reference.html#config-i2s-enable-debug-log) is used to enable the debug log output. Enable this option increases the firmware binary size.

## Application Example

The examples of the I2S driver can be found in the directory [peripherals/i2s](https://github.com/espressif/esp-idf/tree/v6.0.1/examples/peripherals/i2s). Here are some simple usages of each mode:

### Standard TX/RX Usage

- [peripherals/i2s/i2s\_codec/i2s\_es8311](https://github.com/espressif/esp-idf/tree/v6.0.1/examples/peripherals/i2s/i2s_codec/i2s_es8311) demonstrates how to use the I2S ES8311 audio codec with ESP32-P4 to play music or echo sounds, featuring high performance and low power multi-bit delta-sigma audio ADC and DAC, with options to customize music and adjust mic gain and volume.
- [peripherals/i2s/i2s\_basic/i2s\_std](https://github.com/espressif/esp-idf/tree/v6.0.1/examples/peripherals/i2s/i2s_basic/i2s_std) demonstrates how to use the I2S standard mode in either simplex or full-duplex mode on ESP32-P4.

Different slot communication formats can be generated by the following helper macros for standard mode. As described above, there are three formats in standard mode, and their helper macros are:

The clock config helper macro is:

Please refer to for information about STD API. And for more details, please refer to [esp\_driver\_i2s/include/driver/i2s\_std.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_driver_i2s/include/driver/i2s_std.h).

#### STD TX Mode

Take 16-bit data width for example. When the data in a `uint16_t` writing buffer are:

| data 0 | data 1 | data 2 | data 3 | data 4 | data 5 | data 6 | data 7 | ... |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0x0001 | 0x0002 | 0x0003 | 0x0004 | 0x0005 | 0x0006 | 0x0007 | 0x0008 | ... |

Here is the table of the real data on the line with different and.

<table><thead><tr><th><p>data bit width</p></th><th><p>slot mode</p></th><th><p>slot mask</p></th><th><p>WS low</p></th><th><p>WS high</p></th><th><p>WS low</p></th><th><p>WS high</p></th><th><p>WS low</p></th><th><p>WS high</p></th><th><p>WS low</p></th><th><p>WS high</p></th></tr></thead><tbody><tr><td rowspan="6"><p>16 bit</p></td><td rowspan="3"><p>mono</p></td><td><p>left</p></td><td><p>0x0001</p></td><td><p>0x0000</p></td><td><p>0x0002</p></td><td><p>0x0000</p></td><td><p>0x0003</p></td><td><p>0x0000</p></td><td><p>0x0004</p></td><td><p>0x0000</p></td></tr><tr><td><p>right</p></td><td><p>0x0000</p></td><td><p>0x0001</p></td><td><p>0x0000</p></td><td><p>0x0002</p></td><td><p>0x0000</p></td><td><p>0x0003</p></td><td><p>0x0000</p></td><td><p>0x0004</p></td></tr><tr><td><p>both</p></td><td><p>0x0001</p></td><td><p>0x0001</p></td><td><p>0x0002</p></td><td><p>0x0002</p></td><td><p>0x0003</p></td><td><p>0x0003</p></td><td><p>0x0004</p></td><td><p>0x0004</p></td></tr><tr><td rowspan="3"><p>stereo</p></td><td><p>left</p></td><td><p>0x0001</p></td><td><p>0x0000</p></td><td><p>0x0003</p></td><td><p>0x0000</p></td><td><p>0x0005</p></td><td><p>0x0000</p></td><td><p>0x0007</p></td><td><p>0x0000</p></td></tr><tr><td><p>right</p></td><td><p>0x0000</p></td><td><p>0x0002</p></td><td><p>0x0000</p></td><td><p>0x0004</p></td><td><p>0x0000</p></td><td><p>0x0006</p></td><td><p>0x0000</p></td><td><p>0x0008</p></td></tr><tr><td><p>both</p></td><td><p>0x0001</p></td><td><p>0x0002</p></td><td><p>0x0003</p></td><td><p>0x0004</p></td><td><p>0x0005</p></td><td><p>0x0006</p></td><td><p>0x0007</p></td><td><p>0x0008</p></td></tr></tbody></table>

Note

Similar for 8-bit and 32-bit data widths, the type of the buffer is better to be `uint8_t` and `uint32_t`. But specially, when the data width is 24-bit, the data buffer should be aligned with 3-byte (i.e., every 3 bytes stands for a 24-bit data in one slot). Additionally,,, and the writing buffer size should be the multiple of `3`, otherwise the data on the line or the sample rate will be incorrect.

```c
#include "driver/i2s_std.h"
#include "driver/gpio.h"

i2s_chan_handle_t tx_handle;
/* Get the default channel configuration by the helper macro.
 * This helper macro is defined in \`i2s_common.h\` and shared by all the I2S communication modes.
 * It can help to specify the I2S role and port ID */
i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
/* Allocate a new TX channel and get the handle of this channel */
i2s_new_channel(&chan_cfg, &tx_handle, NULL);

/* Setting the configurations, the slot configuration and clock configuration can be generated by the macros
 * These two helper macros are defined in \`i2s_std.h\` which can only be used in STD mode.
 * They can help to specify the slot and clock configurations for initialization or updating */
i2s_std_config_t std_cfg = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(48000),
    .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg = {
        .mclk = I2S_GPIO_UNUSED,
        .bclk = GPIO_NUM_4,
        .ws = GPIO_NUM_5,
        .dout = GPIO_NUM_18,
        .din = I2S_GPIO_UNUSED,
        .invert_flags = {
            .mclk_inv = false,
            .bclk_inv = false,
            .ws_inv = false,
        },
    },
};
/* Initialize the channel */
i2s_channel_init_std_mode(tx_handle, &std_cfg);

/* Before writing data, start the TX channel first */
i2s_channel_enable(tx_handle);
i2s_channel_write(tx_handle, src_buf, bytes_to_write, bytes_written, ticks_to_wait);

/* If the configurations of slot or clock need to be updated,
 * stop the channel first and then update it */
// i2s_channel_disable(tx_handle);
// std_cfg.slot_cfg.slot_mode = I2S_SLOT_MODE_MONO; // Default is stereo
// i2s_channel_reconfig_std_slot(tx_handle, &std_cfg.slot_cfg);
// std_cfg.clk_cfg.sample_rate_hz = 96000;
// i2s_channel_reconfig_std_clock(tx_handle, &std_cfg.clk_cfg);

/* Have to stop the channel before deleting it */
i2s_channel_disable(tx_handle);
/* If the handle is not needed any more, delete it to release the channel resources */
i2s_del_channel(tx_handle);
```

#### STD RX Mode

Taking 16-bit data width for example, when the data on the line are:

| WS low | WS high | WS low | WS high | WS low | WS high | WS low | WS high | ... |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0x0001 | 0x0002 | 0x0003 | 0x0004 | 0x0005 | 0x0006 | 0x0007 | 0x0008 | ... |

Here is the table of the data received in the buffer with different and.

<table><thead><tr><th><p>data bit width</p></th><th><p>slot mode</p></th><th><p>slot mask</p></th><th><p>data 0</p></th><th><p>data 1</p></th><th><p>data 2</p></th><th><p>data 3</p></th><th><p>data 4</p></th><th><p>data 5</p></th><th><p>data 6</p></th><th><p>data 7</p></th></tr></thead><tbody><tr><td rowspan="3"><p>16 bit</p></td><td rowspan="2"><p>mono</p></td><td><p>left</p></td><td><p>0x0001</p></td><td><p>0x0003</p></td><td><p>0x0005</p></td><td><p>0x0007</p></td><td><p>0x0009</p></td><td><p>0x000b</p></td><td><p>0x000d</p></td><td><p>0x000f</p></td></tr><tr><td><p>right</p></td><td><p>0x0002</p></td><td><p>0x0004</p></td><td><p>0x0006</p></td><td><p>0x0008</p></td><td><p>0x000a</p></td><td><p>0x000c</p></td><td><p>0x000e</p></td><td><p>0x0010</p></td></tr><tr><td><p>stereo</p></td><td><p>any</p></td><td><p>0x0001</p></td><td><p>0x0002</p></td><td><p>0x0003</p></td><td><p>0x0004</p></td><td><p>0x0005</p></td><td><p>0x0006</p></td><td><p>0x0007</p></td><td><p>0x0008</p></td></tr></tbody></table>

Note

8-bit, 24-bit, and 32-bit are similar as 16-bit, the data bit-width in the receiving buffer is equal to the data bit-width on the line. Additionally, when using 24-bit data width,,, and the receiving buffer size should be the multiple of `3`, otherwise the data on the line or the sample rate will be incorrect.

```c
#include "driver/i2s_std.h"
#include "driver/gpio.h"

i2s_chan_handle_t rx_handle;
/* Get the default channel configuration by helper macro.
 * This helper macro is defined in \`i2s_common.h\` and shared by all the I2S communication modes.
 * It can help to specify the I2S role and port ID */
i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
/* Allocate a new RX channel and get the handle of this channel */
i2s_new_channel(&chan_cfg, NULL, &rx_handle);

/* Setting the configurations, the slot configuration and clock configuration can be generated by the macros
 * These two helper macros are defined in \`i2s_std.h\` which can only be used in STD mode.
 * They can help to specify the slot and clock configurations for initialization or updating */
i2s_std_config_t std_cfg = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(48000),
    .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg = {
        .mclk = I2S_GPIO_UNUSED,
        .bclk = GPIO_NUM_4,
        .ws = GPIO_NUM_5,
        .dout = I2S_GPIO_UNUSED,
        .din = GPIO_NUM_19,
        .invert_flags = {
            .mclk_inv = false,
            .bclk_inv = false,
            .ws_inv = false,
        },
    },
};
/* Initialize the channel */
i2s_channel_init_std_mode(rx_handle, &std_cfg);

/* Before reading data, start the RX channel first */
i2s_channel_enable(rx_handle);
i2s_channel_read(rx_handle, desc_buf, bytes_to_read, bytes_read, ticks_to_wait);

/* Have to stop the channel before deleting it */
i2s_channel_disable(rx_handle);
/* If the handle is not needed any more, delete it to release the channel resources */
i2s_del_channel(rx_handle);
```

### PDM TX Usage

- [peripherals/i2s/i2s\_basic/i2s\_pdm](https://github.com/espressif/esp-idf/tree/v6.0.1/examples/peripherals/i2s/i2s_basic/i2s_pdm) demonstrates how to use the PDM TX mode on ESP32-P4, including the necessary hardware setup and configuration.

For PDM mode in TX channel, the slot configuration helper macro is:

- `I2S_PDM_TX_SLOT_DEFAULT_CONFIG`

The clock configuration helper macro is:

Please refer to for information about PDM TX API. And for more details, please refer to [esp\_driver\_i2s/include/driver/i2s\_pdm.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_driver_i2s/include/driver/i2s_pdm.h).

The PDM data width is fixed to 16-bit. When the data in an `int16_t` writing buffer is:

| data 0 | data 1 | data 2 | data 3 | data 4 | data 5 | data 6 | data 7 | ... |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0x0001 | 0x0002 | 0x0003 | 0x0004 | 0x0005 | 0x0006 | 0x0007 | 0x0008 | ... |

Here is the table of the real data on the line with different and (The PDM format on the line is transferred to PCM format for easier comprehension).

<table><thead><tr><th><p>line mode</p></th><th><p>slot mode</p></th><th><p>line</p></th><th><p>left</p></th><th><p>right</p></th><th><p>left</p></th><th><p>right</p></th><th><p>left</p></th><th><p>right</p></th><th><p>left</p></th><th><p>right</p></th></tr></thead><tbody><tr><td rowspan="2"><p>one-line Codec</p></td><td><p>mono</p></td><td><p>dout</p></td><td><p>0x0001</p></td><td><p>0x0000</p></td><td><p>0x0002</p></td><td><p>0x0000</p></td><td><p>0x0003</p></td><td><p>0x0000</p></td><td><p>0x0004</p></td><td><p>0x0000</p></td></tr><tr><td><p>stereo</p></td><td><p>dout</p></td><td><p>0x0001</p></td><td><p>0x0002</p></td><td><p>0x0003</p></td><td><p>0x0004</p></td><td><p>0x0005</p></td><td><p>0x0006</p></td><td><p>0x0007</p></td><td><p>0x0008</p></td></tr><tr><td><p>one-line DAC</p></td><td><p>mono</p></td><td><p>dout</p></td><td><p>0x0001</p></td><td><p>0x0001</p></td><td><p>0x0002</p></td><td><p>0x0002</p></td><td><p>0x0003</p></td><td><p>0x0003</p></td><td><p>0x0004</p></td><td><p>0x0004</p></td></tr><tr><td rowspan="4"><p>two-line DAC</p></td><td rowspan="2"><p>mono</p></td><td><p>dout</p></td><td><p>0x0002</p></td><td><p>0x0002</p></td><td><p>0x0004</p></td><td><p>0x0004</p></td><td><p>0x0006</p></td><td><p>0x0006</p></td><td><p>0x0008</p></td><td><p>0x0008</p></td></tr><tr><td><p>dout2</p></td><td><p>0x0000</p></td><td><p>0x0000</p></td><td><p>0x0000</p></td><td><p>0x0000</p></td><td><p>0x0000</p></td><td><p>0x0000</p></td><td><p>0x0000</p></td><td><p>0x0000</p></td></tr><tr><td rowspan="2"><p>stereo</p></td><td><p>dout</p></td><td><p>0x0002</p></td><td><p>0x0002</p></td><td><p>0x0004</p></td><td><p>0x0004</p></td><td><p>0x0006</p></td><td><p>0x0006</p></td><td><p>0x0008</p></td><td><p>0x0008</p></td></tr><tr><td><p>dout2</p></td><td><p>0x0001</p></td><td><p>0x0001</p></td><td><p>0x0003</p></td><td><p>0x0003</p></td><td><p>0x0005</p></td><td><p>0x0005</p></td><td><p>0x0007</p></td><td><p>0x0007</p></td></tr></tbody></table>

Note

There are three line modes for PDM TX mode, i.e., `I2S_PDM_TX_ONE_LINE_CODEC`, `I2S_PDM_TX_ONE_LINE_DAC`, and `I2S_PDM_TX_TWO_LINE_DAC`. One-line codec is for the PDM codecs that require clock signal. The PDM codec can differentiate the left and right slots by the clock level. The other two modes are used to drive power amplifiers directly with a low-pass filter. They do not need the clock signal, so there are two lines to differentiate the left and right slots. Additionally, for the mono mode of one-line codec, users can force change the slot to the right by setting the clock invert flag in GPIO configuration.

```c
#include "driver/i2s_pdm.h"
#include "driver/gpio.h"

/* Allocate an I2S TX channel */
i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
i2s_new_channel(&chan_cfg, &tx_handle, NULL);

/* Init the channel into PDM TX mode */
i2s_pdm_tx_config_t pdm_tx_cfg = {
    .clk_cfg = I2S_PDM_TX_CLK_DEFAULT_CONFIG(36000),
    .slot_cfg = I2S_PDM_TX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
    .gpio_cfg = {
        .clk = GPIO_NUM_5,
        .dout = GPIO_NUM_18,
        .invert_flags = {
            .clk_inv = false,
        },
    },
};
i2s_channel_init_pdm_tx_mode(tx_handle, &pdm_tx_cfg);

...
```

### PDM RX Usage

- [peripherals/i2s/i2s\_recorder](https://github.com/espressif/esp-idf/tree/v6.0.1/examples/peripherals/i2s/i2s_recorder) demonstrates how to record audio from a digital MEMS microphone using the I2S peripheral in PDM data format and save it to an SD card in `.wav` file format on ESP32-P4 development boards.
- [peripherals/i2s/i2s\_basic/i2s\_pdm](https://github.com/espressif/esp-idf/tree/v6.0.1/examples/peripherals/i2s/i2s_basic/i2s_pdm) demonstrates how to use the PDM RX mode on ESP32-P4, including the necessary hardware setup and configuration.

For PDM mode in RX channel, the slot configuration helper macro are:

- It provides some default configurations for receiving the raw PDM format data.
- It provides some default configurations for receiving the converted PCM format data.

The clock configuration helper macro is:

Please refer to for information about PDM RX API. And for more details, please refer to [esp\_driver\_i2s/include/driver/i2s\_pdm.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_driver_i2s/include/driver/i2s_pdm.h).

The PDM data width is fixed to 16-bit. When the data on the line (The PDM format on the line is transferred to PCM format for easier comprehension) is:

| left | right | left | right | left | right | left | right | ... |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0x0001 | 0x0002 | 0x0003 | 0x0004 | 0x0005 | 0x0006 | 0x0007 | 0x0008 | ... |

Here is the table of the data received in a `int16_t` buffer with different and.

```c
#include "driver/i2s_pdm.h"
#include "driver/gpio.h"

i2s_chan_handle_t rx_handle;

/* Allocate an I2S RX channel */
i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
i2s_new_channel(&chan_cfg, NULL, &rx_handle);

/* Init the channel into PDM RX mode */
i2s_pdm_rx_config_t pdm_rx_cfg = {
    .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(36000),
    // If PDM-to-PCM converter is not supported, please use raw PDM format
    // .slot_cfg = I2S_PDM_RX_SLOT_RAW_FMT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
    .slot_cfg = I2S_PDM_RX_SLOT_PCM_FMT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
    .gpio_cfg = {
        .clk = GPIO_NUM_5,
        .din = GPIO_NUM_19,
        .invert_flags = {
            .clk_inv = false,
        },
    },
};
i2s_channel_init_pdm_rx_mode(rx_handle, &pdm_rx_cfg);

...
```

### TDM TX/RX Usage

- [peripherals/i2s/i2s\_codec/i2s\_es7210\_tdm](https://github.com/espressif/esp-idf/tree/v6.0.1/examples/peripherals/i2s/i2s_codec/i2s_es7210_tdm) demonstrates how to use the I2S TDM mode on ESP32-P4 to record four MICs connected to ES7210 codec, with the recorded voice saved to an SD card in `wav` format.
- [peripherals/i2s/i2s\_basic/i2s\_tdm](https://github.com/espressif/esp-idf/tree/v6.0.1/examples/peripherals/i2s/i2s_basic/i2s_tdm) demonstrates how to use the TDM mode in simplex or full-duplex mode on ESP32-P4.

Different slot communication formats can be generated by the following helper macros for TDM mode. As described above, there are four formats in TDM mode, and their helper macros are:

The clock config helper macro is:

Please refer to for information about TDM API. And for more details, please refer to [esp\_driver\_i2s/include/driver/i2s\_tdm.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_driver_i2s/include/driver/i2s_tdm.h).

Note

Due to hardware limitation, when setting the clock configuration for a slave role, please be aware that should not be smaller than 8. Increasing this field can reduce the lagging of the data sent from the slave. In the high sample rate case, the data might lag behind for more than one BCLK which leads to data malposition. Users may gradually increase to correct it.

As is the division of MCLK to BCLK, increasing it also increases the MCLK frequency. Therefore, the clock calculation may fail if MCLK is too high to divide from the source clock. This means that a larger value for is not necessarily better.

#### TDM TX Mode

```c
#include "driver/i2s_tdm.h"
#include "driver/gpio.h"

/* Allocate an I2S TX channel */
i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
i2s_new_channel(&chan_cfg, &tx_handle, NULL);

/* Init the channel into TDM mode */
i2s_tdm_config_t tdm_cfg = {
    .clk_cfg = I2S_TDM_CLK_DEFAULT_CONFIG(44100),
    .slot_cfg = I2S_TDM_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO,
                I2S_TDM_SLOT0 | I2S_TDM_SLOT1 | I2S_TDM_SLOT2 | I2S_TDM_SLOT3),
    .gpio_cfg = {
        .mclk = I2S_GPIO_UNUSED,
        .bclk = GPIO_NUM_4,
        .ws = GPIO_NUM_5,
        .dout = GPIO_NUM_18,
        .din = I2S_GPIO_UNUSED,
        .invert_flags = {
            .mclk_inv = false,
            .bclk_inv = false,
            .ws_inv = false,
        },
    },
};
i2s_channel_init_tdm_mode(tx_handle, &tdm_cfg);

...
```

#### TDM RX Mode

```c
#include "driver/i2s_tdm.h"
#include "driver/gpio.h"

/* Set the channel mode to TDM */
i2s_chan_config_t chan_cfg = I2S_CHANNEL_CONFIG(I2S_ROLE_MASTER, I2S_COMM_MODE_TDM, &i2s_pin);
i2s_new_channel(&chan_cfg, NULL, &rx_handle);

/* Init the channel into TDM mode */
i2s_tdm_config_t tdm_cfg = {
    .clk_cfg = I2S_TDM_CLK_DEFAULT_CONFIG(44100),
    .slot_cfg = I2S_TDM_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO,
                I2S_TDM_SLOT0 | I2S_TDM_SLOT1 | I2S_TDM_SLOT2 | I2S_TDM_SLOT3),
    .gpio_cfg = {
        .mclk = I2S_GPIO_UNUSED,
        .bclk = GPIO_NUM_4,
        .ws = GPIO_NUM_5,
        .dout = I2S_GPIO_UNUSED,
        .din = GPIO_NUM_18,
        .invert_flags = {
            .mclk_inv = false,
            .bclk_inv = false,
            .ws_inv = false,
        },
    },
};
i2s_channel_init_tdm_mode(rx_handle, &tdm_cfg);
...
```

### Full-duplex

Full-duplex mode registers TX and RX channel in an I2S port at the same time, and the channels share the BCLK and WS signals. Currently, standard and TDM communication modes supports full-duplex mode in the following way, but PDM full-duplex is not supported because due to different PDM TX and RX clocks.

Note that one handle can only stand for one channel. Therefore, it is still necessary to configure the slot and clock for both TX and RX channels one by one.

There are two methods to allocate a pair of full-duplex channels:

1. Allocate both TX and RX handles in a single call of.

```c
#include "driver/i2s_std.h"
#include "driver/gpio.h"

i2s_chan_handle_t tx_handle;
i2s_chan_handle_t rx_handle;

/* Allocate a pair of I2S channel */
i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
/* Allocate for TX and RX channel at the same time, then they will work in full-duplex mode */
i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle);

/* Set the configurations for BOTH TWO channels, since TX and RX channel have to be same in full-duplex mode */
i2s_std_config_t std_cfg = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(32000),
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg = {
        .mclk = I2S_GPIO_UNUSED,
        .bclk = GPIO_NUM_4,
        .ws = GPIO_NUM_5,
        .dout = GPIO_NUM_18,
        .din = GPIO_NUM_19,
        .invert_flags = {
            .mclk_inv = false,
            .bclk_inv = false,
            .ws_inv = false,
        },
    },
};
i2s_channel_init_std_mode(tx_handle, &std_cfg);
i2s_channel_init_std_mode(rx_handle, &std_cfg);

i2s_channel_enable(tx_handle);
i2s_channel_enable(rx_handle);

...
```

1. Allocate TX and RX handles separately, and initialize them with the same configuration.

```c
#include "driver/i2s_std.h"
#include "driver/gpio.h"

i2s_chan_handle_t tx_handle;
i2s_chan_handle_t rx_handle;

/* Allocate a pair of I2S channels on a same port */
i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
/* Allocate for TX and RX channel separately, they are not full-duplex yet */
ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));

/* Set the configurations for BOTH TWO channels, they will constitute in full-duplex mode automatically */
i2s_std_config_t std_cfg = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(32000),
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg = {
        .mclk = I2S_GPIO_UNUSED,
        .bclk = GPIO_NUM_4,
        .ws = GPIO_NUM_5,
        .dout = GPIO_NUM_18,
        .din = GPIO_NUM_19,
        .invert_flags = {
            .mclk_inv = false,
            .bclk_inv = false,
            .ws_inv = false,
        },
    },
};
ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));
ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));
// ...
ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));
ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));
ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));

...
```

### Simplex Mode

To allocate a channel in simplex mode, should be called for each channel. The clock and GPIO pins of TX/RX channel on ESP32-P4 are independent, so they can be configured with different modes and clocks, and are able to coexist on the same I2S port in simplex mode. PDM duplex can be realized by registering PDM TX simplex and PDM RX simplex on the same I2S port. But in this way, PDM TX/RX might work with different clocks, so take care when configuring the GPIO pins and clocks.

The following example offers a use case for the simplex mode, but note that although the internal MCLK signals for TX and RX channel are separate, the output MCLK can only be bound to one of them if they are from the same controller. If MCLK has been initialized by both channels, it will be bound to the channel that initializes later.

```c
#include "driver/i2s_std.h"
#include "driver/gpio.h"

i2s_chan_handle_t tx_handle;
i2s_chan_handle_t rx_handle;
i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));
i2s_std_config_t std_tx_cfg = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(48000),
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg = {
        .mclk = GPIO_NUM_0,
        .bclk = GPIO_NUM_4,
        .ws = GPIO_NUM_5,
        .dout = GPIO_NUM_18,
        .din = I2S_GPIO_UNUSED,
        .invert_flags = {
            .mclk_inv = false,
            .bclk_inv = false,
            .ws_inv = false,
        },
    },
};
/* Initialize the channel */
ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_tx_cfg));
ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));

/* RX channel will be registered on another I2S, if no other available I2S unit found
 * it will return ESP_ERR_NOT_FOUND */
ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle)); // Both RX and TX channel will be registered on I2S0, but they can work with different configurations.
i2s_std_config_t std_rx_cfg = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
    .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg = {
        .mclk = I2S_GPIO_UNUSED,
        .bclk = GPIO_NUM_6,
        .ws = GPIO_NUM_7,
        .dout = I2S_GPIO_UNUSED,
        .din = GPIO_NUM_19,
        .invert_flags = {
            .mclk_inv = false,
            .bclk_inv = false,
            .ws_inv = false,
        },
    },
};
ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_rx_cfg));
ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
```

### I2S ETM Usage

ESP32-P4 supports I2S ETM (Event Task Matrix), which allows to trigger other ETM tasks via I2S ETM events, or to control the start/stop by I2S ETM tasks.

The I2S ETM APIs can be found in `driver/i2s_etm.h`, the following example shows how to use GPIO to start/stop I2S channel via ETM:

```c
#include "driver/i2s_etm.h"
// ...
i2s_chan_handle_t tx_handle;
// Initialize I2S channel
// ......
int ctrl_gpio = 4;
// Initialize GPIO
// ......
/* Register GPIO ETM events */
gpio_etm_event_config_t gpio_event_cfg = {
    .edges = {GPIO_ETM_EVENT_EDGE_POS, GPIO_ETM_EVENT_EDGE_NEG},
};
esp_etm_event_handle_t gpio_pos_event_handle;
esp_etm_event_handle_t gpio_neg_event_handle;
gpio_new_etm_event(&gpio_event_cfg, &gpio_pos_event_handle, &gpio_neg_event_handle);
gpio_etm_event_bind_gpio(gpio_pos_event_handle, ctrl_gpio);
gpio_etm_event_bind_gpio(gpio_neg_event_handle, ctrl_gpio);
/* Register I2S ETM tasks */
i2s_etm_task_config_t i2s_start_task_cfg = {
    .task_type = I2S_ETM_TASK_START,
};
esp_etm_task_handle_t i2s_start_task_handle;
i2s_new_etm_task(tx_handle, &i2s_start_task_cfg, &i2s_start_task_handle);
i2s_etm_task_config_t i2s_stop_task_cfg = {
    .task_type = I2S_ETM_TASK_STOP,
};
esp_etm_task_handle_t i2s_stop_task_handle;
i2s_new_etm_task(tx_handle, &i2s_stop_task_cfg, &i2s_stop_task_handle);
/* Bind GPIO events to I2S ETM tasks */
esp_etm_channel_config_t etm_config = {};
esp_etm_channel_handle_t i2s_etm_start_chan = NULL;
esp_etm_channel_handle_t i2s_etm_stop_chan = NULL;
esp_etm_new_channel(&etm_config, &i2s_etm_start_chan);
esp_etm_new_channel(&etm_config, &i2s_etm_stop_chan);
esp_etm_channel_connect(i2s_etm_start_chan, gpio_pos_event_handle, i2s_start_task_handle);
esp_etm_channel_connect(i2s_etm_stop_chan, gpio_neg_event_handle, i2s_stop_task_handle);
esp_etm_channel_enable(i2s_etm_start_chan);
esp_etm_channel_enable(i2s_etm_stop_chan);
/* Enable I2S channel first before starting I2S channel */
i2s_channel_enable(tx_handle);
// (Optional) Able to load the data into the internal DMA buffer here,
// but tx_channel does not start yet, will timeout when the internal buffer is full
// i2s_channel_write(tx_handle, data, data_size, NULL, 0);
/* Start I2S channel by setting the GPIO to high */
gpio_set_level(ctrl_gpio, 1);
// Write data ......
// i2s_channel_write(tx_handle, data, data_size, NULL, 1000);
/* Stop I2S channel by setting the GPIO to low */
gpio_set_level(ctrl_gpio, 0);

/* Free resources */
i2s_channel_disable(tx_handle);
esp_etm_channel_disable(i2s_etm_start_chan);
esp_etm_channel_disable(i2s_etm_stop_chan);
esp_etm_del_event(gpio_pos_event_handle);
esp_etm_del_event(gpio_neg_event_handle);
esp_etm_del_task(i2s_start_task_handle);
esp_etm_del_task(i2s_stop_task_handle);
esp_etm_del_channel(i2s_etm_start_chan);
esp_etm_del_channel(i2s_etm_stop_chan);
// De-initialize I2S and GPIO
// ......
```

## Application Notes

### How to Prevent Data Lost

For applications that need a high frequency sample rate, the massive data throughput may cause data lost. Users can receive data lost event by registering the ISR callback function to receive the event queue:

> ```c
> static IRAM_ATTR bool i2s_rx_queue_overflow_callback(i2s_chan_handle_t handle, i2s_event_data_t *event, void *user_ctx)
> {
>     // handle RX queue overflow event ...
>     return false;
> }
> 
> i2s_event_callbacks_t cbs = {
>     .on_recv = NULL,
>     .on_recv_q_ovf = i2s_rx_queue_overflow_callback,
>     .on_sent = NULL,
>     .on_send_q_ovf = NULL,
> };
> TEST_ESP_OK(i2s_channel_register_event_callback(rx_handle, &cbs, NULL));
> ```

Please follow these steps to prevent data lost:

1. Determine the interrupt interval. Generally, when data lost happens, the bigger the interval, the better, which helps to reduce the interrupt times. This means `dma_frame_num` should be as big as possible while the DMA buffer size is below the maximum value of 4092. The relationships are:
	```c
	interrupt_interval(unit: sec) = dma_frame_num / sample_rate
	dma_buffer_size = dma_frame_num * slot_num * data_bit_width / 8 <= 4092
	```
2. Determine `dma_desc_num`. `dma_desc_num` is decided by the maximum time of `i2s_channel_read` polling cycle. All the received data is supposed to be stored between two `i2s_channel_read`. This cycle can be measured by a timer or an outputting GPIO signal. The relationship is:
	```c
	dma_desc_num > polling_cycle / interrupt_interval
	```
3. Determine the receiving buffer size. The receiving buffer offered by users in `i2s_channel_read` should be able to take all the data in all DMA buffers, which means that it should be larger than the total size of all the DMA buffers:
	```c
	recv_buffer_size > dma_desc_num * dma_buffer_size
	```

For example, if there is an I2S application, and the known values are:

```c
sample_rate = 144000 Hz
data_bit_width = 32 bits
slot_num = 2
polling_cycle = 10 ms
```

Then the parameters `dma_frame_num`, `dma_desc_num`, and `recv_buf_size` can be calculated as follows:

```c
dma_frame_num * slot_num * data_bit_width / 8 = dma_buffer_size <= 4092
dma_frame_num <= 511
interrupt_interval = dma_frame_num / sample_rate = 511 / 144000 = 0.003549 s = 3.549 ms
dma_desc_num > polling_cycle / interrupt_interval = cell(10 / 3.549) = cell(2.818) = 3
recv_buffer_size > dma_desc_num * dma_buffer_size = 3 * 4092 = 12276 bytes
```

## API Reference

### Standard Mode

### Header File

- [components/esp\_driver\_i2s/include/driver/i2s\_std.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_driver_i2s/include/driver/i2s_std.h)
- This header file can be included with:
	> ```c
	> #include "driver/i2s_std.h"
	> ```
- This header file is a part of the API provided by the `esp_driver_i2s` component. To declare that your component depends on `esp_driver_i2s`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES esp_driver_i2s
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES esp_driver_i2s
	> ```

### Functions

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_init\_std\_mode( handle, const \*std\_cfg) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv425i2s_channel_init_std_mode17i2s_chan_handle_tPK16i2s_std_config_t "Permalink to this definition")  

Initialize I2S channel to standard mode.

Note

Only allowed to be called when the channel state is REGISTERED, (i.e., channel has been allocated, but not initialized) and the state will be updated to READY if initialization success, otherwise the state will return to REGISTERED.

Note

When initialize the STD mode with a same configuration as another channel on a same port, these two channels can constitude as full-duplex mode automatically

Parameters:

- **handle** -- **\[in\]** I2S channel handler
- **std\_cfg** -- **\[in\]** Configurations for standard mode, including clock, slot and GPIO The clock configuration can be generated by the helper macro `I2S_STD_CLK_DEFAULT_CONFIG` The slot configuration can be generated by the helper macro `I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG`, `I2S_STD_PCM_SLOT_DEFAULT_CONFIG` or `I2S_STD_MSB_SLOT_DEFAULT_CONFIG`

Returns:

- ESP\_OK Initialize successfully
- ESP\_ERR\_NO\_MEM No memory for storing the channel information
- ESP\_ERR\_INVALID\_ARG NULL pointer or invalid configuration
- ESP\_ERR\_INVALID\_STATE This channel is not registered

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_reconfig\_std\_clock( handle, const \*clk\_cfg) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv430i2s_channel_reconfig_std_clock17i2s_chan_handle_tPK20i2s_std_clk_config_t "Permalink to this definition")  

Reconfigure the I2S clock for standard mode.

Note

Only allowed to be called when the channel state is READY, i.e., channel has been initialized, but not started this function won't change the state. `i2s_channel_disable` should be called before calling this function if I2S has started.

Note

The input channel handle has to be initialized to standard mode, i.e., `i2s_channel_init_std_mode` has been called before reconfiguring

Parameters:

- **handle** -- **\[in\]** I2S channel handler
- **clk\_cfg** -- **\[in\]** Standard mode clock configuration, can be generated by `I2S_STD_CLK_DEFAULT_CONFIG`

Returns:

- ESP\_OK Set clock successfully
- ESP\_ERR\_INVALID\_ARG NULL pointer, invalid configuration or not standard mode
- ESP\_ERR\_INVALID\_STATE This channel is not initialized or not stopped

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_reconfig\_std\_slot( handle, const \*slot\_cfg) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv429i2s_channel_reconfig_std_slot17i2s_chan_handle_tPK21i2s_std_slot_config_t "Permalink to this definition")  

Reconfigure the I2S slot for standard mode.

Note

Only allowed to be called when the channel state is READY, i.e., channel has been initialized, but not started this function won't change the state. `i2s_channel_disable` should be called before calling this function if I2S has started.

Note

The input channel handle has to be initialized to standard mode, i.e., `i2s_channel_init_std_mode` has been called before reconfiguring

Parameters:

- **handle** -- **\[in\]** I2S channel handler
- **slot\_cfg** -- **\[in\]** Standard mode slot configuration, can be generated by `I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG`, `I2S_STD_PCM_SLOT_DEFAULT_CONFIG` and `I2S_STD_MSB_SLOT_DEFAULT_CONFIG`.

Returns:

- ESP\_OK Set clock successfully
- ESP\_ERR\_NO\_MEM No memory for DMA buffer
- ESP\_ERR\_INVALID\_ARG NULL pointer, invalid configuration or not standard mode
- ESP\_ERR\_INVALID\_STATE This channel is not initialized or not stopped

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_reconfig\_std\_gpio( handle, const \*gpio\_cfg) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv429i2s_channel_reconfig_std_gpio17i2s_chan_handle_tPK21i2s_std_gpio_config_t "Permalink to this definition")  

Reconfigure the I2S GPIO for standard mode.

Note

Only allowed to be called when the channel state is READY, i.e., channel has been initialized, but not started this function won't change the state. `i2s_channel_disable` should be called before calling this function if I2S has started.

Note

The input channel handle has to be initialized to standard mode, i.e., `i2s_channel_init_std_mode` has been called before reconfiguring

Parameters:

- **handle** -- **\[in\]** I2S channel handler
- **gpio\_cfg** -- **\[in\]** Standard mode GPIO configuration, specified by user

Returns:

- ESP\_OK Set clock successfully
- ESP\_ERR\_INVALID\_ARG NULL pointer, invalid configuration or not standard mode
- ESP\_ERR\_INVALID\_STATE This channel is not initialized or not stopped

### Structures

struct i2s\_std\_slot\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv421i2s_std_slot_config_t "Permalink to this definition")  

I2S slot configuration for standard mode.

Public Members

data\_bit\_width [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_std_slot_config_t14data_bit_widthE "Permalink to this definition")  

I2S sample data bit width (valid data bits per sample)

slot\_bit\_width [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_std_slot_config_t14slot_bit_widthE "Permalink to this definition")  

I2S slot bit width (total bits per slot)

slot\_mode [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_std_slot_config_t9slot_modeE "Permalink to this definition")  

Set mono or stereo mode with I2S\_SLOT\_MODE\_MONO or I2S\_SLOT\_MODE\_STEREO In TX direction, mono means the written buffer contains only one slot data and stereo means the written buffer contains both left and right data

slot\_mask [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_std_slot_config_t9slot_maskE "Permalink to this definition")  

Select the left, right or both slot

uint32\_t ws\_width [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_std_slot_config_t8ws_widthE "Permalink to this definition")  

WS signal width (i.e. the number of BCLK ticks that WS signal is high)

bool ws\_pol [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_std_slot_config_t6ws_polE "Permalink to this definition")  

WS signal polarity, set true to enable high lever first

bool bit\_shift [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_std_slot_config_t9bit_shiftE "Permalink to this definition")  

Set to enable bit shift in Philips mode

bool left\_align [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_std_slot_config_t10left_alignE "Permalink to this definition")  

Set to enable left alignment

bool big\_endian [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_std_slot_config_t10big_endianE "Permalink to this definition")  

Set to enable big endian

bool bit\_order\_lsb [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_std_slot_config_t13bit_order_lsbE "Permalink to this definition")  

Set to enable lsb first

struct i2s\_std\_clk\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv420i2s_std_clk_config_t "Permalink to this definition")  

I2S clock configuration for standard mode.

Public Members

uint32\_t sample\_rate\_hz [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_std_clk_config_t14sample_rate_hzE "Permalink to this definition")  

I2S sample rate

clk\_src [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_std_clk_config_t7clk_srcE "Permalink to this definition")  

Choose clock source, see `soc_periph_i2s_clk_src_t` for the supported clock sources. selected `I2S_CLK_SRC_EXTERNAL` (if supports) to enable the external source clock input via MCLK pin,

uint32\_t ext\_clk\_freq\_hz [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_std_clk_config_t15ext_clk_freq_hzE "Permalink to this definition")  

External clock source frequency in Hz, only take effect when `clk_src = I2S_CLK_SRC_EXTERNAL`, otherwise this field will be ignored, Please make sure the frequency input is equal or greater than BCLK, i.e. `sample_rate_hz * slot_bits * 2`

mclk\_multiple [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_std_clk_config_t13mclk_multipleE "Permalink to this definition")  

The multiple of MCLK to the sample rate Default is 256 in the helper macro, it can satisfy most of cases, but please set this field a multiple of `3` (like 384) when using 24-bit data width, otherwise the sample rate might be inaccurate

uint32\_t bclk\_div [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_std_clk_config_t8bclk_divE "Permalink to this definition")  

The division from MCLK to BCLK, only take effect for slave role, it shouldn't be smaller than 8. Increase this field when data sent by slave lag behind

struct i2s\_std\_gpio\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv421i2s_std_gpio_config_t "Permalink to this definition")  

I2S standard mode GPIO pins configuration.

Public Members

gpio\_num\_t mclk [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_std_gpio_config_t4mclkE "Permalink to this definition")  

MCK pin, output by default, input if the clock source is selected to `I2S_CLK_SRC_EXTERNAL`

gpio\_num\_t bclk [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_std_gpio_config_t4bclkE "Permalink to this definition")  

BCK pin, input in slave role, output in master role

gpio\_num\_t ws [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_std_gpio_config_t2wsE "Permalink to this definition")  

WS pin, input in slave role, output in master role

gpio\_num\_t dout [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_std_gpio_config_t4doutE "Permalink to this definition")  

DATA pin, output

gpio\_num\_t din [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_std_gpio_config_t3dinE "Permalink to this definition")  

DATA pin, input

uint32\_t mclk\_inv [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_std_gpio_config_t8mclk_invE "Permalink to this definition")  

Set 1 to invert the MCLK input/output

uint32\_t bclk\_inv [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_std_gpio_config_t8bclk_invE "Permalink to this definition")  

Set 1 to invert the BCLK input/output

uint32\_t ws\_inv [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_std_gpio_config_t6ws_invE "Permalink to this definition")  

Set 1 to invert the WS input/output

struct invert\_flags [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_std_gpio_config_t12invert_flagsE "Permalink to this definition")  

GPIO pin invert flags

struct i2s\_std\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv416i2s_std_config_t "Permalink to this definition")  

I2S standard mode major configuration that including clock/slot/GPIO configuration.

Public Members

clk\_cfg [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N16i2s_std_config_t7clk_cfgE "Permalink to this definition")  

Standard mode clock configuration, can be generated by macro I2S\_STD\_CLK\_DEFAULT\_CONFIG

slot\_cfg [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N16i2s_std_config_t8slot_cfgE "Permalink to this definition")  

Standard mode slot configuration, can be generated by macros I2S\_STD\_\[mode\]\_SLOT\_DEFAULT\_CONFIG, \[mode\] can be replaced with PHILIPS/MSB/PCM

gpio\_cfg [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N16i2s_std_config_t8gpio_cfgE "Permalink to this definition")  

Standard mode GPIO configuration, specified by user

### Macros

I2S\_STD\_PHILIPS\_SLOT\_DEFAULT\_CONFIG(bits\_per\_sample, mono\_or\_stereo) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG "Permalink to this definition")  

Philips format in 2 slots.

This file is specified for I2S standard communication mode Features:

- Philips/MSB/PCM are supported in standard mode
- Fixed to 2 slots

Parameters:

- **bits\_per\_sample** -- I2S data bit width
- **mono\_or\_stereo** -- I2S\_SLOT\_MODE\_MONO or I2S\_SLOT\_MODE\_STEREO

I2S\_STD\_PCM\_SLOT\_DEFAULT\_CONFIG(bits\_per\_sample, mono\_or\_stereo) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_STD_PCM_SLOT_DEFAULT_CONFIG "Permalink to this definition")  

PCM(short) format in 2 slots.

Note

PCM(long) is same as Philips in 2 slots

Parameters:

- **bits\_per\_sample** -- I2S data bit width
- **mono\_or\_stereo** -- I2S\_SLOT\_MODE\_MONO or I2S\_SLOT\_MODE\_STEREO

I2S\_STD\_MSB\_SLOT\_DEFAULT\_CONFIG(bits\_per\_sample, mono\_or\_stereo) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_STD_MSB_SLOT_DEFAULT_CONFIG "Permalink to this definition")  

MSB format in 2 slots.

Parameters:

- **bits\_per\_sample** -- I2S data bit width
- **mono\_or\_stereo** -- I2S\_SLOT\_MODE\_MONO or I2S\_SLOT\_MODE\_STEREO

I2S\_STD\_CLK\_DEFAULT\_CONFIG(rate) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_STD_CLK_DEFAULT_CONFIG "Permalink to this definition")  

I2S default standard clock configuration.

Note

Please set the mclk\_multiple to I2S\_MCLK\_MULTIPLE\_384 while using 24 bits data width Otherwise the sample rate might be imprecise since the BCLK division is not a integer

Parameters:

- **rate** -- sample rate

### PDM Mode

### Header File

- [components/esp\_driver\_i2s/include/driver/i2s\_pdm.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_driver_i2s/include/driver/i2s_pdm.h)
- This header file can be included with:
	> ```c
	> #include "driver/i2s_pdm.h"
	> ```
- This header file is a part of the API provided by the `esp_driver_i2s` component. To declare that your component depends on `esp_driver_i2s`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES esp_driver_i2s
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES esp_driver_i2s
	> ```

### Functions

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_init\_pdm\_rx\_mode( handle, const \*pdm\_rx\_cfg) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv428i2s_channel_init_pdm_rx_mode17i2s_chan_handle_tPK19i2s_pdm_rx_config_t "Permalink to this definition")  

Initialize I2S channel to PDM RX mode.

Note

Only allowed to be called when the channel state is REGISTERED, (i.e., channel has been allocated, but not initialized) and the state will be updated to READY if initialization success, otherwise the state will return to REGISTERED.

Parameters:

- **handle** -- **\[in\]** I2S RX channel handler
- **pdm\_rx\_cfg** -- **\[in\]** Configurations for PDM RX mode, including clock, slot and GPIO The clock configuration can be generated by the helper macro `I2S_PDM_RX_CLK_DEFAULT_CONFIG` The slot configuration can be generated by the helper macro `I2S_PDM_RX_SLOT_RAW_FMT_DEFAULT_CONFIG` or `I2S_PDM_RX_SLOT_PCM_FMT_DEFAULT_CONFIG`

Returns:

- ESP\_OK Initialize successfully
- ESP\_ERR\_NO\_MEM No memory for storing the channel information
- ESP\_ERR\_INVALID\_ARG NULL pointer or invalid configuration
- ESP\_ERR\_INVALID\_STATE This channel is not registered

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_reconfig\_pdm\_rx\_clock( handle, const \*clk\_cfg) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv433i2s_channel_reconfig_pdm_rx_clock17i2s_chan_handle_tPK23i2s_pdm_rx_clk_config_t "Permalink to this definition")  

Reconfigure the I2S clock for PDM RX mode.

Note

Only allowed to be called when the channel state is READY, i.e., channel has been initialized, but not started this function won't change the state. `i2s_channel_disable` should be called before calling this function if I2S has started.

Note

The input channel handle has to be initialized to PDM RX mode, i.e., `i2s_channel_init_pdm_rx_mode` has been called before reconfiguring

Parameters:

- **handle** -- **\[in\]** I2S RX channel handler
- **clk\_cfg** -- **\[in\]** PDM RX mode clock configuration, can be generated by `I2S_PDM_RX_CLK_DEFAULT_CONFIG`

Returns:

- ESP\_OK Set clock successfully
- ESP\_ERR\_INVALID\_ARG NULL pointer, invalid configuration or not PDM mode
- ESP\_ERR\_INVALID\_STATE This channel is not initialized or not stopped

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_reconfig\_pdm\_rx\_slot( handle, const \*slot\_cfg) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv432i2s_channel_reconfig_pdm_rx_slot17i2s_chan_handle_tPK24i2s_pdm_rx_slot_config_t "Permalink to this definition")  

Reconfigure the I2S slot for PDM RX mode.

Note

Only allowed to be called when the channel state is READY, i.e., channel has been initialized, but not started this function won't change the state. `i2s_channel_disable` should be called before calling this function if I2S has started.

Note

The input channel handle has to be initialized to PDM RX mode, i.e., `i2s_channel_init_pdm_rx_mode` has been called before reconfiguring

Parameters:

- **handle** -- **\[in\]** I2S RX channel handler
- **slot\_cfg** -- **\[in\]** PDM RX mode slot configuration, can be generated by `I2S_PDM_RX_SLOT_RAW_FMT_DEFAULT_CONFIG` or `I2S_PDM_RX_SLOT_PCM_FMT_DEFAULT_CONFIG`

Returns:

- ESP\_OK Set clock successfully
- ESP\_ERR\_NO\_MEM No memory for DMA buffer
- ESP\_ERR\_INVALID\_ARG NULL pointer, invalid configuration or not PDM mode
- ESP\_ERR\_INVALID\_STATE This channel is not initialized or not stopped

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_reconfig\_pdm\_rx\_gpio( handle, const \*gpio\_cfg) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv432i2s_channel_reconfig_pdm_rx_gpio17i2s_chan_handle_tPK24i2s_pdm_rx_gpio_config_t "Permalink to this definition")  

Reconfigure the I2S GPIO for PDM RX mode.

Note

Only allowed to be called when the channel state is READY, i.e., channel has been initialized, but not started this function won't change the state. `i2s_channel_disable` should be called before calling this function if I2S has started.

Note

The input channel handle has to be initialized to PDM RX mode, i.e., `i2s_channel_init_pdm_rx_mode` has been called before reconfiguring

Parameters:

- **handle** -- **\[in\]** I2S RX channel handler
- **gpio\_cfg** -- **\[in\]** PDM RX mode GPIO configuration, specified by user

Returns:

- ESP\_OK Set clock successfully
- ESP\_ERR\_INVALID\_ARG NULL pointer, invalid configuration or not PDM mode
- ESP\_ERR\_INVALID\_STATE This channel is not initialized or not stopped

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_init\_pdm\_tx\_mode( handle, const \*pdm\_tx\_cfg) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv428i2s_channel_init_pdm_tx_mode17i2s_chan_handle_tPK19i2s_pdm_tx_config_t "Permalink to this definition")  

Initialize I2S channel to PDM TX mode.

Note

Only allowed to be called when the channel state is REGISTERED, (i.e., channel has been allocated, but not initialized) and the state will be updated to READY if initialization success, otherwise the state will return to REGISTERED.

Parameters:

- **handle** -- **\[in\]** I2S TX channel handler
- **pdm\_tx\_cfg** -- **\[in\]** Configurations for PDM TX mode, including clock, slot and GPIO The clock configuration can be generated by the helper macro `I2S_PDM_TX_CLK_DEFAULT_CONFIG` The slot configuration can be generated by the helper macro `I2S_PDM_TX_SLOT_DEFAULT_CONFIG`

Returns:

- ESP\_OK Initialize successfully
- ESP\_ERR\_NO\_MEM No memory for storing the channel information
- ESP\_ERR\_INVALID\_ARG NULL pointer or invalid configuration
- ESP\_ERR\_INVALID\_STATE This channel is not registered

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_reconfig\_pdm\_tx\_clock( handle, const \*clk\_cfg) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv433i2s_channel_reconfig_pdm_tx_clock17i2s_chan_handle_tPK23i2s_pdm_tx_clk_config_t "Permalink to this definition")  

Reconfigure the I2S clock for PDM TX mode.

Note

Only allowed to be called when the channel state is READY, i.e., channel has been initialized, but not started this function won't change the state. `i2s_channel_disable` should be called before calling this function if I2S has started.

Note

The input channel handle has to be initialized to PDM TX mode, i.e., `i2s_channel_init_pdm_tx_mode` has been called before reconfiguring

Parameters:

- **handle** -- **\[in\]** I2S TX channel handler
- **clk\_cfg** -- **\[in\]** PDM TX mode clock configuration, can be generated by `I2S_PDM_TX_CLK_DEFAULT_CONFIG`

Returns:

- ESP\_OK Set clock successfully
- ESP\_ERR\_INVALID\_ARG NULL pointer, invalid configuration or not PDM mode
- ESP\_ERR\_INVALID\_STATE This channel is not initialized or not stopped

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_reconfig\_pdm\_tx\_slot( handle, const \*slot\_cfg) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv432i2s_channel_reconfig_pdm_tx_slot17i2s_chan_handle_tPK24i2s_pdm_tx_slot_config_t "Permalink to this definition")  

Reconfigure the I2S slot for PDM TX mode.

Note

Only allowed to be called when the channel state is READY, i.e., channel has been initialized, but not started this function won't change the state. `i2s_channel_disable` should be called before calling this function if I2S has started.

Note

The input channel handle has to be initialized to PDM TX mode, i.e., `i2s_channel_init_pdm_tx_mode` has been called before reconfiguring

Parameters:

- **handle** -- **\[in\]** I2S TX channel handler
- **slot\_cfg** -- **\[in\]** PDM TX mode slot configuration, can be generated by `I2S_PDM_TX_SLOT_DEFAULT_CONFIG`

Returns:

- ESP\_OK Set clock successfully
- ESP\_ERR\_NO\_MEM No memory for DMA buffer
- ESP\_ERR\_INVALID\_ARG NULL pointer, invalid configuration or not PDM mode
- ESP\_ERR\_INVALID\_STATE This channel is not initialized or not stopped

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_reconfig\_pdm\_tx\_gpio( handle, const \*gpio\_cfg) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv432i2s_channel_reconfig_pdm_tx_gpio17i2s_chan_handle_tPK24i2s_pdm_tx_gpio_config_t "Permalink to this definition")  

Reconfigure the I2S GPIO for PDM TX mode.

Note

Only allowed to be called when the channel state is READY, i.e., channel has been initialized, but not started this function won't change the state. `i2s_channel_disable` should be called before calling this function if I2S has started.

Note

The input channel handle has to be initialized to PDM TX mode, i.e., `i2s_channel_init_pdm_tx_mode` has been called before reconfiguring

Parameters:

- **handle** -- **\[in\]** I2S TX channel handler
- **gpio\_cfg** -- **\[in\]** PDM TX mode GPIO configuration, specified by user

Returns:

- ESP\_OK Set clock successfully
- ESP\_ERR\_INVALID\_ARG NULL pointer, invalid configuration or not PDM mode
- ESP\_ERR\_INVALID\_STATE This channel is not initialized or not stopped

### Structures

struct i2s\_pdm\_rx\_slot\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv424i2s_pdm_rx_slot_config_t "Permalink to this definition")  

I2S slot configuration for PDM RX mode.

Public Members

data\_bit\_width [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_rx_slot_config_t14data_bit_widthE "Permalink to this definition")  

I2S sample data bit width (valid data bits per sample), only support 16 bits for PDM mode

slot\_bit\_width [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_rx_slot_config_t14slot_bit_widthE "Permalink to this definition")  

I2S slot bit width (total bits per slot), only support 16 bits for PDM mode

slot\_mode [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_rx_slot_config_t9slot_modeE "Permalink to this definition")  

Set mono or stereo mode with I2S\_SLOT\_MODE\_MONO or I2S\_SLOT\_MODE\_STEREO

slot\_mask [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_rx_slot_config_t9slot_maskE "Permalink to this definition")  

Choose the slots to activate

data\_fmt [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_rx_slot_config_t8data_fmtE "Permalink to this definition")  

The data format of PDM RX mode. It determines what kind of data format is read in software. Typically, set this field to I2S\_PDM\_DATA\_FMT\_PCM when PCM2PDM filter is supported in the hardware, so that the hardware PDM2PCM filter will help to convert the raw PDM data on the line into PCM format, And then you can read PCM format data in software. Otherwise if this field is set to I2S\_PDM\_DATA\_FMT\_RAW, The data read in software are still in raw PDM format, you may need to convert the raw PDM data into PCM format manually by a software filter.

bool hp\_en [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_rx_slot_config_t5hp_enE "Permalink to this definition")  

High pass filter enable

float hp\_cut\_off\_freq\_hz [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_rx_slot_config_t18hp_cut_off_freq_hzE "Permalink to this definition")  

High pass filter cut-off frequency, range 23.3Hz ~ 185Hz, see cut-off frequency sheet above

uint32\_t amplify\_num [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_rx_slot_config_t11amplify_numE "Permalink to this definition")  

The amplification number of the final conversion result. The data that have converted from PDM to PCM module, will time `amplify_num` additionally to amplify the final result. Note that it's only a multiplier of the digital PCM data, not the gain of the analog signal range 1~15, default 1

struct i2s\_pdm\_rx\_clk\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv423i2s_pdm_rx_clk_config_t "Permalink to this definition")  

I2S clock configuration for PDM RX mode.

Public Members

uint32\_t sample\_rate\_hz [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N23i2s_pdm_rx_clk_config_t14sample_rate_hzE "Permalink to this definition")  

I2S sample rate.

- For raw PDM mode, it typically ranges 1.024MHz ~ 6.144MHz.
- For PCM mode (PDM2PCM filter enabled), it usually ranges 16KHz ~ 48KHz

clk\_src [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N23i2s_pdm_rx_clk_config_t7clk_srcE "Permalink to this definition")  

Choose clock source

mclk\_multiple [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N23i2s_pdm_rx_clk_config_t13mclk_multipleE "Permalink to this definition")  

The multiple of MCLK to the sample rate

dn\_sample\_mode [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N23i2s_pdm_rx_clk_config_t14dn_sample_modeE "Permalink to this definition")  

Down-sampling rate mode

uint32\_t bclk\_div [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N23i2s_pdm_rx_clk_config_t8bclk_divE "Permalink to this definition")  

The division from MCLK to BCLK. The typical and minimum value is I2S\_PDM\_RX\_BCLK\_DIV\_MIN. It will be set to I2S\_PDM\_RX\_BCLK\_DIV\_MIN by default if it is smaller than I2S\_PDM\_RX\_BCLK\_DIV\_MIN

struct i2s\_pdm\_rx\_gpio\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv424i2s_pdm_rx_gpio_config_t "Permalink to this definition")  

I2S PDM RX mode GPIO pins configuration.

Public Members

gpio\_num\_t clk [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_rx_gpio_config_t3clkE "Permalink to this definition")  

PDM clk pin, output

gpio\_num\_t din [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_rx_gpio_config_t3dinE "Permalink to this definition")  

DATA pin 0, input

gpio\_num\_t dins\[(4)\] [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_rx_gpio_config_t4dinsE "Permalink to this definition")  

DATA pins, input, only take effect when corresponding I2S\_PDM\_RX\_LINEx\_SLOT\_xxx is enabled in

uint32\_t clk\_inv [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_rx_gpio_config_t7clk_invE "Permalink to this definition")  

Set 1 to invert the clk output

struct invert\_flags [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_rx_gpio_config_t12invert_flagsE "Permalink to this definition")  

GPIO pin invert flags

struct i2s\_pdm\_rx\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv419i2s_pdm_rx_config_t "Permalink to this definition")  

I2S PDM RX mode major configuration that including clock/slot/GPIO configuration.

Public Members

clk\_cfg [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_rx_config_t7clk_cfgE "Permalink to this definition")  

PDM RX clock configurations, can be generated by macro I2S\_PDM\_RX\_CLK\_DEFAULT\_CONFIG

slot\_cfg [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_rx_config_t8slot_cfgE "Permalink to this definition")  

PDM RX slot configurations, can be generated by macro I2S\_PDM\_RX\_SLOT\_RAW\_FMT\_DEFAULT\_CONFIG or I2S\_PDM\_RX\_SLOT\_PCM\_FMT\_DEFAULT\_CONFIG

gpio\_cfg [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_rx_config_t8gpio_cfgE "Permalink to this definition")  

PDM RX slot configurations, specified by user

struct i2s\_pdm\_tx\_slot\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv424i2s_pdm_tx_slot_config_t "Permalink to this definition")  

I2S slot configuration for PDM TX mode.

Public Members

data\_bit\_width [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_tx_slot_config_t14data_bit_widthE "Permalink to this definition")  

I2S sample data bit width (valid data bits per sample), only support 16 bits for PDM mode

slot\_bit\_width [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_tx_slot_config_t14slot_bit_widthE "Permalink to this definition")  

I2S slot bit width (total bits per slot), only support 16 bits for PDM mode

slot\_mode [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_tx_slot_config_t9slot_modeE "Permalink to this definition")  

Set mono or stereo mode with I2S\_SLOT\_MODE\_MONO or I2S\_SLOT\_MODE\_STEREO For PDM TX mode, mono means the data buffer only contains one slot data, Stereo means the data buffer contains two slots data

data\_fmt [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_tx_slot_config_t8data_fmtE "Permalink to this definition")  

The data format of PDM TX mode. It determines what kind of data format is written in software. Typically, set this field to I2S\_PDM\_DATA\_FMT\_PCM when PCM2PDM filter is supported in the hardware, so that you can write PCM format data in software, and then the hardware PCM2PDM filter will help to convert it into PDM format on the line. Otherwise if this field is set to I2S\_PDM\_DATA\_FMT\_RAW, The data written in software are supposed to be the raw PDM format.

uint32\_t sd\_prescale [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_tx_slot_config_t11sd_prescaleE "Permalink to this definition")  

Sigma-delta filter prescale

sd\_scale [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_tx_slot_config_t8sd_scaleE "Permalink to this definition")  

Sigma-delta filter scaling value

hp\_scale [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_tx_slot_config_t8hp_scaleE "Permalink to this definition")  

High pass filter scaling value

lp\_scale [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_tx_slot_config_t8lp_scaleE "Permalink to this definition")  

Low pass filter scaling value

sinc\_scale [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_tx_slot_config_t10sinc_scaleE "Permalink to this definition")  

Sinc filter scaling value

line\_mode [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_tx_slot_config_t9line_modeE "Permalink to this definition")  

PDM TX line mode, one-line codec, one-line dac, two-line dac mode can be selected

bool hp\_en [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_tx_slot_config_t5hp_enE "Permalink to this definition")  

High pass filter enable

float hp\_cut\_off\_freq\_hz [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_tx_slot_config_t18hp_cut_off_freq_hzE "Permalink to this definition")  

High pass filter cut-off frequency, range 23.3Hz ~ 185Hz, see cut-off frequency sheet above

uint32\_t sd\_dither [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_tx_slot_config_t9sd_ditherE "Permalink to this definition")  

Sigma-delta filter dither

uint32\_t sd\_dither2 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_tx_slot_config_t10sd_dither2E "Permalink to this definition")  

Sigma-delta filter dither2

struct i2s\_pdm\_tx\_clk\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv423i2s_pdm_tx_clk_config_t "Permalink to this definition")  

I2S clock configuration for PDM TX mode.

Public Members

uint32\_t sample\_rate\_hz [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N23i2s_pdm_tx_clk_config_t14sample_rate_hzE "Permalink to this definition")  

I2S sample rate.

- For raw PDM mode, it typically ranges 1.024MHz ~ 6.144MHz.
- For PCM mode (PCM2PDM filter enabled), it usually ranges 16KHz ~ 48KHz

clk\_src [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N23i2s_pdm_tx_clk_config_t7clk_srcE "Permalink to this definition")  

Choose clock source

mclk\_multiple [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N23i2s_pdm_tx_clk_config_t13mclk_multipleE "Permalink to this definition")  

The multiple of MCLK to the sample rate

uint32\_t up\_sample\_fp [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N23i2s_pdm_tx_clk_config_t12up_sample_fpE "Permalink to this definition")  

Up-sampling param fp

uint32\_t up\_sample\_fs [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N23i2s_pdm_tx_clk_config_t12up_sample_fsE "Permalink to this definition")  

Up-sampling param fs, not allowed to be greater than 480

uint32\_t bclk\_div [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N23i2s_pdm_tx_clk_config_t8bclk_divE "Permalink to this definition")  

The division from MCLK to BCLK. The minimum value is I2S\_PDM\_TX\_BCLK\_DIV\_MIN. It will be set to I2S\_PDM\_TX\_BCLK\_DIV\_MIN by default if it is smaller than I2S\_PDM\_TX\_BCLK\_DIV\_MIN

struct i2s\_pdm\_tx\_gpio\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv424i2s_pdm_tx_gpio_config_t "Permalink to this definition")  

I2S PDM TX mode GPIO pins configuration.

Public Members

gpio\_num\_t clk [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_tx_gpio_config_t3clkE "Permalink to this definition")  

PDM clk pin, output

gpio\_num\_t dout [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_tx_gpio_config_t4doutE "Permalink to this definition")  

DATA pin, output

gpio\_num\_t dout2 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_tx_gpio_config_t5dout2E "Permalink to this definition")  

The second data pin for the DAC dual-line mode, only take effect when the line mode is `I2S_PDM_TX_TWO_LINE_DAC`

uint32\_t clk\_inv [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_tx_gpio_config_t7clk_invE "Permalink to this definition")  

Set 1 to invert the clk output

struct invert\_flags [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N24i2s_pdm_tx_gpio_config_t12invert_flagsE "Permalink to this definition")  

GPIO pin invert flags

struct i2s\_pdm\_tx\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv419i2s_pdm_tx_config_t "Permalink to this definition")  

I2S PDM TX mode major configuration that including clock/slot/GPIO configuration.

Public Members

clk\_cfg [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_tx_config_t7clk_cfgE "Permalink to this definition")  

PDM TX clock configurations, can be generated by macro I2S\_PDM\_TX\_CLK\_DEFAULT\_CONFIG

slot\_cfg [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_tx_config_t8slot_cfgE "Permalink to this definition")  

PDM TX slot configurations, can be generated by macro I2S\_PDM\_TX\_SLOT\_DEFAULT\_CONFIG

gpio\_cfg [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_tx_config_t8gpio_cfgE "Permalink to this definition")  

PDM TX GPIO configurations, specified by user

### Macros

I2S\_PDM\_RX\_SLOT\_PCM\_FMT\_DEFAULT\_CONFIG(bits\_per\_sample, mono\_or\_stereo) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_PDM_RX_SLOT_PCM_FMT_DEFAULT_CONFIG "Permalink to this definition")  

PDM mode in 2 slots(RX). Read data in PCM format.

This file is specified for I2S PDM communication mode Features:

- Only support PDM TX/RX mode
- Fixed to 2 slots
- Data bit width only support 16 bits

Parameters:

- **bits\_per\_sample** -- I2S data bit width, only support 16 bits for PDM mode
- **mono\_or\_stereo** -- I2S\_SLOT\_MODE\_MONO or I2S\_SLOT\_MODE\_STEREO

I2S\_PDM\_RX\_SLOT\_RAW\_FMT\_DEFAULT\_CONFIG(bits\_per\_sample, mono\_or\_stereo) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_PDM_RX_SLOT_RAW_FMT_DEFAULT_CONFIG "Permalink to this definition")  

PDM mode in 2 slots(RX). Read data in raw PDM format.

Parameters:

- **bits\_per\_sample** -- I2S data bit width, only support 16 bits for PDM mode
- **mono\_or\_stereo** -- I2S\_SLOT\_MODE\_MONO or I2S\_SLOT\_MODE\_STEREO

I2S\_PDM\_RX\_CLK\_DEFAULT\_CONFIG(rate) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_PDM_RX_CLK_DEFAULT_CONFIG "Permalink to this definition")  

I2S default PDM RX clock configuration.

Parameters:

- **rate** -- sample rate

I2S\_PDM\_TX\_SLOT\_PCM\_FMT\_DEFAULT\_CONFIG(bits\_per\_sample, mono\_or\_stereo) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_PDM_TX_SLOT_PCM_FMT_DEFAULT_CONFIG "Permalink to this definition")  

PDM style in 2 slots(TX) for codec line mode. Write PCM data.

Parameters:

- **bits\_per\_sample** -- I2S data bit width, only support 16 bits for PDM mode
- **mono\_or\_stereo** -- I2S\_SLOT\_MODE\_MONO or I2S\_SLOT\_MODE\_STEREO

I2S\_PDM\_TX\_SLOT\_RAW\_FMT\_DEFAULT\_CONFIG(bits\_per\_sample, mono\_or\_stereo) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_PDM_TX_SLOT_RAW_FMT_DEFAULT_CONFIG "Permalink to this definition")  

PDM style in 2 slots(TX) for codec line mode. Write raw PDM data.

Parameters:

- **bits\_per\_sample** -- I2S data bit width, only support 16 bits for PDM mode
- **mono\_or\_stereo** -- I2S\_SLOT\_MODE\_MONO or I2S\_SLOT\_MODE\_STEREO

I2S\_PDM\_TX\_SLOT\_PCM\_FMT\_DAC\_DEFAULT\_CONFIG(bits\_per\_sample, mono\_or\_stereo) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_PDM_TX_SLOT_PCM_FMT_DAC_DEFAULT_CONFIG "Permalink to this definition")  

PDM style in 1 slots(TX) for DAC line mode.

Note

The noise might be different with different configurations, this macro provides a set of configurations that have relatively high SNR (Signal Noise Ratio), you can also adjust them to fit your case.

Parameters:

- **bits\_per\_sample** -- I2S data bit width, only support 16 bits for PDM mode
- **mono\_or\_stereo** -- I2S\_SLOT\_MODE\_MONO or I2S\_SLOT\_MODE\_STEREO

I2S\_PDM\_TX\_SLOT\_RAW\_FMT\_DAC\_DEFAULT\_CONFIG(bits\_per\_sample, mono\_or\_stereo) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_PDM_TX_SLOT_RAW_FMT_DAC_DEFAULT_CONFIG "Permalink to this definition")  

PDM style in 1 slots(TX) for DAC line mode. Write raw PDM data.

Note

The noise might be different with different configurations, this macro provides a set of configurations that have relatively high SNR (Signal Noise Ratio), you can also adjust them to fit your case.

Parameters:

- **bits\_per\_sample** -- I2S data bit width, only support 16 bits for PDM mode
- **mono\_or\_stereo** -- I2S\_SLOT\_MODE\_MONO or I2S\_SLOT\_MODE\_STEREO

I2S\_PDM\_TX\_CLK\_DEFAULT\_CONFIG(rate) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_PDM_TX_CLK_DEFAULT_CONFIG "Permalink to this definition")  

I2S default PDM TX clock configuration for codec line mode.

Note

TX PDM can only be set to the following two up-sampling rate configurations: 1: fp = 960, fs = sample\_rate\_hz / 100, in this case, Fpdm = 128\*48000 2: fp = 960, fs = 480, in this case, Fpdm = 128\*Fpcm = 128\*sample\_rate\_hz If the PDM receiver do not care the PDM serial clock, it's recommended set Fpdm = 128\*48000. Otherwise, the second configuration should be adopted.

Parameters:

- **rate** -- sample rate (not suggest to exceed 48000 Hz, otherwise more glitches and noise may appear)

I2S\_PDM\_TX\_CLK\_DAC\_DEFAULT\_CONFIG(rate) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_PDM_TX_CLK_DAC_DEFAULT_CONFIG "Permalink to this definition")  

I2S default PDM TX clock configuration for DAC line mode.

Note

TX PDM can only be set to the following two up-sampling rate configurations: 1: fp = 960, fs = sample\_rate\_hz / 100, in this case, Fpdm = 128\*48000 2: fp = 960, fs = 480, in this case, Fpdm = 128\*Fpcm = 128\*sample\_rate\_hz If the PDM receiver do not care the PDM serial clock, it's recommended set Fpdm = 128\*48000. Otherwise, the second configuration should be adopted.

Note

The noise might be different with different configurations, this macro provides a set of configurations that have relatively high SNR (Signal Noise Ratio), you can also adjust them to fit your case.

Parameters:

- **rate** -- sample rate (not suggest to exceed 48000 Hz, otherwise more glitches and noise may appear)

### TDM Mode

### Header File

- [components/esp\_driver\_i2s/include/driver/i2s\_tdm.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_driver_i2s/include/driver/i2s_tdm.h)
- This header file can be included with:
	> ```c
	> #include "driver/i2s_tdm.h"
	> ```
- This header file is a part of the API provided by the `esp_driver_i2s` component. To declare that your component depends on `esp_driver_i2s`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES esp_driver_i2s
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES esp_driver_i2s
	> ```

### Functions

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_init\_tdm\_mode( handle, const \*tdm\_cfg) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv425i2s_channel_init_tdm_mode17i2s_chan_handle_tPK16i2s_tdm_config_t "Permalink to this definition")  

Initialize I2S channel to TDM mode.

Note

Only allowed to be called when the channel state is REGISTERED, (i.e., channel has been allocated, but not initialized) and the state will be updated to READY if initialization success, otherwise the state will return to REGISTERED.

Note

When initialize the TDM mode with a same configuration as another channel on a same port, these two channels can constitude as full-duplex mode automatically

Parameters:

- **handle** -- **\[in\]** I2S channel handler
- **tdm\_cfg** -- **\[in\]** Configurations for TDM mode, including clock, slot and GPIO The clock configuration can be generated by the helper macro `I2S_TDM_CLK_DEFAULT_CONFIG` The slot configuration can be generated by the helper macro `I2S_TDM_PHILIPS_SLOT_DEFAULT_CONFIG`, `I2S_TDM_PCM_SHORT_SLOT_DEFAULT_CONFIG`, `I2S_TDM_PCM_LONG_SLOT_DEFAULT_CONFIG` or `I2S_TDM_MSB_SLOT_DEFAULT_CONFIG`

Returns:

- ESP\_OK Initialize successfully
- ESP\_ERR\_NO\_MEM No memory for storing the channel information
- ESP\_ERR\_INVALID\_ARG NULL pointer or invalid configuration
- ESP\_ERR\_INVALID\_STATE This channel is not registered

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_reconfig\_tdm\_clock( handle, const \*clk\_cfg) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv430i2s_channel_reconfig_tdm_clock17i2s_chan_handle_tPK20i2s_tdm_clk_config_t "Permalink to this definition")  

Reconfigure the I2S clock for TDM mode.

Note

Only allowed to be called when the channel state is READY, i.e., channel has been initialized, but not started this function won't change the state. `i2s_channel_disable` should be called before calling this function if I2S has started.

Note

The input channel handle has to be initialized to TDM mode, i.e., `i2s_channel_init_tdm_mode` has been called before reconfiguring

Parameters:

- **handle** -- **\[in\]** I2S channel handler
- **clk\_cfg** -- **\[in\]** Standard mode clock configuration, can be generated by `I2S_TDM_CLK_DEFAULT_CONFIG`

Returns:

- ESP\_OK Set clock successfully
- ESP\_ERR\_INVALID\_ARG NULL pointer, invalid configuration or not TDM mode
- ESP\_ERR\_INVALID\_STATE This channel is not initialized or not stopped

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_reconfig\_tdm\_slot( handle, const \*slot\_cfg) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv429i2s_channel_reconfig_tdm_slot17i2s_chan_handle_tPK21i2s_tdm_slot_config_t "Permalink to this definition")  

Reconfigure the I2S slot for TDM mode.

Note

Only allowed to be called when the channel state is READY, i.e., channel has been initialized, but not started this function won't change the state. `i2s_channel_disable` should be called before calling this function if I2S has started.

Note

The input channel handle has to be initialized to TDM mode, i.e., `i2s_channel_init_tdm_mode` has been called before reconfiguring

Parameters:

- **handle** -- **\[in\]** I2S channel handler
- **slot\_cfg** -- **\[in\]** Standard mode slot configuration, can be generated by `I2S_TDM_PHILIPS_SLOT_DEFAULT_CONFIG`, `I2S_TDM_PCM_SHORT_SLOT_DEFAULT_CONFIG`, `I2S_TDM_PCM_LONG_SLOT_DEFAULT_CONFIG` or `I2S_TDM_MSB_SLOT_DEFAULT_CONFIG`.

Returns:

- ESP\_OK Set clock successfully
- ESP\_ERR\_NO\_MEM No memory for DMA buffer
- ESP\_ERR\_INVALID\_ARG NULL pointer, invalid configuration or not TDM mode
- ESP\_ERR\_INVALID\_STATE This channel is not initialized or not stopped

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_reconfig\_tdm\_gpio( handle, const \*gpio\_cfg) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv429i2s_channel_reconfig_tdm_gpio17i2s_chan_handle_tPK21i2s_tdm_gpio_config_t "Permalink to this definition")  

Reconfigure the I2S GPIO for TDM mode.

Note

Only allowed to be called when the channel state is READY, i.e., channel has been initialized, but not started this function won't change the state. `i2s_channel_disable` should be called before calling this function if I2S has started.

Note

The input channel handle has to be initialized to TDM mode, i.e., `i2s_channel_init_tdm_mode` has been called before reconfiguring

Parameters:

- **handle** -- **\[in\]** I2S channel handler
- **gpio\_cfg** -- **\[in\]** Standard mode GPIO configuration, specified by user

Returns:

- ESP\_OK Set clock successfully
- ESP\_ERR\_INVALID\_ARG NULL pointer, invalid configuration or not TDM mode
- ESP\_ERR\_INVALID\_STATE This channel is not initialized or not stopped

### Structures

struct i2s\_tdm\_slot\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv421i2s_tdm_slot_config_t "Permalink to this definition")  

I2S slot configuration for TDM mode.

Public Members

data\_bit\_width [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_slot_config_t14data_bit_widthE "Permalink to this definition")  

I2S sample data bit width (valid data bits per sample)

slot\_bit\_width [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_slot_config_t14slot_bit_widthE "Permalink to this definition")  

I2S slot bit width (total bits per slot)

slot\_mode [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_slot_config_t9slot_modeE "Permalink to this definition")  

Set mono or stereo mode with I2S\_SLOT\_MODE\_MONO or I2S\_SLOT\_MODE\_STEREO

slot\_mask [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_slot_config_t9slot_maskE "Permalink to this definition")  

Slot mask. Activating slots by setting 1 to corresponding bits. When the activated slots is not consecutive, those data in inactivated slots will be ignored

uint32\_t ws\_width [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_slot_config_t8ws_widthE "Permalink to this definition")  

WS signal width (i.e. the number of BCLK ticks that WS signal is high)

bool ws\_pol [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_slot_config_t6ws_polE "Permalink to this definition")  

WS signal polarity, set true to enable high lever first

bool bit\_shift [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_slot_config_t9bit_shiftE "Permalink to this definition")  

Set true to enable bit shift in Philips mode

bool left\_align [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_slot_config_t10left_alignE "Permalink to this definition")  

Set true to enable left alignment

bool big\_endian [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_slot_config_t10big_endianE "Permalink to this definition")  

Set true to enable big endian

bool bit\_order\_lsb [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_slot_config_t13bit_order_lsbE "Permalink to this definition")  

Set true to enable lsb first

bool skip\_mask [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_slot_config_t9skip_maskE "Permalink to this definition")  

Set true to enable skip mask. If it is enabled, only the data of the enabled channels will be sent, otherwise all data stored in DMA TX buffer will be sent

uint32\_t total\_slot [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_slot_config_t10total_slotE "Permalink to this definition")  

I2S total number of slots. If it is smaller than the biggest activated channel number, it will be set to this number automatically.

struct i2s\_tdm\_clk\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv420i2s_tdm_clk_config_t "Permalink to this definition")  

I2S clock configuration for TDM mode.

Public Members

uint32\_t sample\_rate\_hz [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_tdm_clk_config_t14sample_rate_hzE "Permalink to this definition")  

I2S sample rate

clk\_src [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_tdm_clk_config_t7clk_srcE "Permalink to this definition")  

Choose clock source, see `soc_periph_i2s_clk_src_t` for the supported clock sources. selected `I2S_CLK_SRC_EXTERNAL` (if supports) to enable the external source clock inputted via MCLK pin, please make sure the frequency inputted is equal or greater than `sample_rate_hz * mclk_multiple`

uint32\_t ext\_clk\_freq\_hz [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_tdm_clk_config_t15ext_clk_freq_hzE "Permalink to this definition")  

External clock source frequency in Hz, only take effect when `clk_src = I2S_CLK_SRC_EXTERNAL`, otherwise this field will be ignored Please make sure the frequency inputted is equal or greater than BCLK, i.e. `sample_rate_hz * slot_bits * slot_num`

mclk\_multiple [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_tdm_clk_config_t13mclk_multipleE "Permalink to this definition")  

The multiple of MCLK to the sample rate, only take effect for master role

uint32\_t bclk\_div [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_tdm_clk_config_t8bclk_divE "Permalink to this definition")  

The division from MCLK to BCLK, only take effect for slave role, it shouldn't be smaller than 8. Increase this field when data sent by slave lag behind

struct i2s\_tdm\_gpio\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv421i2s_tdm_gpio_config_t "Permalink to this definition")  

I2S TDM mode GPIO pins configuration.

Public Members

gpio\_num\_t mclk [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_gpio_config_t4mclkE "Permalink to this definition")  

MCK pin, output by default, input if the clock source is selected to `I2S_CLK_SRC_EXTERNAL`

gpio\_num\_t bclk [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_gpio_config_t4bclkE "Permalink to this definition")  

BCK pin, input in slave role, output in master role

gpio\_num\_t ws [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_gpio_config_t2wsE "Permalink to this definition")  

WS pin, input in slave role, output in master role

gpio\_num\_t dout [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_gpio_config_t4doutE "Permalink to this definition")  

DATA pin, output

gpio\_num\_t din [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_gpio_config_t3dinE "Permalink to this definition")  

DATA pin, input

uint32\_t mclk\_inv [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_gpio_config_t8mclk_invE "Permalink to this definition")  

Set 1 to invert the MCLK input/output

uint32\_t bclk\_inv [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_gpio_config_t8bclk_invE "Permalink to this definition")  

Set 1 to invert the BCLK input/output

uint32\_t ws\_inv [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_gpio_config_t6ws_invE "Permalink to this definition")  

Set 1 to invert the WS input/output

struct invert\_flags [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_tdm_gpio_config_t12invert_flagsE "Permalink to this definition")  

GPIO pin invert flags

struct i2s\_tdm\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv416i2s_tdm_config_t "Permalink to this definition")  

I2S TDM mode major configuration that including clock/slot/GPIO configuration.

Public Members

clk\_cfg [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N16i2s_tdm_config_t7clk_cfgE "Permalink to this definition")  

TDM mode clock configuration, can be generated by macro I2S\_TDM\_CLK\_DEFAULT\_CONFIG

slot\_cfg [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N16i2s_tdm_config_t8slot_cfgE "Permalink to this definition")  

TDM mode slot configuration, can be generated by macros I2S\_TDM\_\[mode\]\_SLOT\_DEFAULT\_CONFIG, \[mode\] can be replaced with PHILIPS/MSB/PCM\_SHORT/PCM\_LONG

gpio\_cfg [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N16i2s_tdm_config_t8gpio_cfgE "Permalink to this definition")  

TDM mode GPIO configuration, specified by user

### Macros

I2S\_TDM\_AUTO\_SLOT\_NUM [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_TDM_AUTO_SLOT_NUM "Permalink to this definition")  

This file is specified for I2S TDM communication mode Features:

- More than 2 slots

I2S\_TDM\_AUTO\_WS\_WIDTH [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_TDM_AUTO_WS_WIDTH "Permalink to this definition")  

I2S\_TDM\_PHILIPS\_SLOT\_DEFAULT\_CONFIG(bits\_per\_sample, mono\_or\_stereo, mask) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_TDM_PHILIPS_SLOT_DEFAULT_CONFIG "Permalink to this definition")  

Philips format in active slot that enabled by mask.

Parameters:

- **bits\_per\_sample** -- I2S data bit width
- **mono\_or\_stereo** -- I2S\_SLOT\_MODE\_MONO or I2S\_SLOT\_MODE\_STEREO
- **mask** -- active slot mask

I2S\_TDM\_MSB\_SLOT\_DEFAULT\_CONFIG(bits\_per\_sample, mono\_or\_stereo, mask) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_TDM_MSB_SLOT_DEFAULT_CONFIG "Permalink to this definition")  

MSB format in active slot enabled that by mask.

Parameters:

- **bits\_per\_sample** -- I2S data bit width
- **mono\_or\_stereo** -- I2S\_SLOT\_MODE\_MONO or I2S\_SLOT\_MODE\_STEREO
- **mask** -- active slot mask

I2S\_TDM\_PCM\_SHORT\_SLOT\_DEFAULT\_CONFIG(bits\_per\_sample, mono\_or\_stereo, mask) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_TDM_PCM_SHORT_SLOT_DEFAULT_CONFIG "Permalink to this definition")  

PCM(short) format in active slot that enabled by mask.

Parameters:

- **bits\_per\_sample** -- I2S data bit width
- **mono\_or\_stereo** -- I2S\_SLOT\_MODE\_MONO or I2S\_SLOT\_MODE\_STEREO
- **mask** -- active slot mask

I2S\_TDM\_PCM\_LONG\_SLOT\_DEFAULT\_CONFIG(bits\_per\_sample, mono\_or\_stereo, mask) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_TDM_PCM_LONG_SLOT_DEFAULT_CONFIG "Permalink to this definition")  

PCM(long) format in active slot that enabled by mask.

Parameters:

- **bits\_per\_sample** -- I2S data bit width
- **mono\_or\_stereo** -- I2S\_SLOT\_MODE\_MONO or I2S\_SLOT\_MODE\_STEREO
- **mask** -- active slot mask

I2S\_TDM\_CLK\_DEFAULT\_CONFIG(rate) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_TDM_CLK_DEFAULT_CONFIG "Permalink to this definition")  

I2S default TDM clock configuration.

Note

Please set the mclk\_multiple to I2S\_MCLK\_MULTIPLE\_384 while the data width in slot configuration is set to 24 bits Otherwise the sample rate might be imprecise since the BCLK division is not a integer

Parameters:

- **rate** -- sample rate

### I2S Driver

### Header File

- [components/esp\_driver\_i2s/include/driver/i2s\_common.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_driver_i2s/include/driver/i2s_common.h)
- This header file can be included with:
	> ```c
	> #include "driver/i2s_common.h"
	> ```
- This header file is a part of the API provided by the `esp_driver_i2s` component. To declare that your component depends on `esp_driver_i2s`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES esp_driver_i2s
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES esp_driver_i2s
	> ```

### Functions

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_new\_channel(const \*chan\_cfg, \*ret\_tx\_handle, \*ret\_rx\_handle) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv415i2s_new_channelPK17i2s_chan_config_tP17i2s_chan_handle_tP17i2s_chan_handle_t "Permalink to this definition")  

Allocate new I2S channel(s)

Note

The new created I2S channel handle will be REGISTERED state after it is allocated successfully.

Note

When the port id in channel configuration is I2S\_NUM\_AUTO, driver will allocate I2S port automatically on one of the I2S controller, otherwise driver will try to allocate the new channel on the selected port.

Note

If both tx\_handle and rx\_handle are not NULL, it means this I2S controller will work at full-duplex mode, the RX and TX channels will be allocated on a same I2S port in this case. Note that some configurations of TX/RX channel are shared on ESP32 and ESP32S2, so please make sure they are working at same condition and under same status(start/stop). Currently, full-duplex mode can't guarantee TX/RX channels write/read synchronously, they can only share the clock signals for now.

Note

If tx\_handle OR rx\_handle is NULL, it means this I2S controller will work at simplex mode. For ESP32 and ESP32S2, the whole I2S controller (i.e. both RX and TX channel) will be occupied, even if only one of RX or TX channel is registered. For the other targets, another channel on this controller will still available.

Parameters:

- **chan\_cfg** -- **\[in\]** I2S controller channel configurations
- **ret\_tx\_handle** -- **\[out\]** I2S channel handler used for managing the sending channel(optional)
- **ret\_rx\_handle** -- **\[out\]** I2S channel handler used for managing the receiving channel(optional)

Returns:

- ESP\_OK Allocate new channel(s) success
- ESP\_ERR\_NOT\_SUPPORTED The communication mode is not supported on the current chip
- ESP\_ERR\_INVALID\_ARG NULL pointer or illegal parameter in
- ESP\_ERR\_NOT\_FOUND No available I2S channel found

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_del\_channel( handle) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv415i2s_del_channel17i2s_chan_handle_t "Permalink to this definition")  

Delete the I2S channel.

Note

Only allowed to be called when the I2S channel is at REGISTERED or READY state (i.e., it should stop before deleting it).

Note

Resource will be free automatically if all channels in one port are deleted

Parameters:

**handle** -- **\[in\]** I2S channel handler

- ESP\_OK Delete successfully
- ESP\_ERR\_INVALID\_ARG NULL pointer

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_get\_info( handle, \*chan\_info) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv420i2s_channel_get_info17i2s_chan_handle_tP15i2s_chan_info_t "Permalink to this definition")  

Get I2S channel information.

Parameters:

- **handle** -- **\[in\]** I2S channel handler
- **chan\_info** -- **\[out\]** I2S channel basic information

Returns:

- ESP\_OK Get I2S channel information success
- ESP\_ERR\_NOT\_FOUND The input handle doesn't match any registered I2S channels, it may not an I2S channel handle or not available any more
- ESP\_ERR\_INVALID\_ARG The input handle or chan\_info pointer is NULL

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_enable( handle) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv418i2s_channel_enable17i2s_chan_handle_t "Permalink to this definition")  

Enable the I2S channel.

Note

Only allowed to be called when the channel state is READY, (i.e., channel has been initialized, but not started) the channel will enter RUNNING state once it is enabled successfully.

Note

Enable the channel can start the I2S communication on hardware. It will start outputting BCLK and WS signal. For MCLK signal, it will start to output when initialization is finished

Parameters:

**handle** -- **\[in\]** I2S channel handler

- ESP\_OK Start successfully
- ESP\_ERR\_INVALID\_ARG NULL pointer
- ESP\_ERR\_INVALID\_STATE This channel has not initialized or already started

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_disable( handle) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv419i2s_channel_disable17i2s_chan_handle_t "Permalink to this definition")  

Disable the I2S channel.

Note

Only allowed to be called when the channel state is RUNNING, (i.e., channel has been started) the channel will enter READY state once it is disabled successfully.

Note

Disable the channel can stop the I2S communication on hardware. It will stop BCLK and WS signal but not MCLK signal

Parameters:

**handle** -- **\[in\]** I2S channel handler

Returns:

- ESP\_OK Stop successfully
- ESP\_ERR\_INVALID\_ARG NULL pointer
- ESP\_ERR\_INVALID\_STATE This channel has not stated

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_write( handle, const void \*src, size\_t size, size\_t \*bytes\_written, uint32\_t timeout\_ms) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv417i2s_channel_write17i2s_chan_handle_tPKv6size_tP6size_t8uint32_t "Permalink to this definition")  

I2S write data.

Note

Only allowed to be called when the channel state is RUNNING, (i.e., TX channel has been started and is not writing now) but the RUNNING only stands for the software state, it doesn't mean there is no the signal transporting on line.

Parameters:

- **handle** -- **\[in\]** I2S channel handler
- **src** -- **\[in\]** The pointer of sent data buffer
- **size** -- **\[in\]** Max data buffer length
- **bytes\_written** -- **\[out\]** Byte number that actually be sent, can be NULL if not needed
- **timeout\_ms** -- **\[in\]** Max block time

Returns:

- ESP\_OK Write successfully
- ESP\_ERR\_INVALID\_ARG NULL pointer or this handle is not TX handle
- ESP\_ERR\_TIMEOUT Writing timeout, no writing event received from ISR within ticks\_to\_wait
- ESP\_ERR\_INVALID\_STATE I2S is not ready to write

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_read( handle, void \*dest, size\_t size, size\_t \*bytes\_read, uint32\_t timeout\_ms) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv416i2s_channel_read17i2s_chan_handle_tPv6size_tP6size_t8uint32_t "Permalink to this definition")  

I2S read data.

Note

Only allowed to be called when the channel state is RUNNING but the RUNNING only stands for the software state, it doesn't mean there is no the signal transporting on line.

Parameters:

- **handle** -- **\[in\]** I2S channel handler
- **dest** -- **\[in\]** The pointer of receiving data buffer
- **size** -- **\[in\]** Max data buffer length
- **bytes\_read** -- **\[out\]** Byte number that actually be read, can be NULL if not needed
- **timeout\_ms** -- **\[in\]** Max block time

Returns:

- ESP\_OK Read successfully
- ESP\_ERR\_INVALID\_ARG NULL pointer or this handle is not RX handle
- ESP\_ERR\_TIMEOUT Reading timeout, no reading event received from ISR within ticks\_to\_wait
- ESP\_ERR\_INVALID\_STATE I2S is not ready to read

Set event callbacks for I2S channel.

Note

Only allowed to be called when the channel state is REGISTERED / READY, (i.e., before channel starts)

Note

User can deregister a previously registered callback by calling this function and setting the callback member in the `callbacks` structure to NULL.

Note

When CONFIG\_I2S\_ISR\_IRAM\_SAFE is enabled, the callback itself and functions called by it should be placed in IRAM. The variables used in the function should be in the SRAM as well. The `user_data` should also reside in SRAM or internal RAM as well.

Parameters:

- **handle** -- **\[in\]** I2S channel handler
- **callbacks** -- **\[in\]** Group of callback functions
- **user\_data** -- **\[in\]** User data, which will be passed to callback functions directly

Returns:

- ESP\_OK Set event callbacks successfully
- ESP\_ERR\_INVALID\_ARG Set event callbacks failed because of invalid argument
- ESP\_ERR\_INVALID\_STATE Set event callbacks failed because the current channel state is not REGISTERED or READY

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_preload\_data( tx\_handle, const void \*src, size\_t size, size\_t \*bytes\_loaded) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv424i2s_channel_preload_data17i2s_chan_handle_tPKv6size_tP6size_t "Permalink to this definition")  

Preload the data into TX DMA buffer.

Note

Only allowed to be called when the channel state is READY, (i.e., channel has been initialized, but not started)

Note

As the initial DMA buffer has no data inside, it will transmit the empty buffer after enabled the channel, this function is used to preload the data into the DMA buffer, so that the valid data can be transmitted immediately after the channel is enabled.

Note

This function can be called multiple times before enabling the channel, the buffer that loaded later will be concatenated behind the former loaded buffer. But when all the DMA buffers have been loaded, no more data can be preload then, please check the `bytes_loaded` parameter to see how many bytes are loaded successfully, when the `bytes_loaded` is smaller than the `size`, it means the DMA buffers are full.

Parameters:

- **tx\_handle** -- **\[in\]** I2S TX channel handler
- **src** -- **\[in\]** The pointer of the source buffer to be loaded
- **size** -- **\[in\]** The source buffer size
- **bytes\_loaded** -- **\[out\]** The bytes that successfully been loaded into the TX DMA buffer

Returns:

- ESP\_OK Load data successful
- ESP\_FAIL Failed to push the message queue
- ESP\_ERR\_INVALID\_ARG NULL pointer or not TX direction
- ESP\_ERR\_INVALID\_STATE This channel has not stated

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") i2s\_channel\_tune\_rate( handle, const \*tune\_cfg, \*tune\_info) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv421i2s_channel_tune_rate17i2s_chan_handle_tPK19i2s_tuning_config_tP17i2s_tuning_info_t "Permalink to this definition")  

Tune the I2S clock rate.

Note

Only allowed to be called when the channel state is READY, (i.e., channel has been initialized, but not started)

Note

This function is mainly to fine-tuning the mclk to match the speed of producer and consumer. So that to avoid exsaust of the memory to store the data from producer. Please take care the how different the frequency error can be tolerant by your codec, otherwise the codec might stop working if the frequency changes a lot.

Parameters:

- **handle** -- **\[in\]** I2S channel handler
- **tune\_cfg** -- **\[in\]** The clock tuning configuration, can be NULL if only need the current clock result
- **tune\_info** -- **\[out\]** The clock tuning information, can be NULL if not needed

Returns:

- ESP\_OK Tune the clock successfully
- ESP\_ERR\_INVALID\_ARG Tune the clock failed because of the invalid argument like NULL pointer or out of range
- ESP\_ERR\_NOT\_SUPPORTED Tune the clock failed because this function does not support to tune the external clock source

### Structures

struct i2s\_event\_callbacks\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv421i2s_event_callbacks_t "Permalink to this definition")  

Group of I2S callbacks.

Note

The callbacks are all running under ISR environment

Note

When CONFIG\_I2S\_ISR\_IRAM\_SAFE is enabled, the callback itself and functions called by it should be placed in IRAM. The variables used in the function should be in the SRAM as well.

Public Members

on\_recv [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_event_callbacks_t7on_recvE "Permalink to this definition")  

Callback of data received event, only for RX channel The event data includes DMA buffer address and size that just finished receiving data

on\_recv\_q\_ovf [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_event_callbacks_t13on_recv_q_ovfE "Permalink to this definition")  

Callback of receiving queue overflowed event, only for RX channel The event data includes buffer size that has been overwritten

on\_sent [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_event_callbacks_t7on_sentE "Permalink to this definition")  

Callback of data sent event, only for TX channel The event data includes DMA buffer address and size that just finished sending data

on\_send\_q\_ovf [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N21i2s_event_callbacks_t13on_send_q_ovfE "Permalink to this definition")  

Callback of sending queue overflowed event, only for TX channel The event data includes buffer size that has been overwritten

struct i2s\_chan\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv417i2s_chan_config_t "Permalink to this definition")  

I2S controller channel configuration.

Public Members

int id [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N17i2s_chan_config_t2idE "Permalink to this definition")  

I2S port id

role [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N17i2s_chan_config_t4roleE "Permalink to this definition")  

I2S role, I2S\_ROLE\_MASTER or I2S\_ROLE\_SLAVE

uint32\_t dma\_desc\_num [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N17i2s_chan_config_t12dma_desc_numE "Permalink to this definition")  

I2S DMA buffer number, it is also the number of DMA descriptor

uint32\_t dma\_frame\_num [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N17i2s_chan_config_t13dma_frame_numE "Permalink to this definition")  

I2S frame number in one DMA buffer. One frame means one-time sample data in all slots, it should be the multiple of `3` when the data bit width is 24.

bool auto\_clear [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N17i2s_chan_config_t10auto_clearE "Permalink to this definition")  

Alias of `auto_clear_after_cb`

bool auto\_clear\_after\_cb [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N17i2s_chan_config_t19auto_clear_after_cbE "Permalink to this definition")  

Set to auto clear DMA TX buffer after `on_sent` callback, I2S will always send zero automatically if no data to send. So that user can assign the data to the DMA buffers directly in the callback, and the data won't be cleared after quit the callback.

bool auto\_clear\_before\_cb [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N17i2s_chan_config_t20auto_clear_before_cbE "Permalink to this definition")  

Set to auto clear DMA TX buffer before `on_sent` callback, I2S will always send zero automatically if no data to send So that user can access data in the callback that just finished to send.

bool allow\_pd [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N17i2s_chan_config_t8allow_pdE "Permalink to this definition")  

Set to allow power down. When this flag set, the driver will backup/restore the I2S registers before/after entering/exist sleep mode. By this approach, the system can power off I2S's power domain. This can save power, but at the expense of more RAM being consumed.

int intr\_priority [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N17i2s_chan_config_t13intr_priorityE "Permalink to this definition")  

I2S interrupt priority, range \[0, 7\], if set to 0, the driver will try to allocate an interrupt with a relative low priority (1,2,3)

struct i2s\_chan\_info\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv415i2s_chan_info_t "Permalink to this definition")  

I2S channel information.

Public Members

int id [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N15i2s_chan_info_t2idE "Permalink to this definition")  

I2S port id

role [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N15i2s_chan_info_t4roleE "Permalink to this definition")  

I2S role, I2S\_ROLE\_MASTER or I2S\_ROLE\_SLAVE

dir [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N15i2s_chan_info_t3dirE "Permalink to this definition")  

I2S channel direction

mode [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N15i2s_chan_info_t4modeE "Permalink to this definition")  

I2S channel communication mode

bool is\_enabled [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N15i2s_chan_info_t10is_enabledE "Permalink to this definition")  

I2S channel is enabled or not

pair\_chan [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N15i2s_chan_info_t9pair_chanE "Permalink to this definition")  

I2S pair channel handle in duplex mode, always NULL in simplex mode

uint32\_t total\_dma\_buf\_size [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N15i2s_chan_info_t18total_dma_buf_sizeE "Permalink to this definition")  

Total size of all the allocated DMA buffers

- 0 if the channel has not been initialized
- non-zero if the channel has been initialized

clk\_src [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N15i2s_chan_info_t7clk_srcE "Permalink to this definition")  

Clock source of I2S

uint32\_t sclk\_hz [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N15i2s_chan_info_t7sclk_hzE "Permalink to this definition")  

Source clock frequency

uint32\_t mclk\_hz [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N15i2s_chan_info_t7mclk_hzE "Permalink to this definition")  

MCLK frequency

uint32\_t bclk\_hz [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N15i2s_chan_info_t7bclk_hzE "Permalink to this definition")  

BCLK frequency

const void \*mode\_cfg [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N15i2s_chan_info_t8mode_cfgE "Permalink to this definition")  

Mode configuration, it need to be casted to the corresponding type according to the communication mode

- I2S\_COMM\_MODE\_STD: i2s\_std\_config\_t\*
- I2S\_COMM\_MODE\_TDM: i2s\_tdm\_config\_t\*
- I2S\_COMM\_MODE\_PDM + I2S\_DIR\_RX: i2s\_pdm\_rx\_config\_t\*
- I2S\_COMM\_MODE\_PDM + I2S\_DIR\_TX: i2s\_pdm\_tx\_config\_t\*

### Macros

I2S\_CHANNEL\_DEFAULT\_CONFIG(i2s\_num, i2s\_role) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_CHANNEL_DEFAULT_CONFIG "Permalink to this definition")  

get default I2S property

I2S\_GPIO\_UNUSED [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_GPIO_UNUSED "Permalink to this definition")  

Used in i2s\_gpio\_config\_t for signals which are not used

### I2S Types

### Header File

- [components/esp\_driver\_i2s/include/driver/i2s\_types.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_driver_i2s/include/driver/i2s_types.h)
- This header file can be included with:
	> ```c
	> #include "driver/i2s_types.h"
	> ```
- This header file is a part of the API provided by the `esp_driver_i2s` component. To declare that your component depends on `esp_driver_i2s`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES esp_driver_i2s
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES esp_driver_i2s
	> ```

### Structures

struct lp\_i2s\_trans\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv414lp_i2s_trans_t "Permalink to this definition")  

LP I2S transaction type.

Public Members

void \*buffer [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N14lp_i2s_trans_t6bufferE "Permalink to this definition")  

Pointer to buffer.

size\_t buflen [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N14lp_i2s_trans_t6buflenE "Permalink to this definition")  

Buffer len, this should be in the multiple of 4.

size\_t received\_size [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N14lp_i2s_trans_t13received_sizeE "Permalink to this definition")  

Received size.

struct i2s\_event\_data\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv416i2s_event_data_t "Permalink to this definition")  

Event structure used in I2S event queue.

Public Members

void \*dma\_buf [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N16i2s_event_data_t7dma_bufE "Permalink to this definition")  

The first level pointer of DMA buffer that just finished sending or receiving for `on_recv` and `on_sent` callback NULL for `on_recv_q_ovf` and `on_send_q_ovf` callback

size\_t size [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N16i2s_event_data_t4sizeE "Permalink to this definition")  

The buffer size of DMA buffer when success to send or receive, also the buffer size that dropped when queue overflow. It is related to the dma\_frame\_num and data\_bit\_width, typically it is fixed when data\_bit\_width is not changed.

struct i2s\_tuning\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv419i2s_tuning_config_t "Permalink to this definition")  

I2S clock tuning configurations.

Public Members

tune\_mode [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tuning_config_t9tune_modeE "Permalink to this definition")  

Tuning mode, which decides how to tune the MCLK with the tuning value

int32\_t tune\_mclk\_val [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tuning_config_t13tune_mclk_valE "Permalink to this definition")  

Tuning value

int32\_t max\_delta\_mclk [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tuning_config_t14max_delta_mclkE "Permalink to this definition")  

The maximum frequency that can be increased comparing to the initial MCLK freuqnecy

int32\_t min\_delta\_mclk [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tuning_config_t14min_delta_mclkE "Permalink to this definition")  

The minimum frequency that can be decreased comparing to the initial MCLK freuqnecy

struct i2s\_tuning\_info\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv417i2s_tuning_info_t "Permalink to this definition")  

I2S clock tuning result.

Public Members

int32\_t curr\_mclk\_hz [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N17i2s_tuning_info_t12curr_mclk_hzE "Permalink to this definition")  

The current MCLK frequency after tuned

int32\_t delta\_mclk\_hz [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N17i2s_tuning_info_t13delta_mclk_hzE "Permalink to this definition")  

The current changed MCLK frequency comparing to the initial MCLK frequency

uint32\_t water\_mark [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N17i2s_tuning_info_t10water_markE "Permalink to this definition")  

The water mark of the internal buffer, in percent

struct lp\_i2s\_evt\_data\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv417lp_i2s_evt_data_t "Permalink to this definition")  

Event data structure for LP I2S.

Public Members

trans [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N17lp_i2s_evt_data_t5transE "Permalink to this definition")  

LP I2S transaction.

### Macros

I2S\_NUM\_0 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_NUM_0 "Permalink to this definition")  

I2S controller port 0

I2S\_NUM\_1 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_NUM_1 "Permalink to this definition")  

I2S controller port 1

I2S\_NUM\_2 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_NUM_2 "Permalink to this definition")  

I2S controller port 2

I2S\_NUM\_AUTO [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#c.I2S_NUM_AUTO "Permalink to this definition")  

Select an available port automatically

### Type Definitions

typedef struct i2s\_channel\_obj\_t \*i2s\_chan\_handle\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv417i2s_chan_handle_t "Permalink to this definition")  

I2S channel object handle, the control unit of the I2S driver

typedef struct lp\_i2s\_channel\_obj\_t \*lp\_i2s\_chan\_handle\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv420lp_i2s_chan_handle_t "Permalink to this definition")  

I2S channel object handle, the control unit of the I2S driver

typedef bool (\*i2s\_isr\_callback\_t)( handle, \*event, void \*user\_ctx) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv418i2s_isr_callback_t "Permalink to this definition")  

I2S event callback.

Param handle:

**\[in\]** I2S channel handle, created from `i2s_new_channel()`

Param event:

**\[in\]** I2S event data

Param user\_ctx:

**\[in\]** User registered context, passed from `i2s_channel_register_event_callback()`

Return:

Whether a high priority task has been waken up by this callback function

typedef bool (\*lp\_i2s\_callback\_t)( handle, \*event, void \*user\_ctx) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv417lp_i2s_callback_t "Permalink to this definition")  

LP I2S event callback type.

Param handle:

**\[in\]** LP I2S channel handle

Param event:

**\[in\]** Event data

Param user\_ctx:

**\[in\]** User data

Return:

Whether a high priority task has been waken up by this callback function

### Enumerations

enum i2s\_comm\_mode\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv415i2s_comm_mode_t "Permalink to this definition")  

I2S controller communication mode.

*Values:*

enumerator I2S\_COMM\_MODE\_STD [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N15i2s_comm_mode_t17I2S_COMM_MODE_STDE "Permalink to this definition")  

I2S controller using standard communication mode, support Philips/MSB/PCM format

enumerator I2S\_COMM\_MODE\_PDM [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N15i2s_comm_mode_t17I2S_COMM_MODE_PDME "Permalink to this definition")  

I2S controller using PDM communication mode, support PDM output or input

enumerator I2S\_COMM\_MODE\_TDM [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N15i2s_comm_mode_t17I2S_COMM_MODE_TDME "Permalink to this definition")  

I2S controller using TDM communication mode, support up to 16 slots per frame

enumerator I2S\_COMM\_MODE\_NONE [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N15i2s_comm_mode_t18I2S_COMM_MODE_NONEE "Permalink to this definition")  

Unspecified I2S controller mode

enum i2s\_mclk\_multiple\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv419i2s_mclk_multiple_t "Permalink to this definition")  

The multiple of MCLK to sample rate.

Note

MCLK is the minimum resolution of the I2S clock. Increasing mclk multiple can reduce the clock jitter of BCLK and WS, which is also useful for the codec that don't require MCLK but have strict requirement to BCLK. For the 24-bit slot width, please choose a multiple that can be divided by 3 (i.e. 24-bit compatible).

*Values:*

enumerator I2S\_MCLK\_MULTIPLE\_128 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_mclk_multiple_t21I2S_MCLK_MULTIPLE_128E "Permalink to this definition")  

MCLK = sample\_rate \* 128

enumerator I2S\_MCLK\_MULTIPLE\_192 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_mclk_multiple_t21I2S_MCLK_MULTIPLE_192E "Permalink to this definition")  

MCLK = sample\_rate \* 192 (24-bit compatible)

enumerator I2S\_MCLK\_MULTIPLE\_256 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_mclk_multiple_t21I2S_MCLK_MULTIPLE_256E "Permalink to this definition")  

MCLK = sample\_rate \* 256

enumerator I2S\_MCLK\_MULTIPLE\_384 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_mclk_multiple_t21I2S_MCLK_MULTIPLE_384E "Permalink to this definition")  

MCLK = sample\_rate \* 384 (24-bit compatible)

enumerator I2S\_MCLK\_MULTIPLE\_512 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_mclk_multiple_t21I2S_MCLK_MULTIPLE_512E "Permalink to this definition")  

MCLK = sample\_rate \* 512

enumerator I2S\_MCLK\_MULTIPLE\_576 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_mclk_multiple_t21I2S_MCLK_MULTIPLE_576E "Permalink to this definition")  

MCLK = sample\_rate \* 576 (24-bit compatible)

enumerator I2S\_MCLK\_MULTIPLE\_768 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_mclk_multiple_t21I2S_MCLK_MULTIPLE_768E "Permalink to this definition")  

MCLK = sample\_rate \* 768 (24-bit compatible)

enumerator I2S\_MCLK\_MULTIPLE\_1024 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_mclk_multiple_t22I2S_MCLK_MULTIPLE_1024E "Permalink to this definition")  

MCLK = sample\_rate \* 1024

enumerator I2S\_MCLK\_MULTIPLE\_1152 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_mclk_multiple_t22I2S_MCLK_MULTIPLE_1152E "Permalink to this definition")  

MCLK = sample\_rate \* 1152 (24-bit compatible)

enum i2s\_tuning\_mode\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv417i2s_tuning_mode_t "Permalink to this definition")  

I2S clock tuning operation.

*Values:*

enumerator I2S\_TUNING\_MODE\_ADDSUB [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N17i2s_tuning_mode_t22I2S_TUNING_MODE_ADDSUBE "Permalink to this definition")  

Add or subtract the tuning value based on the current clock

enumerator I2S\_TUNING\_MODE\_SET [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N17i2s_tuning_mode_t19I2S_TUNING_MODE_SETE "Permalink to this definition")  

Set the tuning value to overwrite the current clock

enumerator I2S\_TUNING\_MODE\_RESET [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N17i2s_tuning_mode_t21I2S_TUNING_MODE_RESETE "Permalink to this definition")  

Set the clock to the initial value

### Header File

- [components/esp\_hal\_i2s/include/hal/i2s\_types.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_hal_i2s/include/hal/i2s_types.h)
- This header file can be included with:
	> ```c
	> #include "hal/i2s_types.h"
	> ```
- This header file is a part of the API provided by the `esp_hal_i2s` component. To declare that your component depends on `esp_hal_i2s`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES esp_hal_i2s
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES esp_hal_i2s
	> ```

### Type Definitions

typedef int i2s\_clock\_src\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv415i2s_clock_src_t "Permalink to this definition")  

Define a default type to avoid compiling warnings

### Enumerations

enum i2s\_slot\_mode\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv415i2s_slot_mode_t "Permalink to this definition")  

I2S channel slot mode.

*Values:*

enumerator I2S\_SLOT\_MODE\_MONO [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N15i2s_slot_mode_t18I2S_SLOT_MODE_MONOE "Permalink to this definition")  

I2S channel slot format mono, transmit same data in all slots for tx mode, only receive the data in the first slots for rx mode.

enumerator I2S\_SLOT\_MODE\_STEREO [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N15i2s_slot_mode_t20I2S_SLOT_MODE_STEREOE "Permalink to this definition")  

I2S channel slot format stereo, transmit different data in different slots for tx mode, receive the data in all slots for rx mode.

enum i2s\_dir\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv49i2s_dir_t "Permalink to this definition")  

I2S channel direction.

*Values:*

enumerator I2S\_DIR\_RX [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N9i2s_dir_t10I2S_DIR_RXE "Permalink to this definition")  

I2S channel direction RX

enumerator I2S\_DIR\_TX [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N9i2s_dir_t10I2S_DIR_TXE "Permalink to this definition")  

I2S channel direction TX

enum i2s\_role\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv410i2s_role_t "Permalink to this definition")  

I2S controller role.

*Values:*

enumerator I2S\_ROLE\_MASTER [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N10i2s_role_t15I2S_ROLE_MASTERE "Permalink to this definition")  

I2S controller master role, bclk and ws signal will be set to output

enumerator I2S\_ROLE\_SLAVE [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N10i2s_role_t14I2S_ROLE_SLAVEE "Permalink to this definition")  

I2S controller slave role, bclk and ws signal will be set to input

enum i2s\_data\_bit\_width\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv420i2s_data_bit_width_t "Permalink to this definition")  

Available data bit width in one slot.

*Values:*

enumerator I2S\_DATA\_BIT\_WIDTH\_8BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_data_bit_width_t23I2S_DATA_BIT_WIDTH_8BITE "Permalink to this definition")  

I2S channel data bit-width: 8

enumerator I2S\_DATA\_BIT\_WIDTH\_16BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_data_bit_width_t24I2S_DATA_BIT_WIDTH_16BITE "Permalink to this definition")  

I2S channel data bit-width: 16

enumerator I2S\_DATA\_BIT\_WIDTH\_24BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_data_bit_width_t24I2S_DATA_BIT_WIDTH_24BITE "Permalink to this definition")  

I2S channel data bit-width: 24

enumerator I2S\_DATA\_BIT\_WIDTH\_32BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_data_bit_width_t24I2S_DATA_BIT_WIDTH_32BITE "Permalink to this definition")  

I2S channel data bit-width: 32

enum i2s\_slot\_bit\_width\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv420i2s_slot_bit_width_t "Permalink to this definition")  

Total slot bit width in one slot.

*Values:*

enumerator I2S\_SLOT\_BIT\_WIDTH\_AUTO [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_slot_bit_width_t23I2S_SLOT_BIT_WIDTH_AUTOE "Permalink to this definition")  

I2S channel slot bit-width equals to data bit-width

enumerator I2S\_SLOT\_BIT\_WIDTH\_8BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_slot_bit_width_t23I2S_SLOT_BIT_WIDTH_8BITE "Permalink to this definition")  

I2S channel slot bit-width: 8

enumerator I2S\_SLOT\_BIT\_WIDTH\_16BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_slot_bit_width_t24I2S_SLOT_BIT_WIDTH_16BITE "Permalink to this definition")  

I2S channel slot bit-width: 16

enumerator I2S\_SLOT\_BIT\_WIDTH\_24BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_slot_bit_width_t24I2S_SLOT_BIT_WIDTH_24BITE "Permalink to this definition")  

I2S channel slot bit-width: 24

enumerator I2S\_SLOT\_BIT\_WIDTH\_32BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_slot_bit_width_t24I2S_SLOT_BIT_WIDTH_32BITE "Permalink to this definition")  

I2S channel slot bit-width: 32

enum i2s\_pcm\_compress\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv418i2s_pcm_compress_t "Permalink to this definition")  

A/U-law decompress or compress configuration.

*Values:*

enumerator I2S\_PCM\_DISABLE [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N18i2s_pcm_compress_t15I2S_PCM_DISABLEE "Permalink to this definition")  

Disable A/U law decompress or compress

enumerator I2S\_PCM\_A\_DECOMPRESS [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N18i2s_pcm_compress_t20I2S_PCM_A_DECOMPRESSE "Permalink to this definition")  

A-law decompress

enumerator I2S\_PCM\_A\_COMPRESS [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N18i2s_pcm_compress_t18I2S_PCM_A_COMPRESSE "Permalink to this definition")  

A-law compress

enumerator I2S\_PCM\_U\_DECOMPRESS [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N18i2s_pcm_compress_t20I2S_PCM_U_DECOMPRESSE "Permalink to this definition")  

U-law decompress

enumerator I2S\_PCM\_U\_COMPRESS [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N18i2s_pcm_compress_t18I2S_PCM_U_COMPRESSE "Permalink to this definition")  

U-law compress

enum i2s\_pdm\_data\_fmt\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv418i2s_pdm_data_fmt_t "Permalink to this definition")  

I2S PDM data format.

*Values:*

enumerator I2S\_PDM\_DATA\_FMT\_PCM [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N18i2s_pdm_data_fmt_t20I2S_PDM_DATA_FMT_PCME "Permalink to this definition")  

PDM RX: Enable the hardware PDM to PCM filter to convert the inputted PDM data on the line into PCM format in software, so that the read data in software is PCM format data already, no need additional software filter. PCM data format is only available when PCM2PDM filter is supported in hardware. PDM TX: Enable the hardware PCM to PDM filter to convert the written PCM data in software into PDM format on the line, so that we only need to write the PCM data in software, no need to prepare raw PDM data in software. PCM data format is only available when PDM2PCM filter is supported in hardware.

enumerator I2S\_PDM\_DATA\_FMT\_RAW [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N18i2s_pdm_data_fmt_t20I2S_PDM_DATA_FMT_RAWE "Permalink to this definition")  

PDM RX: Read the raw PDM data directly in software, without the hardware PDM to PCM filter. You may need a software PDM to PCM filter to convert the raw PDM data that read into PCM format. PDM TX: Write the raw PDM data directly in software, without the hardware PCM to PDM filter. You may need to prepare the raw PDM data in software to output the PDM format data on the line.

enum i2s\_pdm\_dsr\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv413i2s_pdm_dsr_t "Permalink to this definition")  

I2S PDM RX down-sampling mode.

*Values:*

enumerator I2S\_PDM\_DSR\_8S [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N13i2s_pdm_dsr_t14I2S_PDM_DSR_8SE "Permalink to this definition")  

downsampling number is 8 for PDM RX mode

enumerator I2S\_PDM\_DSR\_16S [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N13i2s_pdm_dsr_t15I2S_PDM_DSR_16SE "Permalink to this definition")  

downsampling number is 16 for PDM RX mode

enumerator I2S\_PDM\_DSR\_MAX [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N13i2s_pdm_dsr_t15I2S_PDM_DSR_MAXE "Permalink to this definition")  

enum i2s\_pdm\_sig\_scale\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv419i2s_pdm_sig_scale_t "Permalink to this definition")  

pdm tx signal scaling mode

*Values:*

enumerator I2S\_PDM\_SIG\_SCALING\_DIV\_2 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_sig_scale_t25I2S_PDM_SIG_SCALING_DIV_2E "Permalink to this definition")  

I2S TX PDM signal scaling: /2

enumerator I2S\_PDM\_SIG\_SCALING\_MUL\_1 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_sig_scale_t25I2S_PDM_SIG_SCALING_MUL_1E "Permalink to this definition")  

I2S TX PDM signal scaling: x1

enumerator I2S\_PDM\_SIG\_SCALING\_MUL\_2 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_sig_scale_t25I2S_PDM_SIG_SCALING_MUL_2E "Permalink to this definition")  

I2S TX PDM signal scaling: x2

enumerator I2S\_PDM\_SIG\_SCALING\_MUL\_4 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_sig_scale_t25I2S_PDM_SIG_SCALING_MUL_4E "Permalink to this definition")  

I2S TX PDM signal scaling: x4

enum i2s\_pdm\_tx\_line\_mode\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv422i2s_pdm_tx_line_mode_t "Permalink to this definition")  

PDM TX line mode.

Note

For the standard codec mode, PDM pins are connect to a codec which requires both clock signal and data signal For the DAC output mode, PDM data signal can be connected to a power amplifier directly with a low-pass filter, normally, DAC output mode doesn't need the clock signal.

*Values:*

enumerator I2S\_PDM\_TX\_ONE\_LINE\_CODEC [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N22i2s_pdm_tx_line_mode_t25I2S_PDM_TX_ONE_LINE_CODECE "Permalink to this definition")  

Standard PDM format output, left and right slot data on a single line

enumerator I2S\_PDM\_TX\_ONE\_LINE\_DAC [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N22i2s_pdm_tx_line_mode_t23I2S_PDM_TX_ONE_LINE_DACE "Permalink to this definition")  

PDM DAC format output, left or right slot data on a single line

enumerator I2S\_PDM\_TX\_TWO\_LINE\_DAC [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N22i2s_pdm_tx_line_mode_t23I2S_PDM_TX_TWO_LINE_DACE "Permalink to this definition")  

PDM DAC format output, left and right slot data on separated lines

enum i2s\_std\_slot\_mask\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv419i2s_std_slot_mask_t "Permalink to this definition")  

I2S slot select in standard mode.

Note

It has different meanings in tx/rx/mono/stereo mode, and it may have different behaviors on different targets For the details, please refer to the I2S API reference

*Values:*

enumerator I2S\_STD\_SLOT\_LEFT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_std_slot_mask_t17I2S_STD_SLOT_LEFTE "Permalink to this definition")  

I2S transmits or receives left slot

enumerator I2S\_STD\_SLOT\_RIGHT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_std_slot_mask_t18I2S_STD_SLOT_RIGHTE "Permalink to this definition")  

I2S transmits or receives right slot

enumerator I2S\_STD\_SLOT\_BOTH [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_std_slot_mask_t17I2S_STD_SLOT_BOTHE "Permalink to this definition")  

I2S transmits or receives both left and right slot

enum i2s\_pdm\_slot\_mask\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv419i2s_pdm_slot_mask_t "Permalink to this definition")  

I2S slot select in PDM mode.

*Values:*

enumerator I2S\_PDM\_SLOT\_RIGHT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_slot_mask_t18I2S_PDM_SLOT_RIGHTE "Permalink to this definition")  

I2S PDM only transmits or receives the PDM device whose 'select' pin is pulled up

enumerator I2S\_PDM\_SLOT\_LEFT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_slot_mask_t17I2S_PDM_SLOT_LEFTE "Permalink to this definition")  

I2S PDM only transmits or receives the PDM device whose 'select' pin is pulled down

enumerator I2S\_PDM\_SLOT\_BOTH [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_slot_mask_t17I2S_PDM_SLOT_BOTHE "Permalink to this definition")  

I2S PDM transmits or receives both two slots

enumerator I2S\_PDM\_RX\_LINE0\_SLOT\_RIGHT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_slot_mask_t27I2S_PDM_RX_LINE0_SLOT_RIGHTE "Permalink to this definition")  

I2S PDM receives the right slot on line 0

enumerator I2S\_PDM\_RX\_LINE0\_SLOT\_LEFT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_slot_mask_t26I2S_PDM_RX_LINE0_SLOT_LEFTE "Permalink to this definition")  

I2S PDM receives the left slot on line 0

enumerator I2S\_PDM\_RX\_LINE1\_SLOT\_RIGHT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_slot_mask_t27I2S_PDM_RX_LINE1_SLOT_RIGHTE "Permalink to this definition")  

I2S PDM receives the right slot on line 1

enumerator I2S\_PDM\_RX\_LINE1\_SLOT\_LEFT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_slot_mask_t26I2S_PDM_RX_LINE1_SLOT_LEFTE "Permalink to this definition")  

I2S PDM receives the left slot on line 1

enumerator I2S\_PDM\_RX\_LINE2\_SLOT\_RIGHT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_slot_mask_t27I2S_PDM_RX_LINE2_SLOT_RIGHTE "Permalink to this definition")  

I2S PDM receives the right slot on line 2

enumerator I2S\_PDM\_RX\_LINE2\_SLOT\_LEFT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_slot_mask_t26I2S_PDM_RX_LINE2_SLOT_LEFTE "Permalink to this definition")  

I2S PDM receives the left slot on line 2

enumerator I2S\_PDM\_RX\_LINE3\_SLOT\_RIGHT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_slot_mask_t27I2S_PDM_RX_LINE3_SLOT_RIGHTE "Permalink to this definition")  

I2S PDM receives the right slot on line 3

enumerator I2S\_PDM\_RX\_LINE3\_SLOT\_LEFT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_slot_mask_t26I2S_PDM_RX_LINE3_SLOT_LEFTE "Permalink to this definition")  

I2S PDM receives the left slot on line 3

enumerator I2S\_PDM\_LINE\_SLOT\_ALL [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_pdm_slot_mask_t21I2S_PDM_LINE_SLOT_ALLE "Permalink to this definition")  

I2S PDM receives all slots

enum i2s\_tdm\_slot\_mask\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv419i2s_tdm_slot_mask_t "Permalink to this definition")  

tdm slot number

Note

Multiple slots in TDM mode. For TX module, only the active slot send the audio data, the inactive slot send a constant or will be skipped if 'skip\_msk' is set. For RX module, only receive the audio data in active slots, the data in inactive slots will be ignored. the bit map of active slot can not exceed (0x1<<total\_slot\_num). e.g: slot\_mask = (I2S\_TDM\_SLOT0 | I2S\_TDM\_SLOT3), here the active slot number is 2 and total\_slot is not supposed to be smaller than 4.

*Values:*

enumerator I2S\_TDM\_SLOT0 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tdm_slot_mask_t13I2S_TDM_SLOT0E "Permalink to this definition")  

I2S slot 0 enabled

enumerator I2S\_TDM\_SLOT1 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tdm_slot_mask_t13I2S_TDM_SLOT1E "Permalink to this definition")  

I2S slot 1 enabled

enumerator I2S\_TDM\_SLOT2 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tdm_slot_mask_t13I2S_TDM_SLOT2E "Permalink to this definition")  

I2S slot 2 enabled

enumerator I2S\_TDM\_SLOT3 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tdm_slot_mask_t13I2S_TDM_SLOT3E "Permalink to this definition")  

I2S slot 3 enabled

enumerator I2S\_TDM\_SLOT4 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tdm_slot_mask_t13I2S_TDM_SLOT4E "Permalink to this definition")  

I2S slot 4 enabled

enumerator I2S\_TDM\_SLOT5 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tdm_slot_mask_t13I2S_TDM_SLOT5E "Permalink to this definition")  

I2S slot 5 enabled

enumerator I2S\_TDM\_SLOT6 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tdm_slot_mask_t13I2S_TDM_SLOT6E "Permalink to this definition")  

I2S slot 6 enabled

enumerator I2S\_TDM\_SLOT7 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tdm_slot_mask_t13I2S_TDM_SLOT7E "Permalink to this definition")  

I2S slot 7 enabled

enumerator I2S\_TDM\_SLOT8 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tdm_slot_mask_t13I2S_TDM_SLOT8E "Permalink to this definition")  

I2S slot 8 enabled

enumerator I2S\_TDM\_SLOT9 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tdm_slot_mask_t13I2S_TDM_SLOT9E "Permalink to this definition")  

I2S slot 9 enabled

enumerator I2S\_TDM\_SLOT10 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tdm_slot_mask_t14I2S_TDM_SLOT10E "Permalink to this definition")  

I2S slot 10 enabled

enumerator I2S\_TDM\_SLOT11 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tdm_slot_mask_t14I2S_TDM_SLOT11E "Permalink to this definition")  

I2S slot 11 enabled

enumerator I2S\_TDM\_SLOT12 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tdm_slot_mask_t14I2S_TDM_SLOT12E "Permalink to this definition")  

I2S slot 12 enabled

enumerator I2S\_TDM\_SLOT13 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tdm_slot_mask_t14I2S_TDM_SLOT13E "Permalink to this definition")  

I2S slot 13 enabled

enumerator I2S\_TDM\_SLOT14 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tdm_slot_mask_t14I2S_TDM_SLOT14E "Permalink to this definition")  

I2S slot 14 enabled

enumerator I2S\_TDM\_SLOT15 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_tdm_slot_mask_t14I2S_TDM_SLOT15E "Permalink to this definition")  

I2S slot 15 enabled

enum i2s\_etm\_event\_type\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv420i2s_etm_event_type_t "Permalink to this definition")  

I2S channel events that supported by the ETM module.

*Values:*

enumerator I2S\_ETM\_EVENT\_DONE [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_etm_event_type_t18I2S_ETM_EVENT_DONEE "Permalink to this definition")  

Trigger condition: TX: no data to send in the TX FIFO, i.e., DMA need to stop (next desc is NULL) RX: 1. If rx\_stop\_mode = 0, this event will trigger when DMA is stopped (next desc is NULL)

1. If rx\_stop\_mode = 1, this event will trigger when DMA in\_suc\_eof.
2. If rx\_stop\_mode = 2, this event will trigger when RX FIFO is full. Event that I2S TX or RX stopped

enumerator I2S\_ETM\_EVENT\_REACH\_THRESH [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_etm_event_type_t26I2S_ETM_EVENT_REACH_THRESHE "Permalink to this definition")  

Trigger condition: TX: the sent words(in 32-bit) number reach the threshold that configured in `etm_tx_send_word_num` RX: the received words(in 32-bit) number reach the threshold that configured in `etm_rx_receive_word_num` and `etm_rx_receive_word_num` should be smaller than the size of the DMA buffer in one `in_suc_eof` event. Event that the I2S sent or received data reached the threshold

enumerator I2S\_ETM\_EVENT\_MAX [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N20i2s_etm_event_type_t17I2S_ETM_EVENT_MAXE "Permalink to this definition")  

Maximum number of events

enum i2s\_etm\_task\_type\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv419i2s_etm_task_type_t "Permalink to this definition")  

I2S channel tasks that supported by the ETM module.

*Values:*

enumerator I2S\_ETM\_TASK\_START [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_etm_task_type_t18I2S_ETM_TASK_STARTE "Permalink to this definition")  

Start the I2S channel

enumerator I2S\_ETM\_TASK\_STOP [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_etm_task_type_t17I2S_ETM_TASK_STOPE "Permalink to this definition")  

Stop the I2S channel

enumerator I2S\_ETM\_TASK\_MAX [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/i2s.html#_CPPv4N19i2s_etm_task_type_t16I2S_ETM_TASK_MAXE "Permalink to this definition")  

Maximum number of tasks