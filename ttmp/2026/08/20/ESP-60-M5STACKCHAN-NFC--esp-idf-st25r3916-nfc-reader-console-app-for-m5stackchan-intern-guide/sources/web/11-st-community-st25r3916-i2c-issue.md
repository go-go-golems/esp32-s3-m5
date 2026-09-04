# ST Community: ST25R3916 I2C Issue

- **Canonical URL:** https://community.st.com/st25-nfc-rfid-tags-and-readers-54/st25r3916-i2c-issue-141107
- **Retrieved:** 2026-08-21

Hi everyone,

I'm currently working on reading and writing the registers of the ST25R3916 chip, which is mounted on the NFC08A1
board. However, I'm running into some issues with the I2C read and write operations.

Below is the code I'm using to attempt reading from the chip:

```c
while (1)
 {
 uint8_t value = 0;
 HAL_StatusTypeDef status;
 status = HAL_I2C_Mem_Read(&hi2c1, ST25R_ADDR, 0x000C, I2C_MEMADD_SIZE_16BIT, &value, 1, 1000);

 switch(status){
 case HAL_OK:
 UART1_WriteString("HAL_OK I2C\r\n");
 break;

 case HAL_ERROR:
 UART1_WriteString("HAL_ERROR I2C\r\n");
 break;

 case HAL_BUSY:
 UART1_WriteString("HAL_BUSY I2C\r\n");
 break;

 case HAL_TIMEOUT:
 UART1_WriteString("HAL_TIMEOUT I2C\r\n");
 break;
 }
 }
```

For reference, "ST25R\_ADDR" = 0x 50.

When I flash the micro (NUCLEO-U575IQ), the code keeps printing HAL\_ERROR on the serial monitor, and I can't figure
out why.

From the ST25R3916 datasheet, I see that the I2C address is supposed to be 0x50, and I'm simply trying to read from the
register at address 0x000C (or other registers).

Can anyone help me figure out what might be going wrong with this I2C communication, or if there is any additional
configuration needed to successfully read or write to the ST25R3916 registers?

Thanks in advance!
