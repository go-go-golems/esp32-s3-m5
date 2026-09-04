#include <Arduino.h>
#include <M5Unified.h>
#include <M5ModuleQRCode.h>

M5ModuleQRCode module_qrcode;
static bool is_usb_mode = false;

void setup()
{
    M5.begin();
    M5.Display.setFont(&fonts::efontCN_16);
    M5.Display.setTextScroll(true);

    /* Set module config */
    auto cfg     = module_qrcode.getConfig();
    cfg.pin_tx   = 17;
    cfg.pin_rx   = 16;
    cfg.baudrate = 115200;
    cfg.serial   = &Serial1;
    module_qrcode.setConfig(cfg);

    /* Init module */
    while (!module_qrcode.begin()) {
        M5.Display.setTextColor(TFT_RED);
        M5.Display.println(">> Init module failed, retrying...");
        delay(1000);
    }

    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.println(">> Port Initial Setting\n(5s later, it will skip)");
    M5.Display.setTextColor(TFT_YELLOW);
    M5.Display.println(
        "If you are unsure which port was configured on the device before the last power-off, please scan the serial "
        "communication configuration code when this message appears; otherwise, the QRCode module may not start "
        "correctly.");

    module_qrcode.setTriggerLevel(false);
    delay(30);
    module_qrcode.setTriggerLevel(true);
    delay(5000);
    M5.Display.clear();
    M5.Display.setCursor(0, 0);

    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.println(">> Init module success");
    is_usb_mode = false;

    /* Set light mode */
    M5.Display.println(">> Set lights mode");
    module_qrcode.setFillLightMode(QRCodeM14::FILL_LIGHT_ON_DECODE);
    module_qrcode.setPosLightMode(QRCodeM14::POS_LIGHT_ON_DECODE);

    /* Set trigger mode */
    M5.Display.println(">> Set trigger mode to pulse");
    module_qrcode.setTriggerMode(QRCodeM14::TRIGGER_MODE_PULSE);

    M5.Display.println(">> Click BtnA to toggle scanning");
}

void loop()
{
    M5.update();

    if (!is_usb_mode && M5.BtnA.wasClicked()) {
        /* Trigger a scan */
        module_qrcode.setTriggerLevel(false);
        delay(20);
        module_qrcode.setTriggerLevel(true);

        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.println(">> Start scanning...");
    }

    if (M5.BtnB.wasClicked()) {
        module_qrcode.startDecode();

        // If you want to use the USB virtual serial, please enable the following code.
        module_qrcode.setModeUsbSerial();
        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.println(">> Change to USB Serial");

        // If you want to use the USB HID keyboard, please enable the following code.
        // module_qrcode.setModeUsbKeyboard();
        // M5.Display.setTextColor(TFT_WHITE);
        // M5.Display.println(">> Change to USB KeyBoard");

        // If you want to use the USB HID-POS, please enable the following code.
        // module_qrcode.setModeUsbPos();
        // M5.Display.setTextColor(TFT_WHITE);
        // M5.Display.println(">> Change to USB POS");

        M5.Display.setTextColor(TFT_RED);
        M5.Display.println(
            "If you want to switch back to the serial port, please scan the Serial Communication configuration code in "
            "the user guide.\n");
        is_usb_mode = true;
    }

    /* Update module */
    module_qrcode.update();

    /* If scan result available */
    if (module_qrcode.available()) {
        is_usb_mode = false;
        auto result = module_qrcode.getScanResult();

        /* Display result */
        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.println(">> Get code:");
        M5.Display.setTextColor(TFT_YELLOW);
        M5.Display.println(result.c_str());
    }
}
