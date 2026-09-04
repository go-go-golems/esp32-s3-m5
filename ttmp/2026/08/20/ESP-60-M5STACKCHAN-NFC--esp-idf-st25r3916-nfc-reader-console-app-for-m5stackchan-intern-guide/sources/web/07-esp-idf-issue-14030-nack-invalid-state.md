# I2C NACK causes library to always return ESP_ERR_INVALID_STATE, solved by printf (IDFGH-13084)

- **Canonical URL:** https://github.com/espressif/esp-idf/issues/14030
- **Repository:** `espressif/esp-idf`
- **Issue:** #14030
- **State at retrieval:** open
- **Created:** 2024-06-20T08:34:13Z
- **Updated:** 2026-08-10T14:26:07Z
- **Retrieved:** 2026-08-21
- **Labels:** Type: Bug, Awaiting Response, Status: Opened

> [!note] Source snapshot
> This file preserves an external issue discussion as research evidence. Claims in comments are reports from participants, not automatically verified facts. Consult the canonical issue for later updates.

## Issue body

### Answers checklist.

- [X] I have read the documentation [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/) and the issue is not addressed there.
- [X] I have updated my IDF branch (master or release) to the latest version and checked that the issue is present there.
- [X] I have searched the issue tracker for a similar issue and not found a similar issue.

### IDF version.

v5.2.2-169-g8e82062d41

### Espressif SoC revision.

wokwi ESP32

### Operating System used.

Windows

### How did you build your project?

VS Code IDE

### If you are using Windows, please specify command line type.

None

### Development Kit.

wokwi

### Power Supply used.

USB

### What is the expected behavior?

I2C functions (like `i2c_master_transmit`) should succeed after a few attempts.
I have a custom chip in Wokwi simulator that randomly returns NACK (1 out of 5 times: `rand()%5==0`)


### What is the actual behavior?

They always return ESP_ERR_INVALID_STATE.
After the custom Wokwi slave chip fails once, it then fails every time after that.
Even if I call the function 100 times after that.
But, **if I add printf inside i2c_master.c, it works**. <- this is important
(note: therefore it is probably not because of Wokwi or custom chip, because they are not dependent on i2c_master.c)

Also, function on_connect on custom chip is never called again, only on_disconnect

### Steps to reproduce.

1. Call i2c function (e.g. i2c_master_transmit)
2. The custom chip fails (NACK) (random 1 out of 5 times)
3. call it a few more times
4. return code after that is always ESP_ERR_INVALID_STATE

