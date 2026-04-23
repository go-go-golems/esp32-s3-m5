## SmartConfig

[\[中文\]](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-reference/network/esp_smartconfig.html)

## Introduction

The SmartConfig <sup>TM</sup> is a provisioning technology developed by TI to connect a new Wi-Fi device to a Wi-Fi network. It uses a mobile application to broadcast the network credentials from a smartphone, or a tablet, to an un-provisioned Wi-Fi device.

The advantage of this technology is that the device does not need to directly know SSID or password of an Access Point (AP). This information is provided using the smartphone. This is particularly important to headless device and systems, due to their lack of a user interface.

Currently, ESP32 support three types of SmartConfig: Airkiss, ESPTouch, and ESPTouch v2. ESPTouch v2 has been supported since SmartConfig v3.0 (the version of SmartConfig can be get from ), and it employs a completely different algorithm compared to ESPTouch, resulting in faster setup times. Additionally, ESPTouch v2 introduces AES encryption and custom data fields.

Starting from SmartConfig v3.0.2, ESPTouch v2 introduces support for random IV in AES encryption. On the application side, when the option for random IV is disabled, the default IV is set to 0, maintaining consistency with previous versions. When the random IV option is enabled, the IV will be a random value. It is important to note that when AES encryption is enabled with a random IV, the provision time will be extended due to the need of transmitting the IV to the provisioning device. On the provisioning device side, the device will identify whether the random IV for AES is enabled based on the flag in the provisioning packet.

If you are looking for other options to provision your ESP32 devices, check [Provisioning API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/index.html).

## Application Example

