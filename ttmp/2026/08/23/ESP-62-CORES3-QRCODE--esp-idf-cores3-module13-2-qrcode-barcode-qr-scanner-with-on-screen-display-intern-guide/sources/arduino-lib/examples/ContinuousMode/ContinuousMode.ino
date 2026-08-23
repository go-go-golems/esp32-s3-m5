/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include <Arduino.h>
#include <M5Unified.h>
#include <M5ModuleQRCode.h>

M5ModuleQRCode module_qrcode;

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
        M5.Display.println(">> Init module failed, retry...");
        delay(1000);
    }
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.println(">> Init module success");

    /* Set light mode */
    M5.Display.println(">> Set lights mode");
    module_qrcode.setFillLightMode(QRCodeM14::FILL_LIGHT_ON_DECODE);
    module_qrcode.setPosLightMode(QRCodeM14::POS_LIGHT_ON_DECODE);

    /* Set trigger mode */
    M5.Display.println(">> Set trigger mode to continuous");
    module_qrcode.setTriggerMode(QRCodeM14::TRIGGER_MODE_CONTINUOUS);
    module_qrcode.stopDecode();

    M5.Display.println(">> Click BtnA to toggle scanning");
}

void loop()
{
    M5.update();

    static bool is_scanning = false;

    /* If BtnA was clicked */
    if (M5.BtnA.wasClicked()) {
        is_scanning = !is_scanning;

        if (is_scanning) {
            module_qrcode.startDecode();
        } else {
            module_qrcode.stopDecode();
        }

        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.println(is_scanning ? ">> Start scanning..." : ">> Stop scanning");
    }

    /* Update module */
    module_qrcode.update();

    /* If scan result available */
    if (module_qrcode.available()) {
        auto result = module_qrcode.getScanResult();

        /* Display result */
        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.println(">> Get code:");
        M5.Display.setTextColor(TFT_YELLOW);
        M5.Display.println(result.c_str());
    }
}