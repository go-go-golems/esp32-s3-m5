# ESP_ERR_INVALID_STATE return on write-read-cycle using i2c_master_execute_defined_operations, but it still works somehow. (IDFGH-16424)

- **Canonical URL:** https://github.com/espressif/esp-idf/issues/17556
- **Repository:** `espressif/esp-idf`
- **Issue:** #17556
- **State at retrieval:** closed
- **Created:** 2025-09-09T14:58:28Z
- **Updated:** 2025-09-19T03:05:11Z
- **Retrieved:** 2026-08-21
- **Labels:** Type: Bug, Status: Done, Resolution: Won't Do

> [!note] Source snapshot
> This file preserves an external issue discussion as research evidence. Claims in comments are reports from participants, not automatically verified facts. Consult the canonical issue for later updates.

## Issue body

### Answers checklist.

- [x] I have read the documentation [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/) and the issue is not addressed there.
- [x] I have updated my IDF branch (master or release) to the latest version and checked that the issue is present there.
- [x] I have searched the issue tracker for a similar issue and not found a similar issue.

### IDF version.

v5.5

### Espressif SoC revision.

ESP32-S3

### Operating System used.

Linux

### How did you build your project?

Command line with idf.py

### If you are using Windows, please specify command line type.

None

### Development Kit.

none specifically

### Power Supply used.

USB

### What is the expected behavior?

I expected for i2c_master_execute_defined_operations to return without error. Protocol analyser shows all commands have been processed and sent.

### What is the actual behavior?

Instead it returns an error.

The error is generated in i2c_master.c at line 652:

```
if (atomic_load(&i2c_master->status) != I2C_STATUS_DONE) {
            ret = ESP_ERR_INVALID_STATE;
        }
```

### Steps to reproduce.

```
static bool ll_main_send_receive(const module_info_t *info, module_data_t *data, unsigned int address_in, unsigned int send_buffer_length, const uint8_t *send_buffer,
		unsigned int receive_buffer_size, uint8_t *receive_buffer, bool verbose)
{
	esp_err_t rv;
	i2c_operation_job_t i2c_operations[7];
	unsigned int current;
	unsigned int cooked_send_buffer_length = send_buffer_length + 1;
	uint8_t cooked_send_buffer[cooked_send_buffer_length];
	uint8_t read_address;

	assert(info->available);
	assert(data->device_handle);

	cooked_send_buffer[0] = (uint8_t)((address_in << i2c_address_shift) | i2c_write_flag);
	read_address = (uint8_t)((address_in << i2c_address_shift) | i2c_read_flag);

	memcpy(&cooked_send_buffer[1], send_buffer, send_buffer_length);

	current = 0;

	i2c_operations[current++].command = I2C_MASTER_CMD_START;

	i2c_operations[current].command = I2C_MASTER_CMD_WRITE;
	i2c_operations[current].write.ack_check = true;
	i2c_operations[current].write.data = cooked_send_buffer;
	i2c_operations[current].write.total_bytes = cooked_send_buffer_length;
	current++;

	i2c_operations[current++].command = I2C_MASTER_CMD_START;

	i2c_operations[current].command = I2C_MASTER_CMD_WRITE;
	i2c_operations[current].write.ack_check = true;
	i2c_operations[current].write.data = &read_address;
	i2c_operations[current].write.total_bytes = 1;
	current++;

	if(receive_buffer_size > 0)
	{
		i2c_operations[current].command = I2C_MASTER_CMD_READ;
		i2c_operations[current].read.ack_value = I2C_ACK_VAL;
		i2c_operations[current].read.data = receive_buffer;
		i2c_operations[current].read.total_bytes = receive_buffer_size - 1;
		current++;
	}

	i2c_operations[current].command = I2C_MASTER_CMD_READ;
	i2c_operations[current].read.ack_value = I2C_NACK_VAL;
	i2c_operations[current].read.data = &receive_buffer[receive_buffer_size - 1];
	i2c_operations[current].read.total_bytes = 1;
	current++;

	i2c_operations[current++].command = I2C_MASTER_CMD_STOP;

	rv = i2c_master_execute_defined_operations(data->device_handle, i2c_operations, current, 500);

	if(verbose && (rv != ESP_OK))
		util_warn_on_esp_err("ll main send receive: i2c_master_defined_operations", rv);

	return(rv == ESP_OK);
}
```

