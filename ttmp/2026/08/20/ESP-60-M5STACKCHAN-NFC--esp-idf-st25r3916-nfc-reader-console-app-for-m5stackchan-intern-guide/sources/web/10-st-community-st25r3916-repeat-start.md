# ST Community: ST25R3916 Repeat-Start Transaction Discussion

- **Canonical URL:** https://community.st.com/t5/st25-nfc-rfid-tags-and-readers/st25r3916comrepeatstart-does-not-work-for-the-st25r3916-driver/td-p/806540
- **Retrieved:** 2026-08-21

Hello,

I am working with stm32cubeIDE using some NDEF example for nucleo NFC08A1 extension board with st25r3916b rfid reader.

Debugging the demoCycle example I see that the code that should insert a repeat start on I2C communication does
nothing(which confirms what I have described in another post about the stm32ube example for it, not being able to read
chip id):

```c
#ifdef RFAL_USE_I2C
static void st25r3916comRepeatStart( void )
{
 st25r3916I2CRepeatStart();
 st25r3916I2CSlaveAddrRD( ST25R3916_I2C_ADDR );
}
#endif /* RFAL_USE_I2C */
```

it points some macros but that have nothing associated in rfal\_platform.h

```c
#define platformI2CRepeatStart() /*!< I2C Repeat Start */
#define platformI2CSlaveAddrWR(add) /*!< I2C Slave address for Write operation */
#define platformI2CSlaveAddrRD(add)
```

is it expected that the user implements those?

Thanks,

Mihai
