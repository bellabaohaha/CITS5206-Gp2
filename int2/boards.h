#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "utilities.h"

#ifdef HAS_SDCARD
#include <SD.h>
#include <FS.h>
#endif

// #ifdef HAS_DISPLAY
// #include <U8g2lib.h>

// #ifndef DISPLAY_MODEL
// #define DISPLAY_MODEL U8G2_SSD1306_128X64_NONAME_F_HW_I2C
// #endif

// DISPLAY_MODEL *u8g2 = nullptr;
// #endif

#ifndef OLED_WIRE_PORT
#define OLED_WIRE_PORT Wire
#endif

#if defined(HAS_PMU)
#include "XPowersLib.h"

XPowersLibInterface *PMU = NULL;

#ifndef PMU_WIRE_PORT
#define PMU_WIRE_PORT   Wire
#endif

bool pmuInterrupt;

void setPmuFlag() {
//     u8g2 = true;
}

bool initPMU() {
    if (!PMU) {
        PMU = new XPowersAXP2101(PMU_WIRE_PORT);
        if (!PMU->init()) {
            Serial.println("Warning: Failed to find AXP2101 power management");
            delete PMU;
            PMU = NULL;
        } else {
            Serial.println("AXP2101 PMU init succeeded, using AXP2101 PMU");
        }
    }

    if (!PMU) {
        PMU = new XPowersAXP192(PMU_WIRE_PORT);
        if (!PMU->init()) {
            Serial.println("Warning: Failed to find AXP192 power management");
            delete PMU;
            PMU = NULL;
        } else {
            Serial.println("AXP192 PMU init succeeded, using AXP192 PMU");
        }
    }

    if (!PMU) {
        return false;
    }

    PMU->setChargingLedMode(XPOWERS_CHG_LED_BLINK_1HZ);

    pinMode(PMU_IRQ, INPUT_PULLUP);
    attachInterrupt(PMU_IRQ, setPmuFlag, FALLING);

    if (PMU->getChipModel() == XPOWERS_AXP192) {
        PMU->setProtectedChannel(XPOWERS_DCDC3);
        PMU->setPowerChannelVoltage(XPOWERS_LDO2, 3300);  // LoRa
        PMU->setPowerChannelVoltage(XPOWERS_LDO3, 3300);  // GPS
        PMU->setPowerChannelVoltage(XPOWERS_DCDC1, 3300); // OLED
        PMU->enablePowerOutput(XPOWERS_LDO2);
        PMU->enablePowerOutput(XPOWERS_LDO3);
        PMU->setProtectedChannel(XPOWERS_DCDC1);
        PMU->setProtectedChannel(XPOWERS_DCDC3);  // Protected ESP32 power source
        PMU->enablePowerOutput(XPOWERS_DCDC1);    // Enable OLED power
        PMU->disablePowerOutput(XPOWERS_DCDC2);   // Disable unused channel
        PMU->disableIRQ(XPOWERS_AXP192_ALL_IRQ);
        PMU->enableIRQ(XPOWERS_AXP192_VBUS_REMOVE_IRQ | XPOWERS_AXP192_VBUS_INSERT_IRQ |
                       XPOWERS_AXP192_BAT_CHG_DONE_IRQ | XPOWERS_AXP192_BAT_CHG_START_IRQ |
                       XPOWERS_AXP192_BAT_REMOVE_IRQ | XPOWERS_AXP192_BAT_INSERT_IRQ |
                       XPOWERS_AXP192_PKEY_SHORT_IRQ);
    }

    if (PMU->getChipModel() == XPOWERS_AXP2101) {
        // Unused power channels and configuration
        PMU->disablePowerOutput(XPOWERS_DCDC2);
        PMU->disablePowerOutput(XPOWERS_DCDC3);
        PMU->disablePowerOutput(XPOWERS_ALDO1);
        PMU->setPowerChannelVoltage(XPOWERS_ALDO2, 3300); // LoRa
        PMU->setPowerChannelVoltage(XPOWERS_ALDO3, 3300); // GNSS
        PMU->enablePowerOutput(XPOWERS_ALDO2);
        PMU->enablePowerOutput(XPOWERS_ALDO3);
        PMU->setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);
        PMU->setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
        PMU->disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
        PMU->clearIrqStatus();
        PMU->enableIRQ(XPOWERS_AXP2101_BAT_INSERT_IRQ | XPOWERS_AXP2101_BAT_REMOVE_IRQ |
                       XPOWERS_AXP2101_VBUS_INSERT_IRQ | XPOWERS_AXP2101_VBUS_REMOVE_IRQ |
                       XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ |
                       XPOWERS_AXP2101_BAT_CHG_DONE_IRQ | XPOWERS_AXP2101_BAT_CHG_START_IRQ);
    }

    PMU->enableSystemVoltageMeasure();
    PMU->enableVbusVoltageMeasure();
    PMU->enableBattVoltageMeasure();
    PMU->disableTSPinMeasure(); // Disable TS pin for battery temperature
    return true;
}