### Debug Logs.

```plain

```

### Diagnostic report archive.

```
 19 2025/09/09 16:45:13  s_i2c_synchronous_transaction(946): I2C transaction failed
 20 2025/09/09 16:45:13 warning: ll main send receive: i2c_master_defined_operations 1 (ESP_ERR_INVALID_STATE) [0x103]
```

### More Information.

As to this function being documented very poorly I had to reverse engineer from the IDF driver code. I have located the code where a write-read cycle (repeated start) is handled and copied the process. It's not 1:1 though, because the I2C master driver code is using a static per device address, where I am using a dynamic address. So that explains the extra WRITE command after the write cycle. Maybe that's the problem. But how to solve then? I can't leave out the second START nor the extra WRITE, doesn't work.

## Comments

### Comment 1: mythbuster5

- **Created:** 2025-09-10T03:31:04Z
- **Updated:** 2025-09-10T03:31:04Z
- **URL:** https://github.com/espressif/esp-idf/issues/17556#issuecomment-3273163880

I cannot reproduce it, I run this without a slave and disable nack check, I got wave like this with your code.

<img width="1750" height="279" alt="Image" src="https://github.com/user-attachments/assets/0105b302-2e27-4b3c-8f79-54cc40568dd9" />

### Comment 2: eriksl

- **Created:** 2025-09-10T11:30:19Z
- **Updated:** 2025-09-10T11:30:19Z
- **URL:** https://github.com/espressif/esp-idf/issues/17556#issuecomment-3274540118

Not really the same situation, is it?

I am seeing this on valid transfers. For testing I am mostly using an I2C bus multiplexer that listens to address 0x70. It can accept one byte and it can supply one byte, no registers, just a value. If you send more bytes, it just ignores the extra bytes. If you receive more bytes, it will supply 0xff. It will not send NAK in any of these cases.

My test case is to
- send 1 byte, receive 1 byte
- send 2 bytes, receive 2 bytes

The logic analyser says the transaction is OK (and it actually IS exactly as I exepect), but still I get an error from this function. I also suspect the reset_fsm function is called, because there is a large gap between both sends.

situation 1: <start><address+write bit><data 1><ack><start><address+read bit><data 1><nak><stop>
situation 1: <start><address+write bit><data 1><ack><data 2><ack><start><address+read bit><data 1><ack><data 2><nak><stop>

No strange things happening there. It could be stop condition is generated by the reset_fsm function. I will check that.

### Comment 3: eriksl

- **Created:** 2025-09-12T08:30:02Z
- **Updated:** 2025-09-12T08:30:02Z
- **URL:** https://github.com/espressif/esp-idf/issues/17556#issuecomment-3284292419

I am now testing within the IDF I2C driver code. In file i2_master.c at line 652, after s_i2c_send_commands is called, i2c_master_status is checked for "I2C_STATUS_DONE", but it returns "I2C_STATUS_ACK_ERROR. Which is really strange because the transaction is completely perfectly finished (says my logic analyser).
<img width="2692" height="570" alt="Image" src="https://github.com/user-attachments/assets/52da415c-e308-4a4e-a581-4a8521773165" />
This is for every transaction to this device (bus mux @0x70). I will check with other devices, but I don't it will matter.

Two times the address is ACKed, the last read cycle is NAKed by our side to signal end-of-transfer, so no reason for error.

I will also dig further into the driver code, but if, in the meantime, you can help me in the right direction, it would certainly be appreciated ;-)

### Comment 4: eriksl

- **Created:** 2025-09-13T12:48:04Z
- **Updated:** 2025-09-13T12:48:04Z
- **URL:** https://github.com/espressif/esp-idf/issues/17556#issuecomment-3288310824

Oh my dear, this is humiliating. For me. I've been implementing a unifying layer for all three I2C modules in de ESP32-S3, where the ULP I2C peripheral can be used (as far as possible) like one of the "main" I2C peripherals.

This means that all three I2C buses are probed for devices. It now turns out I've have had the protocol analyser on the wrong bus!

So the error from the IDF driver code was completely appropriate. I've connected everything correctly now and the issue is gone.

This has been very interesting for me, learned a lot of the I2C driving code. Hope you didn't spend any time it!

Apologies!
