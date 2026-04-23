## Wi-Fi Easy ConnectTM (DPP)

[\[中文\]](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-reference/network/esp_dpp.html)

Wi-Fi Easy Connect <sup>TM</sup>, also known as Device Provisioning Protocol (DPP) or Easy Connect, is a provisioning protocol certified by Wi-Fi Alliance. It is a secure and standardized provisioning protocol for configuration of Wi-Fi Devices. With Easy Connect, adding a new device to a network is as simple as scanning a QR Code. This reduces complexity and enhances user experience while onboarding devices without UI like Smart Home and IoT products. Unlike old protocols like Wi-Fi Protected Setup (WPS), Wi-Fi Easy Connect incorporates strong encryption through public key cryptography to ensure networks remain secure as new devices are added.

Easy Connect brings many benefits in the user experience:

> - Simple and intuitive to use; no lengthy instructions to follow for new device setup
> - No need to remember and enter passwords into the device being provisioned
> - Works with electronic or printed QR codes, or human-readable strings
> - Supports both WPA2 and WPA3 networks

Please refer to Wi-Fi Alliance's official page on [Easy Connect](https://www.wi-fi.org/discover-wi-fi/wi-fi-easy-connect) for more information.

ESP32 supports Enrollee mode of Easy Connect with QR Code as the provisioning method. A display is required to display this QR Code. Users can scan this QR Code using their capable device and provision the ESP32 to their Wi-Fi network. The provisioning device needs to be connected to the AP which need not support Wi-Fi Easy Connect <sup>TM</sup>.

Easy Connect is still an evolving protocol. Of known platforms that support the QR Code method are some Android smartphones with Android 10 or higher. To use Easy Connect, no additional App needs to be installed on the supported smartphone.

## Application Examples

- [wifi/wifi\_easy\_connect/dpp-enrollee](https://github.com/espressif/esp-idf/tree/47faecc3/examples/wifi/wifi_easy_connect/dpp-enrollee) demonstrates how to configure ESP32 as an enrollee using DPP to securely onboard ESP devices to a network with the help of a QR code and an Android 10+ device.

## API Reference

### Header File

- [components/wpa\_supplicant/esp\_supplicant/include/esp\_dpp.h](https://github.com/espressif/esp-idf/blob/47faecc3/components/wpa_supplicant/esp_supplicant/include/esp_dpp.h)
- This header file can be included with:
	> ```c
	> #include "esp_dpp.h"
	> ```
- This header file is a part of the API provided by the `wpa_supplicant` component. To declare that your component depends on `wpa_supplicant`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES wpa_supplicant
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES wpa_supplicant
	> ```

### Functions

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_supp\_dpp\_init(void) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_dpp.html#_CPPv417esp_supp_dpp_initv "Permalink to this definition")  

Initialize DPP Supplicant.

```cpp
Starts DPP Supplicant and initializes related Data Structures.
```

return
- ESP\_OK: Success
- ESP\_FAIL: Failure

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_supp\_dpp\_deinit(void) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_dpp.html#_CPPv419esp_supp_dpp_deinitv "Permalink to this definition")  

De-initialize DPP Supplicant.

```cpp
Frees memory from DPP Supplicant Data Structures.
```

Returns:

- ESP\_OK: Success

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_supp\_dpp\_bootstrap\_gen(const char \*chan\_list, type, const char \*key, const char \*info) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_dpp.html#_CPPv426esp_supp_dpp_bootstrap_genPKc24esp_supp_dpp_bootstrap_tPKcPKc "Permalink to this definition")  

Generates Bootstrap Information as an Enrollee.

```cpp
Generates Out Of Band Bootstrap information as an Enrollee which can be
   used by a DPP Configurator to provision the Enrollee.
```

Parameters:

- **chan\_list** -- List of channels device will be available on for listening
- **type** -- Bootstrap method type, only QR Code method is supported for now.
- **key** -- (Optional) 32 byte Raw Private Key for generating a Bootstrapping Public Key
- **info** -- (Optional) Ancillary Device Information like Serial Number

Returns:

- ESP\_OK: Success
- ESP\_ERR\_DPP\_INVALID\_LIST: Channel list not valid
- ESP\_FAIL: Failure

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_supp\_dpp\_start\_listen(void) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_dpp.html#_CPPv425esp_supp_dpp_start_listenv "Permalink to this definition")  

Start listening on Channels provided during esp\_supp\_dpp\_bootstrap\_gen.

```cpp
Listens on every Channel from Channel List for a pre-defined wait time.
```

Returns:

- ESP\_OK: Success
- ESP\_FAIL: Generic Failure
- ESP\_ERR\_INVALID\_STATE: ROC attempted before WiFi is started
- ESP\_ERR\_NO\_MEM: Memory allocation failed while posting ROC request

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_supp\_dpp\_stop\_listen(void) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_dpp.html#_CPPv424esp_supp_dpp_stop_listenv "Permalink to this definition")  

Stop listening on Channels.

```cpp
Stops listening on Channels and cancels ongoing listen operation.
```

Returns:

- ESP\_OK: Success
- ESP\_FAIL: Failure

### Macros

ESP\_DPP\_MAX\_CHAN\_COUNT [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_dpp.html#c.ESP_DPP_MAX_CHAN_COUNT "Permalink to this definition")  

ESP\_ERR\_DPP\_FAILURE [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_dpp.html#c.ESP_ERR_DPP_FAILURE "Permalink to this definition")  

Generic failure during DPP Operation

ESP\_ERR\_DPP\_TX\_FAILURE [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_dpp.html#c.ESP_ERR_DPP_TX_FAILURE "Permalink to this definition")  

DPP Frame Tx failed OR not Acked

ESP\_ERR\_DPP\_INVALID\_ATTR [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_dpp.html#c.ESP_ERR_DPP_INVALID_ATTR "Permalink to this definition")  

Encountered invalid DPP Attribute

ESP\_ERR\_DPP\_AUTH\_TIMEOUT [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_dpp.html#c.ESP_ERR_DPP_AUTH_TIMEOUT "Permalink to this definition")  

DPP Auth response was not received in time

ESP\_ERR\_DPP\_INVALID\_LIST [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_dpp.html#c.ESP_ERR_DPP_INVALID_LIST "Permalink to this definition")  

Channel list given in esp\_supp\_dpp\_bootstrap\_gen() is not valid or too big

ESP\_ERR\_DPP\_CONF\_TIMEOUT [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_dpp.html#c.ESP_ERR_DPP_CONF_TIMEOUT "Permalink to this definition")  

DPP Configuration was not received in time

### Type Definitions

typedef enum esp\_supp\_dpp\_bootstrap\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_dpp.html#_CPPv424esp_supp_dpp_bootstrap_t "Permalink to this definition")  

Types of Bootstrap Methods for DPP.

### Enumerations

enum dpp\_bootstrap\_type [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_dpp.html#_CPPv418dpp_bootstrap_type "Permalink to this definition")  

Types of Bootstrap Methods for DPP.

*Values:*

QR Code Method

enumerator DPP\_BOOTSTRAP\_PKEX [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_dpp.html#_CPPv4N18dpp_bootstrap_type18DPP_BOOTSTRAP_PKEXE "Permalink to this definition")  

Proof of Knowledge Method

enumerator DPP\_BOOTSTRAP\_NFC\_URI [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_dpp.html#_CPPv4N18dpp_bootstrap_type21DPP_BOOTSTRAP_NFC_URIE "Permalink to this definition")  

NFC URI record Method