[gitbug.zip](https://github.com/user-attachments/files/15911108/gitbug.zip)




### Debug Logs.

```plain
not much to log
for `printf("%d: 0x%x", __LINE__, ret_error)`


66: 0
67: 0
69: 0
66: 0
67: 0
69: 0
57: 103
58: 103
59: 103

chip has following logs:
Hello from custom ALU chip!
on connect
1
I2C Process connected
PAR1 := 0.000000
PAR1 := 0.000000
PAR1 := 0.000000
PAR1 := 3.000000
I2C Process disconected!!!
on connect
2
I2C Process connected
Operation := 5
I2C Process disconected!!!
on connect
1
I2C Process connected
Sending rez [9.000000] [0][0]
Sending rez [9.000000] [1][0]
Sending rez [9.000000] [2][16]
Sending rez [9.000000] [3][65]
I2C Process disconected!!!
on connect
1
I2C Process connected
PAR1 := 3.000000
PAR1 := 3.000000
PAR1 := 4.000000
PAR1 := 4.000000
I2C Process disconected!!!
on connect
4
I2C Process connected
Operation := 5
I2C Process disconected!!!
on connect
1
I2C Process connected
Sending rez [16.000000] [0][0]
Sending rez [16.000000] [1][0]
Sending rez [16.000000] [2][128]
Sending rez [16.000000] [3][65]
I2C Process disconected!!!
on connect
0
Device not ready. Please repeat commad
I2C Process disconected!!!
I2C Process disconected!!!
I2C Process disconected!!!
I2C Process disconected!!!
I2C Process disconected!!!
I2C Process disconected!!!
I2C Process disconected!!!
I2C Process disconected!!!
I2C Process disconected!!!
I2C Process disconected!!!
I2C Process disconected!!!
I2C Process disconected!!!
I2C Process disconected!!!
I2C Process disconected!!!
I2C Process disconected!!!
I2C Process disconected!!!
I2C Process disconected!!!
```


### More Information.

Important note: when adding `printf("staa = %x   ", i2c_master->status);` into i2c_master.c, it works.
I suspect it may have something to do with `atomic_store` operation
it may be that the compiler does out-of-order execution and the status is not assigned a value until after `s_i2c_send_commands` is called

## Comments

### Comment 1: mythbuster5

- **Created:** 2024-06-20T08:51:33Z
- **Updated:** 2024-06-20T09:02:58Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-2180158023

where you call the printf? And what is the optimization level?

### Comment 2: BartolHrg

- **Created:** 2024-06-20T09:42:46Z
- **Updated:** 2024-06-20T09:45:13Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-2180257458

I included modified i2c_master.c in zip file
though, printfs are commented
I put them inside `s_i2c_transaction_start` function around `s_i2c_send_commands` call

For printf inside my project, they are in NekiI2C.hpp after 5 calls to `i2c_master_transmit`/`i2c_master_receive` (last one is logged) (I put 5 since NACK is random, so after 5 times, it should be ACK)

Optimization level is debug -Og (in vscode menuconfig) (I didn't touch it).



### Comment 3: dvosully

- **Created:** 2024-08-19T12:01:34Z
- **Updated:** 2024-08-19T12:01:34Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-2296406650

I think I am also experiencing this behaviour, with a real I2C device.
I have not yet traced the I2C bus to verify there is actually a NACK occurring (I will do more detailed testing soon), but I sometimes get the following debug messages for the first I2C transaction:
```
E (1674) i2c.master: I2C hardware NACK detected
E (1694) i2c.master: I2C transaction unexpected nack detected
E (1704) i2c.master: s_i2c_synchronous_transaction(870): I2C transaction failed
E (1714) i2c.master: i2c_master_transmit(1072): I2C transaction failed
```

After getting this NACK, every subsequent I2C transaction fails with an ESP_ERR_INVALID_STATE error:
```
E (2074) i2c.master: s_i2c_synchronous_transaction(870): I2C transaction failed
E (2074) i2c.master: i2c_master_transmit_receive(1095): I2C transaction failed
W (2079) gps: GPS I2C read failed ESP_ERR_INVALID_STATE
E (2119) i2c.master: s_i2c_synchronous_transaction(870): I2C transaction failed
E (2119) i2c.master: i2c_master_transmit_receive(1095): I2C transaction failed
W (2124) gps: GPS I2C read failed ESP_ERR_INVALID_STATE
E (2164) i2c.master: s_i2c_synchronous_transaction(870): I2C transaction failed
E (2164) i2c.master: i2c_master_transmit_receive(1095): I2C transaction failed
W (2169) gps: GPS I2C read failed ESP_ERR_INVALID_STATE
```

The I2C device HAS been initialized correctly in the correct mode (this fault only occurs occasionally on the first I2C transaction, there is no change in the init between when it works and when it fails).
Usually applying an external reset will cause the I2C to initialise and work normally, sometimes after the external reset it still fails.
I have not yet tried inserting a printf as suggested above.

I am using:

- IDF v5.3.0
- a custom board
- I am building using the command prompt.
- I am using a custom 3.3V power supply with 3A capacity
- ESP32-S3-WROOM, rev 0.2
- Communicating with an I2C GPS chip (Quectel L76-L running an I2C capable firmware)


### Comment 4: AxelLin

- **Created:** 2024-08-27T00:52:06Z
- **Updated:** 2024-08-27T00:52:06Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-2311362873

> where you call the printf? And what is the optimization level?

@mythbuster5
Any progress for this issue?

### Comment 5: AxelLin

- **Created:** 2024-10-13T08:24:20Z
- **Updated:** 2024-10-13T08:24:20Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-2408880727

> I think I am also experiencing this behaviour, with a real I2C device. I have not yet traced the I2C bus to verify there is actually a NACK occurring (I will do more detailed testing soon), but I sometimes get the following debug messages for the first I2C transaction:
>
> ```
> E (1674) i2c.master: I2C hardware NACK detected
> E (1694) i2c.master: I2C transaction unexpected nack detected
> E (1704) i2c.master: s_i2c_synchronous_transaction(870): I2C transaction failed
> E (1714) i2c.master: i2c_master_transmit(1072): I2C transaction failed
> ```
>
> **After getting this NACK, every subsequent I2C transaction fails with an ESP_ERR_INVALID_STATE error:**

@mythbuster5 Do you have fix for this?

### Comment 6: libreo-mwebert

- **Created:** 2024-11-19T07:44:15Z
- **Updated:** 2024-11-19T07:44:15Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-2484926429

Push

### Comment 7: RogerDavisWork

- **Created:** 2024-12-06T15:25:11Z
- **Updated:** 2024-12-06T17:45:11Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-2523489454

I am having the exact same problem.  I am using the P4 eval kit and it occurs sometimes when I enable the touch screen.
     BSP_NULL_CHECK(disp_indev = bsp_display_indev_init(disp), NULL);
which is this code in the esp32_p4_function_ev_board.c file:

static lv_indev_t *bsp_display_indev_init(lv_display_t *disp)
{
    esp_lcd_touch_handle_t tp;
    BSP_ERROR_CHECK_RETURN_NULL(bsp_touch_new(NULL, &tp));
    assert(tp);

    /* Add touch input (for selected screen) */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = tp,
    };

    return lvgl_port_add_touch(&touch_cfg);
}

When it happens it soft reboots and does it over and over again forever.  It reports a NACK and the suggested fix of adding printf's does not help in my setup.
When I hard power cycle the board, it clears it about 50% of the time and then the board works perfectly.
It happens with IDF 5.3.1 and also 5.4.

Log output:
E (1484) i2c.master: I2C transaction unexpected nack detected
E (1484) i2c.master: s_i2c_synchronous_transaction(893): I2C transaction failed
E (1494) i2c.master: i2c_master_transmit_receive(1118): I2C transaction failed
E (1504) lcd_panel.io.i2c: panel_io_i2c_rx_buffer(140): i2c transaction failed
E (1514) GT911: touch_gt911_read_cfg(352): GT911 read error!
E (1514) GT911: esp_lcd_touch_new_i2c_gt911(122): GT911 init failed
E (1524) GT911: Error (0x103)! Touch controller GT911 initialization failed!



### Comment 8: GCTechnology

- **Created:** 2025-02-17T15:30:43Z
- **Updated:** 2025-02-17T15:30:43Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-2663449404

Any possible solutions? Same issue on https://github.com/espressif/esp-adf/issues/1334.

### Comment 9: vandy

- **Created:** 2025-03-11T14:15:17Z
- **Updated:** 2025-03-11T14:15:17Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-2714467710

I have something similar but also the code crashes.

I'm using two devices each on its own I2C port (`I2C_NUM_0` and `I2C_NUM_1` which use different pins).
```
i2c_device_config_t dev_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = DEVICE_ADDR,
    .scl_speed_hz = 100000,
};
```
I'm using function to read data from devices:
```
static int readResult(struct device *device) {
    uint8_t buffer[6];
    int error = i2c_master_receive(device->dev_handle, buffer, sizeof buffer, TIMEOUT_MS);
    if (error) {
        ESP_LOGE("APP", "Problems to read data, error: %d", error);
        return error;
    }
    // ...
}
```
Which is called in the loop by a task. I deliberately set low timeout (10 ms), after several successful readings there is always one with the error:
```
2025-03-11 16:42:11 E (30768) i2c.master: s_i2c_synchronous_transaction(920): I2C transaction failed
2025-03-11 16:42:11 E (30768) i2c.master: i2c_master_receive(1236): I2C transaction failed
2025-03-11 16:42:11 E (30778) APP: Problems to read data, error: 259

// and after that code crashes (function readResult returns, some logic in caller is executed)

2025-03-11 16:42:12 Guru Meditation Error: Core  0 panic'ed (Store access fault). Exception was unhandled.
2025-03-11 16:42:12
2025-03-11 16:42:12 Core  0 register dump:
2025-03-11 16:42:12 MEPC    : 0x40804b6c  RA      : 0x40804b1e  SP      : 0x4080fb10  GP      : 0x4080dac4
--- 2025-03-11 16:42:12 0x40804b6c: i2c_ll_read_rxfifo at D:/dev/github.com/esp-idf/components/hal/esp32h2/include/hal/i2c_ll.h:642
2025-03-11 16:42:12  (inlined by) i2c_isr_receive_handler at D:/dev/github.com/esp-idf/components/esp_driver_i2c/i2c_master.c:672
2025-03-11 16:42:12 0x40804b1e: i2c_isr_receive_handler at D:/dev/github.com/esp-idf/components/esp_driver_i2c/i2c_master.c:671

2025-03-11 16:42:12 TP      : 0x40812af0  T0      : 0x0000001f  T1      : 0x2000103c  T2      : 0x00000002
2025-03-11 16:42:12 S0/FP   : 0x40819690  S1      : 0x408196c4  A0      : 0x4080d404  A1      : 0x60005000
2025-03-11 16:42:12 A2      : 0x00000000  A3      : 0x000000a4  A4      : 0x00000000  A5      : 0x00000000
2025-03-11 16:42:12 A6      : 0x60005000  A7      : 0x00000001  S2      : 0x40819964  S3      : 0x00000001
2025-03-11 16:42:12 S4      : 0x00000000  S5      : 0x40819974  S6      : 0x00000000  S7      : 0x00000000
2025-03-11 16:42:12 S8      : 0x00000000  S9      : 0x00000000  S10     : 0x00000000  S11     : 0x00000000
2025-03-11 16:42:12 T3      : 0x4080d404  T4      : 0x00000900  T5      : 0x00000000  T6      : 0x00000000
2025-03-11 16:42:12 MSTATUS : 0x00001881  MTVEC   : 0x40800001  MCAUSE  : 0x00000007  MTVAL   : 0x00000000
--- 2025-03-11 16:42:12 0x40800001: _vector_table at D:/dev/github.com/esp-idf/components/riscv/vectors_intc.S:54

2025-03-11 16:42:12 MHARTID : 0x00000000
2025-03-11 16:42:12
2025-03-11 16:42:12 Stack memory:
2025-03-11 16:42:12 4080fb10: 0x00000000 0x40819974 0x000000a4 0x00000001 0x40819964 0x00000080 0x40819690 0x40804c42
--- 2025-03-11 16:42:12 0x40804c42: i2c_master_isr_handler_default at D:/dev/github.com/esp-idf/components/esp_driver_i2c/i2c_master.c:710

2025-03-11 16:42:12 4080fb30: 0x00000004 0x40818c14 0x00000001 0x4080347e 0x00000000 0x00000000 0x00000001 0x00000000
--- 2025-03-11 16:42:12 0x4080347e: esp_vApplicationTickHook at D:/dev/github.com/esp-idf/components/esp_system/freertos_hooks.c:36

2025-03-11 16:42:12 4080fb50: 0x00001881 0x8000000b 0x40819ac8 0x40800e42 0x00000000 0x80000008 0x40819690 0x4080c36c
--- 2025-03-11 16:42:12 0x40800e42: shared_intr_isr at D:/dev/github.com/esp-idf/components/esp_hw_support/intr_alloc.c:452
2025-03-11 16:42:12 0x4080c36c: _global_interrupt_handler at D:/dev/github.com/esp-idf/components/riscv/interrupt.c:69

2025-03-11 16:42:12 4080fb70: 0x00000000 0x00000000 0x00000000 0x4080026e 0x6000b000 0x40805f36 0x40805f42 0x00000000
--- 2025-03-11 16:42:13 0x4080026e: _interrupt_handler at D:/dev/github.com/esp-idf/components/riscv/vectors.S:369
2025-03-11 16:42:13 0x40805f36: systimer_ticks_to_us at D:/dev/github.com/esp-idf/components/esp_hw_support/port/esp32h2/systimer.c:15
2025-03-11 16:42:13 0x40805f42: systimer_us_to_ticks at D:/dev/github.com/esp-idf/components/esp_hw_support/port/esp32h2/systimer.c:20

2025-03-11 16:42:13 4080fb90: 0x4080fb8c 0x00000000 0x00000000 0x00000000 0x4080fba4 0xffffffff 0x4080fba4 0x4080fba4
2025-03-11 16:42:13 4080fbb0: 0x00000000 0x4080fbb8 0xffffffff 0x4080fbb8 0x4080fbb8 0x00000001 0x00000001 0x00000000
2025-03-11 16:42:13 4080fbd0: 0x0001ffff 0x00000000 0x00000000 0x00000000 0x00000000 0x4080fbe0 0x00000000 0x00000000
2025-03-11 16:42:13 4080fbf0: 0x00000000 0x4080fbf8 0xffffffff 0x4080fbf8 0x4080fbf8 0x00000000 0x4080fc0c 0xffffffff
2025-03-11 16:42:13 4080fc10: 0x4080fc0c 0x4080fc0c 0x00000001 0x00000001 0x00000000 0x0001ffff 0x00000000 0x00000000
2025-03-11 16:42:13 4080fc30: 0x00000000 0x6000b000 0x40805f36 0x40805f42 0x40814bac 0x40814bd4 0x40814bfc 0x40814c24
--- 2025-03-11 16:42:13 0x40805f36: systimer_ticks_to_us at D:/dev/github.com/esp-idf/components/esp_hw_support/port/esp32h2/systimer.c:15
2025-03-11 16:42:13 0x40805f42: systimer_us_to_ticks at D:/dev/github.com/esp-idf/components/esp_hw_support/port/esp32h2/systimer.c:20

2025-03-11 16:42:13 4080fc50: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fc70: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fc90: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fcb0: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fcd0: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fcf0: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fd10: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fd30: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fd50: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fd70: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fd90: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fdb0: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fdd0: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fdf0: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fe10: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fe30: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fe50: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fe70: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fe90: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080feb0: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fed0: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
2025-03-11 16:42:13 4080fef0: 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000 0x00000000
```

It looks like i2c_master (`i2c_master_receive`) code returns with error, my function returns with error, buffer is destroyed and somehow interrupt is trying to read data to provided buffer.

But if I put `ESP_LOGE(...)` inside [s_i2c_transaction_start](https://github.com/espressif/esp-idf/blob/94cfe394fe94af5e53da8398af5b886016e7fc11/components/esp_driver_i2c/i2c_master.c#L634) when there is a wrong state, crash doesn't happen. It seems like log takes a lot of time which allows interrupt to happen and not corrupt memory. My `i2c_master->status` is always `7` (when error happens) which is timeout. But I don't see any error logs about timeouts which exist in `s_i2c_send_commands`.

### Comment 10: eriksl

- **Created:** 2025-04-23T10:51:08Z
- **Updated:** 2025-04-23T10:51:08Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-2823886274

IMHO there are two issues:
- unexpected NAK's
- IDF driver code verbosely reporting this fact

Ad 2) sometimes a NAK is unavoidable (e.g. when a slave isn't ready to address, that's it way to say: retry later). So this is not always an ERROR. I don't think the IDF driver should report it on the console, an "error" status return is sufficient.
Ad 1) I've seen this phenomenon a few times too. I2C reporting a NAK while no NAK was sent (NAK = failure to pull SDA low (in time)). I think I2C module samples the SDA level exactly when the SCL level has returned to 1. Some slaves may be a bit slow to pull SDA in some cases. For those devices it would be beneficial to have the possibility to skew SDA sampling a few nanoseconds from SCL clock rise.