Connect ESP32 to the target AP using SmartConfig: [wifi/smart\_config](https://github.com/espressif/esp-idf/tree/47faecc3/examples/wifi/smart_config).

## API Reference

### Header File

- [components/esp\_wifi/include/esp\_smartconfig.h](https://github.com/espressif/esp-idf/blob/47faecc3/components/esp_wifi/include/esp_smartconfig.h)
- This header file can be included with:
	> ```c
	> #include "esp_smartconfig.h"
	> ```
- This header file is a part of the API provided by the `esp_wifi` component. To declare that your component depends on `esp_wifi`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES esp_wifi
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES esp_wifi
	> ```

### Functions

const char \*esp\_smartconfig\_get\_version(void) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv427esp_smartconfig_get_versionv "Permalink to this definition")  

Get the version of SmartConfig.

Returns:

- SmartConfig version const char.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_smartconfig\_start(const \*config) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv421esp_smartconfig_startPK26smartconfig_start_config_t "Permalink to this definition")  

Start SmartConfig, config ESP device to connect AP. You need to broadcast information by phone APP. Device sniffer special packets from the air that containing SSID and password of target AP.

**Attention**

1\. This API can be called in station or softAP-station mode.

**Attention**

2\. Can not call esp\_smartconfig\_start twice before it finish, please call esp\_smartconfig\_stop first.

Parameters:

**config** -- pointer to smartconfig start configure structure

Returns:

- ESP\_OK: succeed
- others: fail

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_smartconfig\_stop(void) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv420esp_smartconfig_stopv "Permalink to this definition")  

Stop SmartConfig, free the buffer taken by esp\_smartconfig\_start.

**Attention**

Whether connect to AP succeed or not, this API should be called to free memory taken by smartconfig\_start.

Returns:

- ESP\_OK: succeed
- others: fail

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_esptouch\_set\_timeout(uint8\_t time\_s) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv424esp_esptouch_set_timeout7uint8_t "Permalink to this definition")  

Set timeout of SmartConfig process.

**Attention**

Timing starts from SC\_STATUS\_FIND\_CHANNEL status. SmartConfig will restart if timeout.

Parameters:

**time\_s** -- range 15s~255s, offset:45s.

Returns:

- ESP\_OK: succeed
- others: fail

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_smartconfig\_set\_type( type) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv424esp_smartconfig_set_type18smartconfig_type_t "Permalink to this definition")  

Set protocol type of SmartConfig.

**Attention**

If users need to set the SmartConfig type, please set it before calling esp\_smartconfig\_start.

Parameters:

**type** -- Choose from the smartconfig\_type\_t.

Returns:

- ESP\_OK: succeed
- others: fail

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_smartconfig\_fast\_mode(bool enable) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv425esp_smartconfig_fast_modeb "Permalink to this definition")  

Set mode of SmartConfig. default normal mode.

**Attention**

1\. Please call it before API esp\_smartconfig\_start.

**Attention**

2\. Fast mode have corresponding APP(phone).

**Attention**

3\. Two mode is compatible.

Parameters:

**enable** -- false-disable(default); true-enable;

Returns:

- ESP\_OK: succeed
- others: fail

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_smartconfig\_get\_rvd\_data(uint8\_t \*rvd\_data, uint8\_t len) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv428esp_smartconfig_get_rvd_dataP7uint8_t7uint8_t "Permalink to this definition")  

Get reserved data of ESPTouch v2.

Parameters:

- **rvd\_data** -- reserved data
- **len** -- length of reserved data

Returns:

- ESP\_OK: succeed
- others: fail

### Structures

struct smartconfig\_event\_got\_ssid\_pswd\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv433smartconfig_event_got_ssid_pswd_t "Permalink to this definition")  

Argument structure for SC\_EVENT\_GOT\_SSID\_PSWD event

Public Members

uint8\_t ssid\[32\] [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv4N33smartconfig_event_got_ssid_pswd_t4ssidE "Permalink to this definition")  

SSID of the AP. Null terminated string.

uint8\_t password\[64\] [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv4N33smartconfig_event_got_ssid_pswd_t8passwordE "Permalink to this definition")  

Password of the AP. Null terminated string.

bool bssid\_set [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv4N33smartconfig_event_got_ssid_pswd_t9bssid_setE "Permalink to this definition")  

whether set MAC address of target AP or not.

uint8\_t bssid\[6\] [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv4N33smartconfig_event_got_ssid_pswd_t5bssidE "Permalink to this definition")  

MAC address of target AP.

type [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv4N33smartconfig_event_got_ssid_pswd_t4typeE "Permalink to this definition")  

Type of smartconfig(ESPTouch or AirKiss).

uint8\_t token [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv4N33smartconfig_event_got_ssid_pswd_t5tokenE "Permalink to this definition")  

Token from cellphone which is used to send ACK to cellphone.

uint8\_t cellphone\_ip\[4\] [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv4N33smartconfig_event_got_ssid_pswd_t12cellphone_ipE "Permalink to this definition")  

IP address of cellphone.

struct smartconfig\_start\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv426smartconfig_start_config_t "Permalink to this definition")  

Configure structure for esp\_smartconfig\_start

Public Members

bool enable\_log [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv4N26smartconfig_start_config_t10enable_logE "Permalink to this definition")  

Enable smartconfig logs.

bool esp\_touch\_v2\_enable\_crypt [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv4N26smartconfig_start_config_t25esp_touch_v2_enable_cryptE "Permalink to this definition")  

Enable ESPTouch v2 crypt.

char \*esp\_touch\_v2\_key [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv4N26smartconfig_start_config_t16esp_touch_v2_keyE "Permalink to this definition")  

ESPTouch v2 crypt key, len should be 16.

### Macros

SMARTCONFIG\_START\_CONFIG\_DEFAULT() [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#c.SMARTCONFIG_START_CONFIG_DEFAULT "Permalink to this definition")  

### Enumerations

enum smartconfig\_type\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv418smartconfig_type_t "Permalink to this definition")  

*Values:*

enumerator SC\_TYPE\_ESPTOUCH [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv4N18smartconfig_type_t16SC_TYPE_ESPTOUCHE "Permalink to this definition")  

protocol: ESPTouch

enumerator SC\_TYPE\_AIRKISS [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv4N18smartconfig_type_t15SC_TYPE_AIRKISSE "Permalink to this definition")  

protocol: AirKiss

enumerator SC\_TYPE\_ESPTOUCH\_AIRKISS [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv4N18smartconfig_type_t24SC_TYPE_ESPTOUCH_AIRKISSE "Permalink to this definition")  

protocol: ESPTouch and AirKiss

enumerator SC\_TYPE\_ESPTOUCH\_V2 [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv4N18smartconfig_type_t19SC_TYPE_ESPTOUCH_V2E "Permalink to this definition")  

protocol: ESPTouch v2

enum smartconfig\_event\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv419smartconfig_event_t "Permalink to this definition")  

Smartconfig event declarations

*Values:*

enumerator SC\_EVENT\_SCAN\_DONE [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv4N19smartconfig_event_t18SC_EVENT_SCAN_DONEE "Permalink to this definition")  

Station smartconfig has finished to scan for APs

enumerator SC\_EVENT\_FOUND\_CHANNEL [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv4N19smartconfig_event_t22SC_EVENT_FOUND_CHANNELE "Permalink to this definition")  

Station smartconfig has found the channel of the target AP

enumerator SC\_EVENT\_GOT\_SSID\_PSWD [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv4N19smartconfig_event_t22SC_EVENT_GOT_SSID_PSWDE "Permalink to this definition")  

Station smartconfig got the SSID and password

enumerator SC\_EVENT\_SEND\_ACK\_DONE [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html#_CPPv4N19smartconfig_event_t22SC_EVENT_SEND_ACK_DONEE "Permalink to this definition")  

Station smartconfig has sent ACK to cellphone