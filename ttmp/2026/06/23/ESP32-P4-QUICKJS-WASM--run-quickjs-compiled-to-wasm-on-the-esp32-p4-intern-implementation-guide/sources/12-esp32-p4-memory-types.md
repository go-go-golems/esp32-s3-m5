---
title: "ESP32-P4 memory types (ESP-IDF docs)"
doc_type: reference
ticket: ESP32-P4-QUICKJS-WASM
topics:
  - wasm
  - quickjs
  - esp32p4
  - esp-idf
status: active
source_type: harvested
---

## Memory Types

[\[中文\]](https://docs.espressif.com/projects/esp-idf/zh_CN/v6.0.1/esp32p4/api-guides/memory-type
s.html)

ESP32-P4 chip has multiple memory types and flexible memory mapping features. This section 
describes how ESP-IDF uses these features by default.

ESP-IDF distinguishes between instruction memory bus (IRAM, IROM, RTC FAST memory) and data memory 
bus (DRAM, DROM). Instruction memory is executable, and can only be read or written via 4-byte 
aligned words. Data memory is not executable and can be accessed via individual byte operations. 
For more information about the different memory buses consult the *ESP32-P4 Technical Reference 
Manual* > *System and Memory* 
\[[PDF](https://www.espressif.com/sites/default/files/documentation/esp32-p4_technical_reference_man
ual_en.pdf#sysmem)\].

## DRAM (Data RAM)

Non-constant static data (.data) and zero-initialized data (.bss) is placed by the linker into 
Internal SRAM as data memory. The remaining space in this region is used for the runtime heap.

By applying the `EXT_RAM_BSS_ATTR` macro, zero-initialized data can also be placed into external 
RAM. To use this macro, the 
[CONFIG\_SPIRAM\_ALLOW\_BSS\_SEG\_EXTERNAL\_MEMORY](https://docs.espressif.com/projects/esp-idf/en/s
table/esp32p4/api-reference/kconfig-reference.html#config-spiram-allow-bss-seg-external-memory) 
needs to be enabled. See [Allow.bss Segment to Be Placed in External 
Memory](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-guides/external-ram.html#e
xternal-ram-config-bss).

> [!note] Note
> The maximum statically allocated DRAM size is reduced by the size of the compiled application. 
The available heap memory at runtime is reduced by the total static IRAM and DRAM usage of the 
application.

Constant data may also be placed into DRAM, for example if it is used in an non-flash-safe ISR (see 
explanation under ).

### "noinit" DRAM

The macro `__NOINIT_ATTR` can be used as attribute to place data into `.noinit` section. The values 
placed into this section will not be initialized at startup and should keep its value after 
software restart.

By applying the `EXT_RAM_NOINIT_ATTR` macro, non-initialized value could also be placed in external 
RAM. To do this, the 
[CONFIG\_SPIRAM\_ALLOW\_NOINIT\_SEG\_EXTERNAL\_MEMORY](https://docs.espressif.com/projects/esp-idf/e
n/stable/esp32p4/api-reference/kconfig-reference.html#config-spiram-allow-noinit-seg-external-memory
) needs to be enabled. See [Allow.noinit Segment to Be Placed in External 
Memory](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-guides/external-ram.html#e
xternal-ram-config-noinit). If the 
[CONFIG\_SPIRAM\_ALLOW\_NOINIT\_SEG\_EXTERNAL\_MEMORY](https://docs.espressif.com/projects/esp-idf/e
n/stable/esp32p4/api-reference/kconfig-reference.html#config-spiram-allow-noinit-seg-external-memory
) is not enabled, `EXT_RAM_NOINIT_ATTR` will behave just as `__NOINIT_ATTR`, it will make data to 
be placed into `.noinit` segment in internal RAM.

Example:

```
__NOINIT_ATTR uint32_t noinit_data;
```

## IRAM (Instruction RAM)

> [!note] Note
> Any internal SRAM which is not used for Instruction RAM will be made available as for static data 
and dynamic allocation (heap).

### When to Place Code in IRAM

Cases when parts of the application should be placed into IRAM:

- Interrupt handlers must be placed into IRAM if `ESP_INTR_FLAG_IRAM` is used when registering the 
interrupt handler. For more information, see [IRAM-Safe Interrupt 
Handlers](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sp
i_flash/spi_flash_concurrency.html#iram-safe-interrupt-handlers).
- Some timing critical code may be placed into IRAM to reduce the penalty associated with loading 
the code from flash. ESP32-P4 reads code and data from flash via the MMU cache. In some cases, 
placing a function into IRAM may reduce delays caused by a cache miss and significantly improve 
that function's performance.

### How to Place Code in IRAM

Some code is automatically placed into the IRAM region using the linker script.

If some specific application code needs to be placed into IRAM, it can be done by using the [Linker 
Script 
Generation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-guides/linker-script-g
eneration.html) feature and adding a linker script fragment file to your component that targets at 
the entire source files or functions with the `noflash` placement. See the [Linker Script 
Generation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-guides/linker-script-g
eneration.html) docs for more information.

Alternatively, it is possible to specify IRAM placement in the source code using the `IRAM_ATTR` 
macro:

```
#include "esp_attr.h"

void IRAM_ATTR gpio_isr_handler(void* arg)
{
    // ...
}
```

There are some possible issues with placement in IRAM, that may cause problems with IRAM-safe 
interrupt handlers:

- Strings or constants inside an `IRAM_ATTR` function may not be placed in RAM automatically. It is 
possible to use `DRAM_ATTR` attributes to mark these, or using the linker script method will cause 
these to be automatically placed correctly.
	```c
	void IRAM_ATTR gpio_isr_handler(void* arg)
	{
	   const static DRAM_ATTR uint8_t INDEX_DATA[] = { 45, 33, 12, 0 };
	   const static char *MSG = DRAM_STR("I am a string stored in RAM");
	}
	```

Note that knowing which data should be marked with `DRAM_ATTR` can be hard, the compiler will 
sometimes recognize that a variable or expression is constant (even if it is not marked `const`) 
and optimize it into flash, unless it is marked with `DRAM_ATTR`.

- GCC optimizations that automatically generate jump tables or switch/case lookup tables place 
these tables in flash. IDF by default builds all files with `-fno-jump-tables 
-fno-tree-switch-conversion` flags to avoid this.

Jump table optimizations can be re-enabled for individual source files that do not need to be 
placed in IRAM. For instructions on how to add the `-fno-jump-tables -fno-tree-switch-conversion` 
options when compiling individual source files, see [Controlling Component 
Compilation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-guides/build-system.h
tml#component-build-control).

## IROM (Code Executed from flash)

If a function is not explicitly placed into or RTC memory, it is placed into flash. As IRAM is 
limited, most of an application's binary code must be placed into IROM instead.

During [Application Startup 
Flow](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-guides/startup.html), the 
bootloader (which runs from IRAM) configures the MMU flash cache to map the app's instruction code 
region to the instruction space. Flash accessed via the MMU is cached using some internal SRAM and 
accessing cached flash data is as fast as accessing other types of internal memory.

## DROM (Data Stored in flash)

By default, constant data is placed by the linker into a region mapped to the MMU flash cache. This 
is the same as the section, but is for read-only data not executable code.

The only constant data not placed into this memory type by default are literal constants which are 
embedded by the compiler into application code. These are placed as the surrounding function's 
executable instructions.

The `DRAM_ATTR` attribute can be used to force constants from DROM into the section (see above).

## RTC FAST Memory

Remaining RTC FAST memory is added to the heap unless the option 
[CONFIG\_ESP\_SYSTEM\_ALLOW\_RTC\_FAST\_MEM\_AS\_HEAP](https://docs.espressif.com/projects/esp-idf/e
n/stable/esp32p4/api-reference/kconfig-reference.html#config-esp-system-allow-rtc-fast-mem-as-heap) 
is disabled. This memory can be used interchangeably with, but is slightly slower to access.

## SPM (Scratchpad Memory)

SPM (Scratchpad Memory) is a dedicated on-chip memory located near the processor core, offering 
deterministic access timing. SPM does not rely on hardware caching mechanisms; instead, its access 
is explicitly managed by software, ensuring predictable and stable latency. The access latency of 
SPM is configuration dependent. When parity check is enabled, the latency is 4 clock cycles and 
memory bandwidth is reduced. When parity check is disabled, the latency is 1 clock cycle. This type 
of memory is typically used to store critical code and data that are sensitive to access timing, 
making it suitable for real-time systems or embedded applications with strict performance and 
response time requirements.

## DMA-Capable Requirement

Most peripheral DMA controllers (e.g., SPI, sdmmc, etc.) have requirements that sending/receiving 
buffers should be placed in DRAM and word-aligned. We suggest to place DMA buffers in static 
variables rather than in the stack. Use macro `DMA_ATTR` to declare global/local static variables 
like:

```c
DMA_ATTR uint8_t buffer[]="I want to send something";

void app_main()
{
    // initialization code...
    spi_transaction_t temp = {
        .tx_buffer = buffer,
        .length = 8 * sizeof(buffer),
    };
    spi_device_transmit(spi, &temp);
    // other stuff
}
```

Or:

```c
void app_main()
{
    DMA_ATTR static uint8_t buffer[] = "I want to send something";
    // initialization code...
    spi_transaction_t temp = {
        .tx_buffer = buffer,
        .length = 8 * sizeof(buffer),
    };
    spi_device_transmit(spi, &temp);
    // other stuff
}
```

It is also possible to allocate DMA-capable memory buffers dynamically by using the 
[MALLOC\_CAP\_DMA](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/syste
m/mem_alloc.html#dma-capable-memory) capabilities flag.

## DMA Buffer in the Stack

Placing DMA buffers in the stack is possible but discouraged. If doing so, pay attention to the 
following:

- Placing DRAM buffers on the stack is not recommended if the stack may be in PSRAM. If the stack 
of a task is placed in the PSRAM, several steps have to be taken as described in [Support for 
External 
RAM](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-guides/external-ram.html).
- Use macro `WORD_ALIGNED_ATTR` in functions before variables to place them in proper positions 
like:
	```c
	void app_main()
	{
	    uint8_t stuff;
	    WORD_ALIGNED_ATTR uint8_t buffer[] = "I want to send something";   //or the buffer will 
be placed right after stuff.
	    // initialization code...
	    spi_transaction_t temp = {
	        .tx_buffer = buffer,
	        .length = 8 * sizeof(buffer),
	    };
	    spi_device_transmit(spi, &temp);
	    // other stuff
	}
	```
