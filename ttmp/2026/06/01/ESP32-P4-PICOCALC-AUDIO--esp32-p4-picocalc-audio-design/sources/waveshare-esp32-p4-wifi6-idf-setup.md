---
Title: Source - waveshare-esp32-p4-wifi6-idf-setup
Ticket: ESP32-P4-PICOCALC-AUDIO
Status: active
Topics:
    - esp32-p4
    - picocalc
DocType: source
Intent: reference
Summary: "Downloaded reference material for ESP32-P4-PICOCALC-AUDIO"
---

This chapter includes the following sections, please read as needed:

- [Setting Up Development Environment](https://docs.waveshare.com/ESP32-P4-WIFI6-Touch-LCD-X/Development-Environment-Setup-IDF#esp-idf-setup)

## Setting up the Development Environment

### Install the ESP-IDF Development Environment

1. Download the installation manager from the [ESP-IDF Installation Manager](https://dl.espressif.com/dl/eim/) page. This is Espressif's latest cross-platform installer. The following steps demonstrate how to use its offline installation feature.
	Click the **Offline Installer** tab on the page, then select **Windows** as the operating system and **the ESP-IDF version you need** (the version shown in the screenshot is for reference only — choose the version that fits your actual needs).
	![Download EIM and offline package](https://docs.waveshare.com/assets/images/01-EIM-Offline-Installation-1-4bf0df8afe2b21c6be95828003f0b94d.webp)
	After confirming your selection, click the download button. The browser will automatically download two files: the **ESP-IDF Offline Package (.zst)** and the **ESP-IDF Installer (.exe)**.
	![Download EIM and offline package 2](https://docs.waveshare.com/assets/images/01-EIM-Offline-Installation-2-6209ce57a020782adf348ae124ff6f67.webp)
	Please wait for both files to finish downloading.
2. Once the download is complete, double-click to run the **ESP-IDF Installer (eim-gui-windows-x64.exe)**.
	The installer will automatically detect if the offline package exists in the same directory. Click **Install from archive**.
	![Auto-detect offline package](https://docs.waveshare.com/assets/images/01-EIM-Offline-Installation-4-e49c31342be5c78da1adfd7d9249c491.webp)
	Next, select the installation path. We recommend using the default path. If you need to customize it, ensure the path does not contain Chinese characters or spaces. Click **Start installation** to proceed.
	![Select installation path](https://docs.waveshare.com/assets/images/01-EIM-Offline-Installation-5-dc945b666c83940cf17180c98ed687d1.webp)
3. When you see the following screen, the ESP-IDF installation is successful.
	![Installation successful](https://docs.waveshare.com/assets/images/01-EIM-Offline-Installation-6-1df9ba6d5d954dcbf8ff0bc231f19b64.webp)
4. We recommend installing the drivers as well. Click **Finish installation**, then select **Install driver**.
	![Install drivers via ESP-IDF Installation Manager](https://docs.waveshare.com/assets/images/01-EIM-Offline-Installation-7-d6839851019130942567b2e0e51ed339.webp)

### Install Visual Studio Code and the ESP-IDF Extension

1. Download and install [Visual Studio Code](https://code.visualstudio.com/).
2. During installation, it is recommended to check **Add "Open with Code" action to Windows Explorer file context menu** to facilitate opening project folders quickly.
3. In VS Code, click the **Extensions** icon ![Extensions Icon](data:image/webp;base64,UklGRmwCAABXRUJQVlA4IGACAACQDwCdASohACEAAAAAJZwDOK8c4IVLzgNsBogH9P/nPWAfsB7AH7Felh+xXwS/sd+4fs3//+6X4oPpf8t/IDwAN5n9QHoAapf+OXiV8TXrAH8r/3fn0/t35Af4D2p/LX+q/mHsZf5D8reMR/Tc7NJa7BmtoefSUDTIRXP24s6CSp4wehu4AP7//1fNCFT/kO3+oALgIvZh/Qe48AXTGbuqLluKdXKPacd9/pZl6Sty8X2HXk3b0py+5NjxmXyV02tHyuIUWdHyy/pRUTpke+tXvBFfoJ/53lx1/6fMiThKVy/z/I7PoY7XL3PuvFvQ1fQLrRkDgRf0rO++1UerDL4amvR7a4PBiSTax/TPRi7hzdRkf70rKea+iBPxTwMQJT/4KSrXf45KEfj3Uxk+qKOhJBzmIBEHZpkVHNc6ACZKxUrf/iyW5rkkYW3mMThmug+7MxU2eo4+srR27Polr235vgXoE6afM6dYRXiGiAEZQNdBWxmn7Tz0D35CMSySUld6zOcbdOXhMqZL5J3+U3r/e9jN7wyAtrQc/7KYt/2t0SX+PoorQwf48smvo4roQvsZjNg6OH/mA3/3n5ZXPeSrxmKFXv2fGW+fdf5NHYBu0o8V5aRRMcAsnyDJz6BP/yQXd0MXZxRP4gf9h9srGtTQrT6fysoN7+Q4WJT4PzGovPTT1tLVIHzj7STMmI6ftMo1SF6Mf/efmq/bgXqDNwQVMU58JOi6IoQ316N2Pa/OqFJ7M/bfXkk+DML+z87mbG+gw7n9AY1n4zkkHcH9V5d3+auNoeJ8TeJnQ6awAAAAAA==) in the Activity Bar on the side (or use the shortcut Ctrl + Shift + X) to open the **Extensions** view.
4. Enter **ESP-IDF** in the search box, locate the [ESP-IDF](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension) extension, and click Install.
	![Search and install ESP-IDF extension in VS Code](https://docs.waveshare.com/assets/images/01-VSCode-Install-ESP-IDF-Extension-7eb3e6eafde6406a3db6740ee801f182.webp)
5. For **ESP-IDF extension versions ≥ 2.0**, the extension will automatically detect and recognize the ESP-IDF environment installed in the previous steps, requiring no manual configuration.

## Example

The best way to learn a language or development environment is to start from the basics. This section will provide a detailed guide on how to create projects, develop from existing projects, and include embedded classic tutorials such as HelloWorld and the usage of common port I2C.

### 1\. Introduction to Basic Structure of ESP-IDF Project

- **Project Structure:**
	- Open the ESP-IDF plugin, click New project, select the ESP-IDF demo -- > sample\_project -- > click Create
		- Create a new project and open it in the window, you can see the structure of VSCode as follows:
		```markdown
		├── CMakeLists.txt
		├── main
		│   ├── CMakeLists.txt
		│   └── main.c
		└── README.md
		```
- **ESP-IDF Project Details:**
	- Component: The components in ESP-IDF are the basic modules for building applications, each component is usually a relatively independent code base or library, which can implement specific functions or services, and can be reused by applications or other components, similar to the definition of libraries in Python development.
		- Component reference: The import of libraries in the Python development environment only requires to "import library name or path", while ESP-IDF is based on the C language, and the importing of libraries is configured and defined through CMakeLists.txt.
				- When we use online components, we usually use `idf.py add-dependency <componetsName>` to add online components to the project, which generates an `idf_component.yml` file for managing components.
				- The purpose of CmakeLists.txt: When compiling ESP-IDF, the build tool CMake first reads the content of the top-level CMakeLists.txt in the project directory to read the build rules and identify the content to be compiled. When the required components and demos are imported into the CMakeLists.txt, the compilation tool CMake will import everything that needs to be compiled according to the index. The compilation process is as follows:
			![ESP-IDF Compilation Process](https://docs.waveshare.com/assets/images/ESP32-P4_VSCode_ESP-IDF_GettingStart_240906_02-e7d47d6a886877ea7d19c6cc09a26ebb.webp)
- **Description of Bottom Toolbar of VS Code User Interface:**
	When opening an ESP-IDF project, the environment will be loaded automatically at the bottom. For the development of ESP32-P4, the bottom toolbar is very important, as shown in the figure:
	![Description of Bottom Toolbar of VS Code User Interface](https://docs.waveshare.com/assets/images/ESP32-P4_VSCode_ESP-IDF_GettingStart_240905_03-3a82192d04014978c132921f3aa4dd6e.webp)
	- **①. ESP-IDF Development Environment Version Manager**: When our project requires differentiation of development environment versions, it can be managed by installing different versions of ESP-IDF. When the project uses a specific version, it can be switched to by utilizing it
		- **②. Device flashing COM port**: Select to flash the compiled program into the chip
		- **③. Select set-target chip model**: Select the corresponding chip model, for example, ESP32-P4 needs to choose esp32p4 as the target chip
		- **④. menuconfig**: Click it to modify sdkconfig configuration file
		- **⑤. fullclean button**: When the project compilation error or other operations pollute the compiled content, you can clean up all the compiled content by clicking it
		- **⑥. Build project**: When a project satisfies the build, click this button to compile
		- **⑦. flash button**: When a project build is completed, select the COM port of the corresponding development board, and click this button to flash the compiled firmware to the chip
		- **⑧. monitor enable flashing port monitoring**: When a project passes through Build --> Flash, click this button to view the log of output from flashing port and debugging port, so as to observe whether the application works normally
		- **⑨.Build Flash Monitor one-click button**: Which is used to continuously execute Build->Flash->Monitor, often referred to as "little flame”

### 2\. Hello World Example

After understanding the description of bottom toolbar of VS Code user interface, the Hello World project allows you to quickly get started and understand the basic projects of the ESP32 development environment. It demonstrates how to use ESP-IDF to create a basic application, and covers the ESP32 development process, including compilation, flashing, and monitor debugging steps.

1. After opening the sample project `HelloWorld`, set the target port and chip type (Note: There is a loading action in the lower right corner when the chip type is selected, indicating that ESP-IDF is executing the command `idf.py set-target esp32p4`. It needs to pull the architecture package environment corresponding to the chip from the package manager, which may take some time. Please wait patiently. If you perform build or other operations at this time, there will be errors!!!)
2. By using the bottom tool 🔥 to build, burn, and monitor with just one click, you can view the terminal output Hello World!
3. Code content analysis
	1. There is only one `app_main` main function in the code, which determines the print content output through conditional judgment, and adds a loop at the end to achieve 10s restart of the chip.
		2. `app_main` function is the entry point for user applications in the ESP-IDF (Espressif IoT Development Framework) development framework. It is the core function of the ESP-IDF project and is equivalent to the main function in the standard program of the C language. In ESP32 development, `app_main` function is the first task scheduled by the real-time operating system (FreeRTOS), which is the starting point for the execution of the user's code.

### 3\. I2C Example

I2C is a commonly used serial communication bus, which can communicate through two lines, one data cable (SDA, Serial Data) and one clock cable (SCL, Serial Clock), and supports multi-master and multi-slave mode. The ESP32-P4 chip features two I2C bus interfaces. Internally, the GPIO switch matrix allows these interfaces to be configured to use any GPIO pin. This flexibility enables users to freely assign any GPIO as I2C pins. Additionally, the ESP32-P4 I2C supports both slave and master modes. The following section focuses on the I2C master mode, which is used by the ESP32-P4 to initiate communication, control, and send data requests to or receive data from slave devices (such as any I2C‑compatible sensor). On the ESP32-P4, the I2C pins are configured by default as `SCL(GPIO8)` and `SDA(GPIO7)`

![I2C Wiring Diagram](data:image/webp;base64,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)

In ESP-IDF, the I2C bus must be configured using the `i2c_master_bus_config_t`:

- `i2c_master_bus_config_t::clk_source` selects the source clock for the I2C bus. To use the default I2C clock source (which is typically recommended), set it to I2C\_CLK\_SRC\_DEFAULT.
- `i2c_master_bus_config_t::i2c_port` specifies the I2C port to be used by the controller. As mentioned earlier, the ESP32-P4 has two I2C interfaces. When two separate I2C buses need to operate simultaneously, this setting is used to distinguish between them.
- `i2c_master_bus_config_t::scl_io_num` sets the GPIO number for the Serial Clock (SCL) line. On the ESP32-P4-WIFI6-Touch-LCD-X, this is 8.
- `i2c_master_bus_config_t::sda_io_num` sets the GPIO number for the Serial Data (SDA) line. On the ESP32-P4-WIFI6-Touch-LCD-X, this is 7.
- `i2c_master_bus_config_t::glitch_ignore_cnt` defines the glitch period threshold for the Master Bus. Glitches on the line shorter than this value will be filtered out. A typical setting is 7.
- `i2c_master_bus_config_t::enable_internal_pullup` enables internal pull-up resistors. On the ESP32-P4-WIFI6-Touch-LCD-X, external I2C pull-ups are already provided, so internal pull-ups should not be enabled.

Based on the above, the I2C configuration is defined as follows:

```c
i2c_master_bus_config_t i2c_bus_config = {
       .clk_source = I2C_CLK_SRC_DEFAULT,
       .i2c_port = I2C_NUM_0,
       .scl_io_num = 8,
       .sda_io_num = 7,
       .glitch_ignore_cnt = 7,
       .flags.enable_internal_pullup = false,
   };
```
1. Open the `i2c_tools` project, select the correct COM port and chip model, then click the ⚙️ button to enter the settings. This will open a new tab: **SDK Configuration editor**, also known as menuconfig. Directly search for I2C in the search bar. You will see the relevant configuration options, and the SCL GPIO Num and SDA GPIO Num in the example code should already correspond to `SCL(GPIO8)` and `SDA(GPIO7)`.
2. Next, you can directly compile, flash, and monitor by clicking 🔥. After completion, a command menu will appear in the terminal. When you execute the i2cdetect command, it will print all I2C addresses. If a device is present, its address will be displayed as a number (the device at I2C address 0x18 is the onboard ES8311 Codec audio chip, which will be explained in detail in the I2S section), as shown in the figure:
	![i2cdetect](https://docs.waveshare.com/assets/images/ESP32-P4-Nano_I2C_demo_240906_01-6e3d0c19d8ba4a7e5af3819ad05cc629.webp)
3. The above steps have established the foundation for I2C device communication. In I2C protocol devices, it is often necessary to write register configurations to the corresponding device address via the I2C bus to enable its functions. This requires writing the I2C device initialization code in the program to drive the device. Different I2C devices have different I2C addresses. During development, we can use the i2ctools utility to scan for connected I2C addresses. Then, by consulting the device's datasheet for register maps and configuration details, we can implement communication over the I2C bus.

---

### 4\. WIFI Networking Example

The ESP32-P4 itself does not have built-in WIFI/BT functionality. However, the ESP32-P4 expands WIFI capability by connecting to an ESP32-C6 module via SDIO. The ESP32-C6 acts as a Slave and, through a set of command sets, enables the ESP32-P4 as the Host to utilize WIFI 6/BT 5 features over SDIO. After adding the following two components, seamless integration with `esp_wifi` can be achieved.

```c
// In a WIFI project, add the following two components using the ESP-IDF component management tool
// Depending on the component version, different versions may be required; actual results may vary
idf.py add-dependency espressif/esp_wifi_remote==0.14.*
idf.py add-dependency espressif/esp_hosted==1.4.*
```
1. Open the `wifistation` project and proceed to add the required components.
	![Add espressif/esp_wifi_remote and espressif/esp_hosted Conponent](https://docs.waveshare.com/assets/images/ESP32-P4-Nano-WiFistation_240907_01-2b58a8a6b960420c3d4ea962375d18a3.webp)
2. The image above illustrates the specific steps for adding components:
	1. Open the ESP-IDF Terminal.
		2. Add the required components in the Terminal.
		3. After successful addition, an `idf_component.yml` file will appear in the main folder of the project. As explained in the ESP‑IDF project directory section, this file is used to manage project components.
		4. Opening this file, it can be seen that two components have been added: `espressif/esp_hosted: "1.4.*"` and `espressif/esp_wifi_remote: "0.14.*"`. These components will be included in the project during the build process.
3. Next, click the ⚙️ button to open the settings. Search for Example and configure the **ssid** and **password** of the target Wi‑Fi network. **Note: The ESP32‑C6 supports 2.4 GHz Wi‑Fi 6. Make sure to select a Wi‑Fi network operating in the 2.4 GHz band.** After modifying the settings, remember to save them; otherwise, errors may occur.
	![Configure Wi-Fi Information](https://docs.waveshare.com/assets/images/ESP32-P4-Nano-WiFistation_240907_02-e754c49fed111997a9d7127438373661.webp)
4. Next, you can directly compile, flash, and monitor by clicking 🔥. After completion, the terminal will display the following result, indicating that the ESP32-P4-WIFI6-Touch-LCD-X has successfully connected to the Wi‑Fi network and is online:
	![Wi-Fi Networking Example Output](https://docs.waveshare.com/assets/images/ESP32-P4-Nano-WiFistation_240907_03-ccfcf23e9853749026c8e61b45836eb9.webp)

### 5\. SDMMC Example

The ESP32-P4 features an onboard 4-Wire SDIO 3.0 card slot for expanding off-chip storage.

- **Supported Rate Modes**
	- Default rate (20 MHz)
		- High-speed mode (40 MHz)
- **Configuring Bus Width and Frequency**
	In ESP-IDF, configuration is set using `sdmmc_host_t` and `sdmmc_slot_config_t `. For example, to set the default 20 MHz communication frequency with a 4‑line bus width, it would be:
	```c
	sdmmc_host_t host = SDMMC_HOST_DEFAULT();
	sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
	```
	In the design that supports 40 MHz communication, you can adjust the max\_freq\_khz field in the sdmmc\_host\_t structure to increase the bus frequency:
	```c
	sdmmc_host_t host = SDMMC_HOST_DEFAULT();
	host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
	```
	The SDMMC 4-wire connection on the ESP32-P4 should be defined as:
	```c
	sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
	slot_config.width = 4;
	slot_config.clk = 43;
	slot_config.cmd = 44;
	slot_config.d0 = 39;
	slot_config.d1 = 40;
	slot_config.d2 = 41;
	slot_config.d3 = 42;
	slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
	```
	1. Open the SDMMC project, select the appropriate COM port and chip model. Since the demo project defines the pins as macros, they need to be configured here; alternatively, you can directly enter the pin numbers. Click `⚙️` button to enter the settings. This will open a new tab: SDK Configuration editor, also known as menuconfig. In the search bar, type sd to find the relevant configuration. The example settings are already prepared. Enable the option for default initialization and ensure the example file is created by default.
		![ESP-IDF Configuration SDMMC](https://docs.waveshare.com/assets/images/ESP32-P4-Nano-SDMMC_240906_02-41bd02dca294658e47e7215efa738676.webp)
		2. Next, insert the prepared TF card. Click 🔥 to compile, flash and monitor. After completion, the terminal will display a command menu and list the contents of the directory on the TF card.
		![SDMMC Example Output](https://docs.waveshare.com/assets/images/ESP32-P4-Nano-SDMMC_240906_03-d0044128e94e2569c80bd1d29a5bf31f.webp)

### 6\. I2S Audio Example

**I2S (Inter-IC Sound)** is a digital communication protocol designed for transmitting audio data. I2S is a serial bus interface that is primarily used for digital audio data transmission between audio devices, such as digital audio processors (DSPs), digital-to-analog converters (DACs), analog-to-digital converters (ADCs), and audio codecs. The ESP32-P4 includes one I2S peripheral. By configuring this peripheral with the I2S driver, sampled audio data can be input and output.

The ESP32-P4 board integrates an ES8311 Codec chip and an NS4150B power amplifier. The I2S bus and pin assignments are as follows:

- **MCLK (Master Clock)**: The master clock signal. The clock is typically provided to the ES8311 by an external device (such as an MCU or DSP), which serves as the clock source for its internal digital audio processing module.
- **SCLK (Serial Clock)**: The serial clock signal. This signal is typically used for clock synchronization for I2S data transmission and is generated by the master device to indicate the rate at which the data is transferred. The transmission of each bit of each audio sample requires a clock cycle.
- **ASDOUT (Audio Serial Data Output)** or **DOUT**: The audio data output pin. The ES8311 outputs decoded digital audio data to this pin, which is then transmitted to an amplifier chip or other audio device.
- **LRCK (Left/Right Clock)** or **WS (Word Select)**: The left/right channel selection signal, indicating whether the current data sample belongs to the left or right channel. Typically in the I2S protocol, one clock cycle represents the left channel data and the other clock cycle represents the right channel data.
- **DSDIN (Digital Serial Data Input)** or **DIN**: The digital audio data input pin. This pin receives audio data from an external audio device or a master. The ES8311 decodes this data and processes the audio signals through an internal digital signal processing module.

![Audio Signal Processing Block Diagram of ESP32-P4 with ES8311 Codec and NS4150B Power Amplifier](https://docs.waveshare.com/assets/images/ESP32-P4-Nano-i2scodec_240909_05-fc24d386e8de8af763d24d3cd27ea4f4.webp)

| Functional Pin | ESP32-P4 Pin |
| --- | --- |
| MCLK | GPIO13 |
| SCLK | GPIO12 |
| ASDOUT | GPIO11 |
| LRCK | GPIO10 |
| DSDIN | GPIO9 |
| PA\_Ctrl (Power amplifier chip enable pin, active high) | GPIO53 |

The ES8311 driver for ESP32-P4 utilizes the [ES8311](https://components.espressif.com/component/espressif/es8311) component, which can be added via the IDF Component Manager:

```powershell
idf.py add-dependency "espressif/es8311"
```
1. Open the `i2scodec` project and proceed to add the required components.
	![Add espressif/es8311 Component](https://docs.waveshare.com/assets/images/ESP32-P4-Nano-i2scodec_240909_01-a1706d772648b1691087e1b40b1ce3ba.webp)
	1. Open the ESP-IDF Terminal.
		2. Add the required components in the Terminal.
		3. After successful addition, an `idf_component.yml` file will appear in the main folder of the project. As explained in the ESP‑IDF project directory section, this file is used to manage project components.
		4. Once opened, you can see that `espressif/es8311` component has been added, and will be included in the project during the build process.
2. Next, click the ⚙️ button to open the settings, search for Example, and adjust the volume to a suitable level.
	![Configure Volume](https://docs.waveshare.com/assets/images/ESP32-P4-Nano-i2scodec_240909_02-9257233be8f6ea1c0310627f7dfd7a5e.webp)
3. Connect a speaker, you can directly compile, flash, and monitor by clicking 🔥. After completion, the terminal will display the following result, indicating that the ESP32-P4-WIFI6-Touch-LCD-X is now playing audio.
	![I2S Audio Sample Output](https://docs.waveshare.com/assets/images/ESP32-P4-Nano-i2scodec_240909_03-c4036810ce5c18517dc75bff3d1845e1.webp)
4. When the `echo` mode is set in the settings, the audio will be recorded by the microphone and output through the speaker.
	![Set echo Mode](https://docs.waveshare.com/assets/images/ESP32-P4-Nano-i2scodec_240909_04-686a19883e2a921779e4be59414d5353.webp)

### 7\. MIPI-DSI Display Example

The ESP32-P4 utilizes the ESP32-P4NRW32 chip, which features the following new capabilities:

- Compliant with the MIPI-DSI protocol, using D-PHY v1.1, supporting up to 2-lane x 1.5Gbps (3Gbps total)
- Supports RGB888, RGB565, and YUV422 input formats
- Supports RGB888, RGB666, and RGB565 output formats
- Uses video mode to output video streams and supports outputting fixed image patterns

For MIPI-DSI image processing, it can also utilize the 2D-DMA controller, supporting the PPA and JPEG codec peripherals.

**MIPI-DSI LCD Driving Principle**

![MIPI-DSI LCD Driving Principle](https://docs.waveshare.com/assets/images/ESP32-P4-Nano-ETH_to_WiFi_240925_02-1d37e45ee59edc6a3aa5e37696fc5f4b.webp)

**Hardware Required**

ESP32-P4-WIFI6-Touch-LCD-X Any Kit

**Display Initialization Steps**

1. The compatible screen driver has been packaged as a component, available in the [ESP Component Registry](https://components.espressif.com/components?q=namespace:waveshare)
2. Open the corresponding project, select the esp32p4 target, then proceed by clicking 🔥 to compile, flash, and monitor. Upon completion, you can observe that the screen has lit up and is displaying color bars.
	![ESP32-P4-WIFI6-Touch-LCD-7-260109-01](https://docs.waveshare.com/assets/images/ESP32-P4-WIFI6-Touch-LCD-7-260109-01-ee0a49f2ac99784375dcd0fd76025199.webp)

---

### 8\. LVGL HMI Human-Machine Interface

This example shows that the ESP32-P4 displays LVGL images through the MIPI DSI interface, which fully demonstrates the powerful image processing capabilities of the ESP32-P4

**Hardware Required**

ESP32-P4-WIFI6-Touch-LCD-X Any Kit

**Display Initialization Steps**

1. The compatible screen driver is packaged as a component and invoked via the BSP (Board Support Package).
2. After opening the project, configure the relevant parameters in menuconfig under the Display settings. Select the esp32p4 target, then proceed by clicking 🔥 to compile, flash, and monitor. Upon completion, the display will show the rendered images.

| ![ESP32-P4-WIFI6-Touch-LCD-7-260109-02](https://docs.waveshare.com/assets/images/ESP32-P4-WIFI6-Touch-LCD-7-260109-02-d1f308196c85bf78264e865cdd0e39f7.webp) | ![ESP32-P4-WIFI6-Touch-LCD-7-260109-03](https://docs.waveshare.com/assets/images/ESP32-P4-WIFI6-Touch-LCD-7-260109-03-3fe244b600bef29cecc95fdc124a24c2.webp) | ![ESP32-P4-WIFI6-Touch-LCD-7-260109-04](https://docs.waveshare.com/assets/images/ESP32-P4-WIFI6-Touch-LCD-7-260109-04-14feeb4e6bdda2e6ac2152b80113b046.webp) |
| --- | --- | --- |

### 9\. Camera LCD Display

This example showcases ESP32-P4's robust image processing power by capturing video from a camera via the MIPI CSI interface and displaying it in real-time on a screen via the MIPI DSI interface.

**Hardware Required**

ESP32-P4-WIFI6-Touch-LCD-X Any Kit

**Display Initialization Steps**

1. The compatible screen driver is packaged as a component and invoked via the BSP (Board Support Package).
2. After opening the project, configure the relevant parameters in menuconfig under the Display settings. Select the esp32p4 target, then proceed by clicking 🔥 to compile, flash, and monitor. Upon completion, the display will show the rendered images.

### 10\. MP4 Player

This example showcases ESP32-P4's robust image processing power by capturing video from a camera via the MIPI CSI interface and displaying it in real-time on a screen via the MIPI DSI interface.

**Hardware Required**

- ESP32-P4-WIFI6-Touch-LCD-X Any Kit
- A TF card (with storage capacity ≥ 16GB, Class 10, formatted in FAT32)
- **File Format**: `.mp4`
- **Download Link**: [test\_video.mp4](https://dl.espressif.com/AE/esp-dev-kits/test_video.mp4)

**Recommended Settings**

**High Quality (720x1280/800x1280, RGB888 displays):**

```bash
# scale=800:1280
ffmpeg -i input.mp4 -c:v mjpeg -q:v 3 -vf scale=720:1280 -r 20 -c:a aac output.mp4
```

**Display Initialization Steps**

1. Place the provided video file onto the TF card, and then insert the card into the main board's card slot.
2. After opening the project, configure the relevant parameters in menuconfig under the Display settings. Select the esp32p4 target, then proceed by clicking 🔥 to compile, flash, and monitor. Upon completion, the display will show the rendered images.

### 11\. ESP-Phone

This example is based on [ESP\_Brookesia](https://github.com/espressif/esp-brookesia) and demonstrates an Android-like interface containing various applications. This example uses the board's MIPI-DSI port, MIPI-CSI port, ESP32-C6, TF card slot, and audio jack. Based on this example, you can create a use case based on ESP\_Brookesia to efficiently develop multimedia applications.

**Hardware Required**

- ESP32-P4-WIFI6-Touch-LCD-X Any Kit

**Display Initialization Steps**

1. After opening the project, select esp32p4 core, and you can directly click 🔥 to compile, flash, and monitor. Upon completion, the display will show the rendered images.

| ![ESP32-P4-WIFI6-Touch-LCD-7-260109-08](https://docs.waveshare.com/assets/images/ESP32-P4-WIFI6-Touch-LCD-7-260109-07-6ab5e72001c6cbda8a1650fa028dfcc2.webp) | ![ESP32-P4-WIFI6-Touch-LCD-7-260109-09](https://docs.waveshare.com/assets/images/ESP32-P4-WIFI6-Touch-LCD-7-260109-06-4aa1b06f9e46651ec97d198545ed3614.webp) | ![ESP32-P4-WIFI6-Touch-LCD-7-260109-10](https://docs.waveshare.com/assets/images/ESP32-P4-WIFI6-Touch-LCD-7-260109-05-a977dc3ecab2ee98b0ad2b0d552a1aa2.webp) |
| --- | --- | --- |