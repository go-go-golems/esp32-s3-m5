## Support for External RAM

[\[中文\]](../../../../zh_CN/v5.4.2/esp32p4/api-guides/external-ram.html)

## Introduction

ESP32-P4 has a few hundred kilobytes of internal RAM, residing on the same die as the rest of the chip components. It can be insufficient for some purposes, so ESP32-P4 has the ability to use up to 64 MB of virtual addresses for external PSRAM (Psuedostatic RAM) memory. The external memory is incorporated in the memory map and, with certain restrictions, is usable in the same way as internal data RAM.

## Hardware

ESP32-P4 supports PSRAM connected in parallel with the SPI flash chip. While ESP32-P4 is capable of supporting several types of RAM chips, ESP-IDF currently only supports Espressif branded PSRAM chips (e.g., ESP-PSRAM32, ESP-PSRAM64, etc).

Note

Some PSRAM chips are 1.8 V devices and some are 3.3 V. Consult the datasheet for your PSRAM chip and ESP32-P4 device to find out the working voltages.

By default, the PSRAM is powered up by the on-chip LDO2. You can use [CONFIG\_ESP\_LDO\_CHAN\_PSRAM\_DOMAIN](../api-reference/kconfig.html#config-esp-ldo-chan-psram-domain) to switch the LDO channel accordingly. Set this value to -1 to use an external power supply, which means the on-chip LDO will not be used. By default, the PSRAM connected to LDO is set to the correct voltage based on the Espressif module used. You can still use [CONFIG\_ESP\_LDO\_VOLTAGE\_PSRAM\_DOMAIN](../api-reference/kconfig.html#config-esp-ldo-voltage-psram-domain) to select the LDO output voltage if you are not using an Espressif module. When using an external power supply, this option does not exist.

Note

Espressif produces both modules and system-in-package chips that integrate compatible PSRAM and flash and are ready to mount on a product PCB. Consult the Espressif website for more information. If you are using a custom PSRAM chip, ESP-IDF SDK might not be compatible with it.

For specific details about connecting the SoC or module pins to an external PSRAM chip, consult the SoC or module datasheet.

## Configuring External RAM

ESP-IDF fully supports the use of external RAM in applications. Once the external RAM is initialized at startup, ESP-IDF can be configured to integrate the external RAM in several ways:

- (default)

### Integrate RAM into the ESP32-P4 Memory Map

Select this option by choosing `Integrate RAM into memory map` from [CONFIG\_SPIRAM\_USE](../api-reference/kconfig.html#config-spiram-use).

This is the most basic option for external RAM integration. Most likely, you will need another, more advanced option.

During the ESP-IDF startup, external RAM is mapped into the data virtual address space. The address space is dynamically allocated. The length will be the minimum length between the PSRAM size and the available data virtual address space size.

Applications can manually place data in external memory by creating pointers to this region. So if an application uses external memory, it is responsible for all management of the external RAM: coordinating buffer usage, preventing corruption, etc.

It is recommended to access the PSRAM by ESP-IDF heap memory allocator (see next chapter).

### Add External RAM to the Capability Allocator

Select this option by choosing `Make RAM allocatable using heap_caps_malloc(..., MALLOC_CAP_SPIRAM)` from [CONFIG\_SPIRAM\_USE](../api-reference/kconfig.html#config-spiram-use).

When enabled, memory is mapped to data virtual address space and also added to the [capabilities-based heap memory allocator](../api-reference/system/mem_alloc.html) using `MALLOC_CAP_SPIRAM`.

To allocate memory from external RAM, a program should call `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)`. After use, this memory can be freed by calling the normal `free()` function.

### Provide External RAM via malloc()

Select this option by choosing `Make RAM allocatable using malloc() as well` from [CONFIG\_SPIRAM\_USE](../api-reference/kconfig.html#config-spiram-use). This is the default option.

In this case, memory is added to the capability allocator as described for the previous option. However, it is also added to the pool of RAM that can be returned by the standard `malloc()` function.

This allows any application to use the external RAM without having to rewrite the code to use `heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`.

An additional configuration item, [CONFIG\_SPIRAM\_MALLOC\_ALWAYSINTERNAL](../api-reference/kconfig.html#config-spiram-malloc-alwaysinternal), can be used to set the size threshold when a single allocation should prefer external memory:

- When allocating a size less than or equal to the threshold, the allocator will try internal memory first.
- When allocating a size larger than the threshold, the allocator will try external memory first.

If a suitable block of preferred internal/external memory is not available, the allocator will try the other type of memory.

Because some buffers can only be allocated in internal memory, a second configuration item [CONFIG\_SPIRAM\_MALLOC\_RESERVE\_INTERNAL](../api-reference/kconfig.html#config-spiram-malloc-reserve-internal) defines a pool of internal memory which is reserved for *only* explicitly internal allocations (such as memory for DMA use). Regular `malloc()` will not allocate from this pool. The [MALLOC\_CAP\_DMA](../api-reference/system/mem_alloc.html#dma-capable-memory) and `MALLOC_CAP_INTERNAL` flags can be used to allocate memory from this pool.

### Allow.bss Segment to Be Placed in External Memory

Enable this option by checking [CONFIG\_SPIRAM\_ALLOW\_BSS\_SEG\_EXTERNAL\_MEMORY](../api-reference/kconfig.html#config-spiram-allow-bss-seg-external-memory).

If enabled, the region of the data virtual address space where the PSRAM is mapped to will be used to store zero-initialized data (BSS segment) from the lwIP, net80211, libpp, wpa\_supplicant and bluedroid ESP-IDF libraries.

Additional data can be moved from the internal BSS segment to external RAM by applying the macro `EXT_RAM_BSS_ATTR` to any static declaration (which is not initialized to a non-zero value).

It is also possible to place the BSS section of a component or a library to external RAM using linker fragment scheme `extram_bss`.

This option reduces the internal static memory used by the BSS segment.

Remaining external RAM can also be added to the capability heap allocator using the method shown above.

### Allow.noinit Segment to Be Placed in External Memory

Enable this option by checking [CONFIG\_SPIRAM\_ALLOW\_NOINIT\_SEG\_EXTERNAL\_MEMORY](../api-reference/kconfig.html#config-spiram-allow-noinit-seg-external-memory). If enabled, the region of the data virtual address space where the PSRAM is mapped to will be used to store non-initialized data. The values placed in this segment will not be initialized or modified even during startup or restart.

By applying the macro `EXT_RAM_NOINIT_ATTR`, data could be moved from the internal NOINIT segment to external RAM. Remaining external RAM can still be added to the capability heap allocator using the method shown above, .

#### Execute In Place (XiP) from PSRAM

The [CONFIG\_SPIRAM\_XIP\_FROM\_PSRAM](../api-reference/kconfig.html#config-spiram-xip-from-psram) option enables the executable in place (XiP) from PSRAM feature. With this option sections that are normally placed in flash, `.text` (for instructions) and `.rodata` (for read only data), will be loaded in PSRAM.

With this option enabled, the cache will not be disabled during an SPI1 flash operation, so code that requires executing during an SPI1 flash operation does not have to be placed in internal RAM.

Since the flash and PSRAM in ESP32-P4 use two separate SPI buses, moving flash content to PSRAM will actually increase the load on the PSRAM MSPI bus. Therefore, the exact impact on performance will be dependent on your app usage of PSRAM.

The PSRAM bus can operate at a higher speed than the flash bus. For example, if the PSRAM is a HEX (16-line PSRAM on ESP32P4) running at 200 MHz, it is significantly faster than a Quad flash (4-line flash) running at 80 MHz.

If the instructions and data previously stored in flash are not accessed frequently, then enabling this option could improve performance. It is recommended to conduct performance profiling to evaluate how this option will affect your system.

## Restrictions

External RAM use has the following restrictions:

- When flash cache is disabled (for example, if the flash is being written to), the external RAM also becomes inaccessible. Any read operations from or write operations to it will lead to an illegal cache access exception. This is also the reason why ESP-IDF does not by default allocate any task stacks in external RAM (see below).
- External RAM uses the same cache region as the external flash. This means that frequently accessed variables in external RAM can be read and modified almost as quickly as in internal RAM. However, when accessing large chunks of data (> 32 KB), the cache can be insufficient, and speeds will fall back to the access speed of the external RAM. Moreover, accessing large chunks of data can "push out" cached flash, possibly making the execution of code slower afterwards.
- In general, external RAM will not be used as task stack memory. [`xTaskCreate()`](../api-reference/system/freertos_idf.html#_CPPv411xTaskCreate14TaskFunction_tPCKcK22configSTACK_DEPTH_TYPEPCv11UBaseType_tPC12TaskHandle_t "xTaskCreate") and similar functions will always allocate internal memory for stack and task TCBs.

The option [CONFIG\_FREERTOS\_TASK\_CREATE\_ALLOW\_EXT\_MEM](../api-reference/kconfig.html#config-freertos-task-create-allow-ext-mem) can be used to allow placing task stacks into external memory. In these cases [`xTaskCreateStatic()`](../api-reference/system/freertos_idf.html#_CPPv417xTaskCreateStatic14TaskFunction_tPCKcK8uint32_tPCv11UBaseType_tPC11StackType_tPC12StaticTask_t "xTaskCreateStatic") must be used to specify a task stack buffer allocated from external memory, otherwise task stacks will still be allocated from internal memory.

## Failure to Initialize

By default, failure to initialize external RAM will cause the ESP-IDF startup to abort. This can be disabled by enabling the config item [CONFIG\_SPIRAM\_IGNORE\_NOTFOUND](../api-reference/kconfig.html#config-spiram-ignore-notfound).

## Encryption

It is possible to enable automatic encryption for data stored in external RAM. When this is enabled any data read and written through the cache will automatically be encrypted or decrypted by the external memory encryption hardware.

This feature is enabled whenever flash encryption is enabled. For more information on how to enable and how it works see [Flash Encryption](../security/flash-encryption.html).

---

**Was this page helpful?**

- Thank you! We received your feedback.
- If you have any comments, fill in [Espressif Documentation Feedback Form](<https://www.espressif.com/en/company/documents/documentation_feedback?docId=4287&sections=Support for External RAM \(api-guides/external-ram\)&version=esp32p4 v5.4.2 \(v5.4.2\)>).

- We value your feedback.
- Let us know how we can improve this page by filling in [Espressif Documentation Feedback Form](<https://www.espressif.com/en/company/documents/documentation_feedback?docId=4287&sections=Support for External RAM \(api-guides/external-ram\)&version=esp32p4 v5.4.2 \(v5.4.2\)>).