extern void disablePeripherals();
#else
#define initPMU()
#define disablePeripherals()
#endif

SPIClass SDSPI(HSPI);

void initBoard() {
    Serial.begin(115200);
    SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
    Wire.begin(I2C_SDA, I2C_SCL);

#ifdef RADIO_TCXO_EN_PIN
    pinMode(RADIO_TCXO_EN_PIN, OUTPUT);
    digitalWrite(RADIO_TCXO_EN_PIN, HIGH);
#endif

#ifdef HAS_GPS
    Serial1.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
#endif

#if OLED_RST
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, HIGH); delay(20);
    digitalWrite(OLED_RST, LOW); delay(20);
    digitalWrite(OLED_RST, HIGH); delay(20);
#endif

    initPMU();

#ifdef BOARD_LED
    pinMode(BOARD_LED, OUTPUT);
    digitalWrite(BOARD_LED, LED_ON);
#endif

// #ifdef HAS_DISPLAY
//     Wire.beginTransmission(0x3C);
//     if (Wire.endTransmission() == 0) {
//         Serial.println("Started OLED");
//         u8g2 = new DISPLAY_MODEL(U8G2_R0, U8X8_PIN_NONE);
//         u8g2->begin();
//         u8g2->clearBuffer();
//         u8g2->setFlipMode(0);
//         u8g2->setFontMode(1); // Transparent
//         u8g2->setDrawColor(1);
//         u8g2->setFontDirection(0);
//         u8g2->firstPage();
//         do {
//             u8g2->setFont(u8g2_font_inb19_mr);
//             u8g2->drawStr(0, 30, "LilyGo");
//             u8g2->drawHLine(2, 35, 47);
//             u8g2->drawHLine(3, 36, 47);
//             u8g2->drawVLine(45, 32, 12);
//             u8g2->drawVLine(46, 33, 12);
//             u8g2->setFont(u8g2_font_inb19_mf);
//             u8g2->drawStr(58, 60, "LoRa");
//         } while (u8g2->nextPage());
//         u8g2->sendBuffer();
//         u8g2->setFont(u8g2_font_fur11_tf);
//         delay(3000);
//     }
// #endif

#ifdef HAS_SDCARD
    if (u8g2) {
        u8g2->setFont(u8g2_font_ncenB08_tr);
    }
    pinMode(SDCARD_MISO, INPUT_PULLUP);
    SDSPI.begin(SDCARD_SCLK, SDCARD_MISO, SDCARD_MOSI, SDCARD_CS);
    if (u8g2) {
        u8g2->clearBuffer();
    }
    if (!SD.begin(SDCARD_CS, SDSPI)) {
        Serial.println("setupSDCard FAIL");
    } else {
        uint32_t cardSize = SD.cardSize() / (1024 * 1024);
        Serial.print("setupSDCard PASS . SIZE = ");
        Serial.print(cardSize / 1024.0);
        Serial.println(" GB");
    }
#endif
}
