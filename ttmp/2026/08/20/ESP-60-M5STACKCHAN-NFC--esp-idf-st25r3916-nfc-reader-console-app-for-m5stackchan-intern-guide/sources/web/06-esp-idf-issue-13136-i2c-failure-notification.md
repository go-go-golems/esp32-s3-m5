# i2c_master: Support I2C transaction failure notification because the new ESP32 I2C driver always returns ESP_OK (IDFGH-12075)

- **Canonical URL:** https://github.com/espressif/esp-idf/issues/13136
- **Repository:** `espressif/esp-idf`
- **Issue:** #13136
- **State at retrieval:** closed
- **Created:** 2024-02-08T02:01:35Z
- **Updated:** 2024-09-19T03:56:36Z
- **Retrieved:** 2026-08-21
- **Labels:** Type: Feature Request, Status: Done, Resolution: Done

> [!note] Source snapshot
> This file preserves an external issue discussion as research evidence. Claims in comments are reports from participants, not automatically verified facts. Consult the canonical issue for later updates.

## Issue body

### Is your feature request related to a problem?

Originally posted here: [Why does i2c_master_transmit_receive return ESP_OK when the device is unplugged?](https://esp32.com/viewtopic.php?f=13&t=38283)

I can successfully read from an I2C device (its an RTC clock) and it works. However, if the device is disconnected from the bus, the `i2c_master_transmit_receive()` function always returns `ESP_OK`.

Digging through the code, it appears that synchronous requests will set (or rather, pass-through) any of these in `i2c_master_bus_t->status`:
- I2C_STATUS_ACK_ERROR
- I2C_STATUS_TIMEOUT
- I2C_STATUS_DONE

as shown in [esp_driver_i2c/i2c_master.c](https://github.com/espressif/esp-idf/blob/master/components/esp_driver_i2c/i2c_master.c#L439):
```c
    if (i2c_master->status != I2C_STATUS_ACK_ERROR && i2c_master->status != I2C_STATUS_TIMEOUT) {
        atomic_store(&i2c_master->status, I2C_STATUS_DONE);
    }
```

### Describe the solution you'd like.

Provide an API accessor, something like this:

```c
i2c_master_status_t i2c_master_get_previous_io_status(i2c_master_bus_handle_t i2c_master)
{
  return i2c_master->status;
}
```

and expose [these enums](https://github.com/espressif/esp-idf/blob/master/components/esp_driver_i2c/include/driver/i2c_types.h#L26) publically (if they are not already):

```c
 */
typedef enum {
    I2C_STATUS_READ,      /*!< read status for current master command */
    I2C_STATUS_WRITE,     /*!< write status for current master command */
    I2C_STATUS_START,     /*!< Start status for current master command */
    I2C_STATUS_STOP,      /*!< stop status for current master command */
    I2C_STATUS_IDLE,      /*!< idle status for current master command */
    I2C_STATUS_ACK_ERROR, /*!< ack error status for current master command */
    I2C_STATUS_DONE,      /*!< I2C command done */
    I2C_STATUS_TIMEOUT,   /*!< I2C bus status error, and operation timeout */
} i2c_master_status_t;
```

### Describe alternatives you've considered.

~~Could try using the async code and work with the `i2c_master_event_t` enum values in the [i2c_master event callback](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/api-reference/peripherals/i2c.html#_CPPv435i2c_master_register_event_callbacks23i2c_master_dev_handle_tPK28i2c_master_event_callbacks_tPv). According to the [forum response](https://esp32.com/viewtopic.php?f=13&t=38283), that should work, but I've not tried it yet.  However, it would be nice to trivially query the status of the previous i2c request for sync requests without setting up everything needed for async.~~

**Update:** I tried the async code using [this simple test project](https://github.com/KJ7LNW/esp32-i2c-test/) but it provides no mechanism to get a bus read failure status of any kind.  There appears to be no alternative: neither sync nor async can provide an indication of i2c request failure.

### Additional context.

_No response_

## Comments

### Comment 1: KJ7LNW

- **Created:** 2024-02-08T02:03:12Z
- **Updated:** 2024-02-08T02:03:20Z
- **URL:** https://github.com/espressif/esp-idf/issues/13136#issuecomment-1933249814

(I'm adding mention of #13134, but I think that's a slightly different issue.)

### Comment 2: mythbuster5

- **Created:** 2024-02-08T04:20:39Z
- **Updated:** 2024-02-08T04:20:39Z
- **URL:** https://github.com/espressif/esp-idf/issues/13136#issuecomment-1933340595

I know there is no nack checking... Fine, I will add one...

### Comment 3: KJ7LNW

- **Created:** 2024-02-18T02:10:30Z
- **Updated:** 2024-02-18T02:10:30Z
- **URL:** https://github.com/espressif/esp-idf/issues/13136#issuecomment-1950783260

After some testing, the callback  NACK checking does not work in async, either.  It always returns `I2C_EVENT_DONE` even if you unplug the device.  Here is a simple esp32 test project that can toggle either sync or async operation in case it helps with testing new NACK support:
- https://github.com/KJ7LNW/esp32-i2c-test/

IMHO, error handling is a critical consideration in microcontroller development: Maybe this should be escalated from Feature to BUG status.

### Comment 4: KJ7LNW

- **Created:** 2024-03-11T01:41:39Z
- **Updated:** 2024-03-11T01:41:39Z
- **URL:** https://github.com/espressif/esp-idf/issues/13136#issuecomment-1987481043

@mythbuster5, I was going to write a PR for this issue, but it looks like `i2c_master->status` is always `I2C_EVENT_DONE` after a sync transaction.    I tried commenting the `atomic_store` line just to see what ends up in `i2c_master->status`, even if it fails (of course that breaks the FSM, this is just a test); it contains `I2C_STATUS_STOP` but I was hoping for something like `I2C_STATUS_ACK_ERROR`:

https://github.com/espressif/esp-idf/blob/60a2bf6a68867fd468a250966230a067f8d97ea2/components/esp_driver_i2c/i2c_master.c#L439-L441

However, `i2c_master_probe()` is able to detect a `I2C_STATUS_ACK_ERROR` so I'm not sure what needs to happen so that `s_i2c_send_commands()` can pass an error back up to functions like `i2c_master_transmit()` so the caller can know that the IO failed.  Here is `i2c_master_probe()`'s detection of a missing device:

https://github.com/espressif/esp-idf/blob/60a2bf6a68867fd468a250966230a067f8d97ea2/components/esp_driver_i2c/i2c_master.c#L1064-L1070

Below is the diff we were working on, it's just a `i2c_master->status` accessor.  Do you know what needs to happen to handle missing devices and failed requests?

```diff
diff --git a/components/esp_driver_i2c/i2c_master.c b/components/esp_driver_i2c/i2c_master.c
index 175fb15703..5a50f048b4 100644
--- a/components/esp_driver_i2c/i2c_master.c
+++ b/components/esp_driver_i2c/i2c_master.c
@@ -967,6 +967,11 @@ esp_err_t i2c_master_bus_reset(i2c_master_bus_handle_t bus_handle)
     return ESP_OK;
 }

+i2c_master_status_t i2c_master_bus_status(i2c_master_bus_handle_t i2c_master)
+{
+    return i2c_master->status;
+}
+
 esp_err_t i2c_master_transmit(i2c_master_dev_handle_t i2c_dev, const uint8_t *write_buffer, size_t write_size, int xfer_timeout_ms)
 {
     ESP_RETURN_ON_FALSE(i2c_dev != NULL, ESP_ERR_INVALID_ARG, TAG, "i2c handle not initialized");
diff --git a/components/esp_driver_i2c/include/driver/i2c_master.h b/components/esp_driver_i2c/include/driver/i2c_master.h
index 228f64fcc9..b8b9bb0abf 100644
--- a/components/esp_driver_i2c/include/driver/i2c_master.h
+++ b/components/esp_driver_i2c/include/driver/i2c_master.h
@@ -196,6 +196,18 @@ esp_err_t i2c_master_register_event_callbacks(i2c_master_dev_handle_t i2c_dev, c
  */
 esp_err_t i2c_master_bus_reset(i2c_master_bus_handle_t bus_handle);

+/**
+ * @brief Return the status of the last I2C transaction.
+ *
+ * @param bus_handle I2C bus handle.
+ * @return
+ *      - I2C_STATUS_DONE: I2C command done
+ *      - I2C_STATUS_TIMEOUT: I2C bus status error, and operation timeout
+ *      - I2C_STATUS_ACK_ERROR: ack error status for current master command
+ *      - Otherwise: transaction in progress (see `i2c_master_status_t`)
+ */
+i2c_master_status_t i2c_master_bus_status(i2c_master_bus_handle_t i2c_master);
+
 /**
  * @brief Wait for all pending I2C transactions done
  *
```

### Comment 5: AxelLin

- **Created:** 2024-09-19T03:08:25Z
- **Updated:** 2024-09-19T03:08:25Z
- **URL:** https://github.com/espressif/esp-idf/issues/13136#issuecomment-2359886565

@mythbuster5
It's marked as Resolution: Done since Jun 24, but it's not yet closed.
How is the status of this issue now?

### Comment 6: mythbuster5

- **Created:** 2024-09-19T03:56:36Z
- **Updated:** 2024-09-19T03:56:36Z
- **URL:** https://github.com/espressif/esp-idf/issues/13136#issuecomment-2359924276

It should be done already, I forgot to close it.