### Comment 11: AxelLin

- **Created:** 2025-05-06T02:13:33Z
- **Updated:** 2025-05-06T02:13:33Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-2853075270

@mythbuster5 How is the status of this issue?

### Comment 12: mythbuster5

- **Created:** 2025-05-12T02:48:56Z
- **Updated:** 2025-05-12T02:48:56Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-2870570495

I think for the original issue, it can be solved after this commit https://github.com/espressif/esp-idf/commit/459b75f81a121dc83beb103a10aee8216c657fce

### Comment 13: andrew-sz

- **Created:** 2025-06-25T15:15:58Z
- **Updated:** 2025-06-25T15:15:58Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-3005167453

I have not tested the "fix" yet.  I'm currently on esp-idf v5.4.1 and this happened after more than an hour of testing.  Is this the same "bug"?  025-06-25 23:12:40:Temperature: 27.67 °C, Humidity: 39.14 %
2025-06-25 23:12:42:Temperature: 27.70 °C, Humidity: 39.14 %
2025-06-25 23:12:44:.[0;31mE (5366915) i2c.master: I2C transaction timeout detected.[0m
2025-06-25 23:12:44:.[0;31mE (5366915) i2c.master: s_i2c_synchronous_transaction(924): I2C transaction failed.[0m
2025-06-25 23:12:45:.[0;31mE (5366918) i2c.master: i2c_master_receive(1240): I2C transaction failed.[0m
2025-06-25 23:12:45:.[0;31mE (5366925) aht20: Failed to read data: ESP_ERR_INVALID_STATE.[0m
2025-06-25 23:12:45:Failed to read measurements from AHT20 sensor
2025-06-25 23:12:46:Guru Meditation Error: Core  1 panic'ed (StoreProhibited). Exception was unhandled.
2025-06-25 23:12:46:
2025-06-25 23:12:46:Core  1 register dump:
2025-06-25 23:12:46:PC      : 0x40379ebc  PS      : 0x00060033  A0      : 0x80379f61  A1      : 0x3fc996a0
2025-06-25 23:12:46:A2      : 0x3fca8b60  A3      : 0x3fca8b70  A4      : 0x3fc996d0  A5      : 0x00000000
2025-06-25 23:12:46:A6      : 0x3fca8b94  A7      : 0x3fca8e8c  A8      : 0x00000000  A9      : 0x00000000
2025-06-25 23:12:46:A10     : 0x0000001c  A11     : 0x00000000  A12     : 0x60013000  A13     : 0x3fc9f800
2025-06-25 23:12:46:A14     : 0x60013000  A15     : 0x0000abab  SAR     : 0x00000000  EXCCAUSE: 0x0000001d
2025-06-25 23:12:46:EXCVADDR: 0x00000000  LBEG    : 0x40056f5c  LEND    : 0x40056f72  LCOUNT  : 0xffffffff
2025-06-25 23:12:46:
2025-06-25 23:12:46:
2025-06-25 23:12:46:Backtrace: 0x40379eb9:0x3fc996a0 0x40379f5e:0x3fc996d0 0x40376a31:0x3fc99710 0x403777ed:0x3fc99730 0x4037ad17:0x3fca0730 0x420038f6:0x3fca0750 0x4037fd4d:0x3fca0770 0x4037ee21:0x3fca0790



### Comment 14: andrew-sz

- **Created:** 2025-06-25T15:17:36Z
- **Updated:** 2025-06-25T15:19:08Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-3005172440

I can post entire log, but it was literally after an hour of successful readings.  If this is not the same bug then please let me know and I'll create a new issue.  At the very least, I need to catch this exception as I can't have this issue reset my board.


### Comment 15: andrew-sz

- **Created:** 2025-06-25T15:38:18Z
- **Updated:** 2025-06-25T15:40:17Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-3005235624

> IMHO there are two issues:
>
> * unexpected NAK's
> * IDF driver code verbosely reporting this fact
>
> Ad 2) sometimes a NAK is unavoidable (e.g. when a slave isn't ready to address, that's it way to say: retry later). So this is not always an ERROR. I don't think the IDF driver should report it on the console, an "error" status return is sufficient. Ad 1) I've seen this phenomenon a few times too. I2C reporting a NAK while no NAK was sent (NAK = failure to pull SDA low (in time)). I think I2C module samples the SDA level exactly when the SCL level has returned to 1. Some slaves may be a bit slow to pull SDA in some cases. For those devices it would be beneficial to have the possibility to skew SDA sampling a few nanoseconds from SCL clock rise.

What is an unexpected NACK?  Shouldn't this always be handled in user code?  i.e. device not ready, or something?  the bus master should not fail on a NACK like, ever, in my thinking.

### Comment 16: eriksl

- **Created:** 2025-06-25T15:46:00Z
- **Updated:** 2025-06-25T15:46:00Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-3005259964

Exactly. IDF code should not concern nor report about NAK's, as the IDF can't know what's "expected" (or not). It should simply return a **valid** or **invalid** status code.

I read in the development branch an alternative interface is available where you can have much more control over the transaction. I hope it will get into main soon.

### Comment 17: andrew-sz

- **Created:** 2025-06-25T15:50:59Z
- **Updated:** 2025-06-25T16:38:26Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-3005288137

> Exactly. IDF code should not concern nor report about NAK's, as the IDF can't know what's "expected" (or not). It should simply return a **valid** or **invalid** status code.
>
> I read in the development branch an alternative interface is available where you can have much more control over the transaction. I hope it will get into main soon.

I hope so also!  Until then I will catch exception and handle in my code. I2C master should report bus status and comms like a NACK from a peripheral but not timeout because of it. Bus controller shouldn't care.  Send data/Receive data...if Data is NACK then report this upstream to app/customer driver.  A peripheral NACK should NOT crash bus driver.

Sorry one more edit.

### Comment 18: luftaquila

- **Created:** 2025-07-22T14:22:36Z
- **Updated:** 2025-07-22T14:31:29Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-3102974134

Experiencing exact same issue here; the transfer randomly fails.

100+ times testing shows me that sometimes it fails and sometimes succeed with no issue.

If it fails, there are 2 cases: `unexpected nack detected` message present or not.

Returned error is always `ESP_ERR_INVALID_STATE`

```
E (410) i2c.master: s_i2c_synchronous_transaction(945): I2C transaction failed
E (410) i2c.master: i2c_master_transmit_receive(1241): I2C transaction failed
GYR error: ESP_ERR_INVALID_STATE 198083
W (410) GYRO: read transfer failure
E (410) i2c.master: I2C transaction unexpected nack detected
E (410) i2c.master: s_i2c_synchronous_transaction(945): I2C transaction failed
E (410) i2c.master: i2c_master_transmit_receive(1241): I2C transaction failed
GYR error: ESP_ERR_INVALID_STATE 201785
W (410) GYRO: read transfer failure
```

Adding `i2c_master_bus_wait_all_done(i2c0, 50);` didn't solved the issue.

### Comment 19: ryancog

- **Created:** 2025-09-06T04:09:14Z
- **Updated:** 2025-09-06T04:09:14Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-3260455504

I'm seeing similar behavior with crashes after a failed i2c master transaction. I see ESP_ERR_INVALID_STATE returned from s_i2c_synchronous_transaction and shortly after the chip crashes.

This is on esp32s3 on esp-idf v5.4.2. The crash occurs in the ISR handler i2c_master_isr_handler_default, specifically within i2c_ll_read_rxfifo, as seen in @vandy's output above.

### Comment 20: eriksl

- **Created:** 2025-09-06T07:15:12Z
- **Updated:** 2025-09-06T07:15:12Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-3261475845

I believe some fixes where applied in v5.5.

### Comment 21: ryancog

- **Created:** 2025-09-10T15:57:49Z
- **Updated:** 2025-09-10T15:57:49Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-3275577630

Thank you @eriksl. For now I'm unable to build v5.5.1 (#17567), but I'll test it out once I'm able to and update here.

### Comment 22: mmackh

- **Created:** 2025-10-31T17:07:07Z
- **Updated:** 2025-10-31T17:07:07Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-3474041448

> I believe some fixes where applied in v5.5.

We're running 5.5 and are experiencing a similar issue. Once I2C fails randomly with a NACK, we cannot recover to a working state, unless restarting.

### Comment 23: mythbuster5

- **Created:** 2025-11-07T09:25:40Z
- **Updated:** 2025-11-07T09:25:40Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-3501472698

@mmackh Could you please check on line data via logic analyzer to check what is happening around nack. like unknown wave or stm. Because master should not have behavior like that,.

### Comment 24: Alvin1Zhang

- **Created:** 2025-11-19T03:20:21Z
- **Updated:** 2025-11-19T03:20:21Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-3550524969

@mmackh @BartolHrg Thanks for reporting, would you please help share if any updates for the issue? Thanks.

### Comment 25: mmackh

- **Created:** 2025-11-19T06:21:55Z
- **Updated:** 2025-11-19T06:21:55Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-3550992506

We have currently switched over to the legacy I2C driver due to more stability with our setup. Before the hardware side finalises a new revision, we will check with a logic analyzer with the new driver again

### Comment 26: sandynomad

- **Created:** 2026-01-03T19:31:34Z
- **Updated:** 2026-01-04T02:25:15Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-3707299791

I'm seeing a similar behaviour with "i2c_master" and idf-release_v5.5-9bb7aa84-v2

    uint8_t cmd = QMI8658_REVISION_ID;
    uint8_t tmp;

    ESP_ERROR_CHECK( i2c_master_transmit_receive( dev_handle, &cmd, 1, &tmp, 1, -1 ) )

results in:

    ESP_ERROR_CHECK failed: esp_err_t 0x103 (ESP_ERR_INVALID_STATE) at 0x42006185

*ESP_ERR_INVALID_STATE* is not listed as a valid error for *i2c_master_transmit_receive*

I2C communication to another chip on the bus works ok; using the legacy I2C driver with the QMI8658 also works ok.

Increasing the debug level, this is displayed:

    D (668) i2c.master: I2C transaction unexpected nack detected

### Comment 27: sandynomad

- **Created:** 2026-01-05T10:57:25Z
- **Updated:** 2026-01-05T10:57:25Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-3709943655

Updating my last comment - it seems the ESP32s3 board I was using failed mid-test hence the results. While trying to diagnose this issue further the vendor demo software now also fails. Testing my original code with a QMI8658 on a different ESP32s3 board works fine.


### Comment 28: timbo100

- **Created:** 2026-03-12T05:18:33Z
- **Updated:** 2026-03-12T05:18:33Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-4044052236

I've been working on my own C++ abstraction for I2CBus.  I've done a lot of exploring with my v5.5.1 install on Ubuntu by building a scanner from other examples.  end result was that virtually all of my i2c master code had to be compiled with C.
But in the process I discovered that when my scanner calls  i2c_new_master_bus() and DOESN't call "bypass_nac_log=true I would get "I2C transactoin unexpected nack detected" from on all non replies during the i2c Bus scan.

Here the important code fragments that made the scanner finally work without nack errors:

file: scanner.c

```
....
#include "i2c_private.h"	 // include IDF_PATH + /components/esp_driver_i2c/i2c_private.h" in CMakeLists.txt
.....
void scanner() {
	i2c_master_bus_config_t i2c_mst_config ;
	i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
	i2c_mst_config.i2c_port = I2C_NUM_0;
	i2c_mst_config.scl_io_num = I2C_MASTER_SCL_IO;
	i2c_mst_config.sda_io_num = I2C_MASTER_SDA_IO;
	i2c_mst_config.glitch_ignore_cnt = 7;
	// .intr_priority = 2,   // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/i2c.html#install-i2c-slave-device
	// i2c_mst_config.flags.enable_internal_pullup = true; //  this works but should be for two devices
	i2c_mst_config.flags.enable_internal_pullup = false; // when one i2c dev on bus this works

	ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
	/** the following is needed otherwise we get NACK errors */
	bus_handle->bypass_nack_log= true; // if this is not in effect then we get  --E (827) i2c.master: I2C transaction unexpected nack detected

	uint8_t data_wr[DATA_LENGTH];
	data_wr[0]= 0xaa;

	printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	printf("00:         ");

	for (uint8_t i=3; i<= 0x7F; i++) 	{
.....
```
Here's the output area that my BME688 device is:

```
.....
70: --E (2548) i2c.master: I2C transaction unexpected nack detected
 --E (2558) i2c.master: I2C transaction unexpected nack detected
 --E (2568) i2c.master: I2C transaction unexpected nack detected
 --E (2578) i2c.master: I2C transaction unexpected nack detected
 --E (2578) i2c.master: I2C transaction unexpected nack detected
 --E (2588) i2c.master: I2C transaction unexpected nack detected
 -- 77E (2598) i2c.master: I2C transaction unexpected nack detected
 --E (2598) i2c.master: I2C transaction unexpected nack detected
 --E (2608) i2c.master: I2C transaction unexpected nack detected
 --E (2618) i2c.master: I2C transaction unexpected nack detected
 --E (2618) i2c.master: I2C transaction unexpected nack detected
 --E (2628) i2c.master: I2C transaction unexpected nack detected
 --E (2638) i2c.master: I2C transaction unexpected nack detected
 --E (2638) i2c.master: I2C transaction unexpected nack detected
 --
.....
```

I hope this is helpful....

I figure this out while trying to debug another issues I've reported at
[components/esp_driver_i2c/i2c_private.h:138:5: error: '_Atomic' does not name a type](https://github.com/espressif/esp-modbus/issues/160)


### Comment 29: eriksl

- **Created:** 2026-03-12T13:57:47Z
- **Updated:** 2026-03-12T13:58:05Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-4046985134

I have written a "full" I2C abstraction in C++, that, wherever possible, hands the same interface to both "main" I2C modules and the ULP module. This has some limitations because the ULP module is rather limited, but as long as you refrain from certain procedures (like reading the bus without writing anything first), it sort of works. Also the ULP can't properly probe devices, so you really need to write+read to probe.

For the main modules I use the "i2c_master_execute_defined_operations" interface for maximum control. It's way better than the high-level functions, but still it leaves some things to desire. One of them is that the way the operations struct is built, suggests that it's passed to the I2C hardware 1:1. That's not the case. Some combinations of "operations" are legal in I2C, the hardware can perform them, but it's not possible to build them using the command struct. That was a major disappointment.  The struct is copied by the IDF code into another struct that actually IS used to drive the hardware and has a different layout. So I don't understand why they made the struct for the interface ("i2c_master_execute_defined_operations") so clumsy to begin with.

Anyway, during the probing stage, my logs would clog with all sorts of spurious messages from the IDF code. I've done a few proposals and some of them have been accepted in the meantime.

Note that there is an option to ignore NACK's, but that's useless because for probing, the NACK is actually required to be seen. We just don't want all that log clogging. For that I am using a patch to ESP-IDF, I hope one day it will  be applied one way or another, maybe with an option to selectively mute logging.

Regarding the pull-ups: yes it's really annoying that it's always logged when you're not using the internal pull-ups. I'd really advice to stay away from internal pull-ups, no matter how many devices or how long/short the bus. The internall pull-ups are really far too weak to be reliable. They're actually implemented as a weak current source, not a resistor. I2C needs 2 kOhms (for fast devices or large buses that have many devices, but all devices need to be able to drive such a strong current) up to 10 kOhms. I always use the go-in-between value of 4k7 Ohms, which is also recommended by NXP. This creates a much stronger pull-up than the internal ones.

This is the patch I use to ESP-IDF (to the current or almost current version):

```
diff --git a/components/esp_driver_i2c/i2c_master.c b/components/esp_driver_i2c/i2c_master.c
index 162042c701..1c22f8a572 100644
--- a/components/esp_driver_i2c/i2c_master.c
+++ b/components/esp_driver_i2c/i2c_master.c
@@ -153,14 +153,6 @@ static esp_err_t s_i2c_hw_fsm_reset(i2c_master_bus_handle_t i2c_master, bool cle

 static void s_i2c_err_log_print(i2c_master_event_t event, bool bypass_nack_log)
 {
-    if (event == I2C_EVENT_TIMEOUT) {
-        ESP_LOGE(TAG, "I2C transaction timeout detected");
-    }
-    if (bypass_nack_log != true) {
-        if (event == I2C_EVENT_NACK) {
-            ESP_LOGD(TAG, "I2C transaction unexpected nack detected");
-        }
-    }
 }

 //////////////////////////////////////I2C operation functions////////////////////////////////////////////
@@ -1041,9 +1033,6 @@ esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *bus_config, i2c_mast
     i2c_master->base->sda_num = bus_config->sda_io_num;
     i2c_master->base->pull_up_enable = bus_config->flags.enable_internal_pullup;

-    if (i2c_master->base->pull_up_enable == false) {
-        ESP_LOGW(TAG, "Please check pull-up resistances whether be connected properly. Otherwise unexpected behavior would happen. For more detailed information, please read docs");
-    }
     ESP_GOTO_ON_ERROR(i2c_param_master_config(i2c_master->base, bus_config), err, TAG, "i2c configure parameter failed");

     if (!i2c_master->base->is_lp_i2c) {
diff --git a/components/ulp/ulp_riscv/ulp_riscv_i2c.c b/components/ulp/ulp_riscv/ulp_riscv_i2c.c
index 12881ae57a..1c808070d4 100644
--- a/components/ulp/ulp_riscv/ulp_riscv_i2c.c
+++ b/components/ulp/ulp_riscv/ulp_riscv_i2c.c
@@ -389,12 +389,6 @@ esp_err_t ulp_riscv_i2c_master_read_from_device(uint8_t *data_rd, size_t size)

     portEXIT_CRITICAL(&rtc_i2c_lock);

-    if (ret != ESP_OK) {
-        ESP_LOGE(RTCI2C_TAG, "ulp_riscv_i2c: Read Failed!");
-        ESP_LOGE(RTCI2C_TAG, "ulp_riscv_i2c: RTC I2C Interrupt Raw Reg 0x%"PRIx32"", status);
-        ESP_LOGE(RTCI2C_TAG, "ulp_riscv_i2c: RTC I2C Status Reg 0x%"PRIx32"", READ_PERI_REG(RTC_I2C_STATUS_REG));
-    }
-
     /* Clear the RTC I2C transmission bits */
     CLEAR_PERI_REG_MASK(SENS_SAR_I2C_CTRL_REG, SENS_SAR_I2C_START_FORCE);
     CLEAR_PERI_REG_MASK(SENS_SAR_I2C_CTRL_REG, SENS_SAR_I2C_START);
@@ -472,13 +466,6 @@ esp_err_t ulp_riscv_i2c_master_write_to_device(const uint8_t *data_wr, size_t si

     portEXIT_CRITICAL(&rtc_i2c_lock);

-    /* In case of error, print the status after critical section */
-    if (ret != ESP_OK) {
-        ESP_LOGE(RTCI2C_TAG, "ulp_riscv_i2c: Write Failed!");
-        ESP_LOGE(RTCI2C_TAG, "ulp_riscv_i2c: RTC I2C Interrupt Raw Reg 0x%"PRIx32"", status);
-        ESP_LOGE(RTCI2C_TAG, "ulp_riscv_i2c: RTC I2C Status Reg 0x%"PRIx32"", READ_PERI_REG(RTC_I2C_STATUS_REG));
-    }
-
     /* Clear the RTC I2C transmission bits */
     CLEAR_PERI_REG_MASK(SENS_SAR_I2C_CTRL_REG, SENS_SAR_I2C_START_FORCE);
     CLEAR_PERI_REG_MASK(SENS_SAR_I2C_CTRL_REG, SENS_SAR_I2C_START);

```

### Comment 30: vidurp

- **Created:** 2026-04-30T02:06:47Z
- **Updated:** 2026-04-30T02:06:47Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-4349194361

i see it with DF-robot C4001 mmW radar as well, i see the data is returned but get this error . Seen with ESP-IDF 5.4.1 on ( Ubuntu 24.04  on WSL2 ) and WROOM32

### Comment 31: HKPhysicist

- **Created:** 2026-05-10T20:01:03Z
- **Updated:** 2026-05-10T20:01:03Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-4416211944

It seems that I also suffer from this error with an Aosong AHT25 sensor.

Any body can show me a quick fix?  :(

### Comment 32: eriksl

- **Created:** 2026-08-10T13:25:03Z
- **Updated:** 2026-08-10T13:25:03Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-5240901906

> It seems that I also suffer from this error with an Aosong AHT25 sensor.
> Any body can show me a quick fix? :(

The AOSONG devices are notoriously fragile in their I2C implementation. In the am232x series this was really bad, the AHT10 was better but still really fragile. Aosong recommended to place them on their own I2C bus (no other devices connected). My experience with the newer ones (AHT20 etc) is the compliance has improved but still isn't great. Absolutely not comparable to for instance te TMP and LM temperature sensors which are rock solid.

Sadly the only sensible (cheap) alternative (Si7021/HTU21) breaks easily (stops reporting valid humidity or temperature values). The Sensirion SHT series seems to be more robust, but I am not completely sure yet.



### Comment 33: HKPhysicist

- **Created:** 2026-08-10T14:26:07Z
- **Updated:** 2026-08-10T14:26:07Z
- **URL:** https://github.com/espressif/esp-idf/issues/14030#issuecomment-5241614820

> Aosong recommended to place them on their own I2C bus (no other devices connected).

Thank you for your opinion!
