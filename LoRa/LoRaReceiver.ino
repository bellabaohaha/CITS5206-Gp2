// Only supports SX1276/SX1278
#include <LoRa.h>
#include "LoRaBoards.h"
#include <TinyGPS++.h>
 
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           915.0
#endif
#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   17
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             125.0
#endif
 
 
#if !defined(USING_SX1276) && !defined(USING_SX1278)
#error "LoRa example is only allowed to run SX1276/78. For other RF models, please run examples/RadioLibExamples
#endif
 
TinyGPSPlus gps;
HardwareSerial GPS(1);
 
void setup()
{
    setupBoards();
    // When the power is turned on, a delay is required.
    delay(1500);
 
    Serial.println("LoRa Receiver");
 
#ifdef  RADIO_TCXO_ENABLE
    pinMode(RADIO_TCXO_ENABLE, OUTPUT);
    digitalWrite(RADIO_TCXO_ENABLE, HIGH);
#endif
 
    LoRa.setPins(RADIO_CS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN);
    if (!LoRa.begin(CONFIG_RADIO_FREQ * 1000000)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }
 
    LoRa.setTxPower(CONFIG_RADIO_OUTPUT_POWER);
 
    LoRa.setSignalBandwidth(CONFIG_RADIO_BW * 1000);
 
    LoRa.setSpreadingFactor(10);
 
    LoRa.setPreambleLength(16);
 
    LoRa.setSyncWord(0xAB);
 
    LoRa.disableCrc();
 
    LoRa.disableInvertIQ();
 
    LoRa.setCodingRate4(7);
 
    // put the radio into receive mode
    LoRa.receive();
 
}
 
void loop()
{
    // try to parse packet
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        // received a packet
        Serial.print("Received packet:");
 
        String recv = "";
        // read packet
        while (LoRa.available()) {
            recv += (char)LoRa.read();
        }
 
        float latitude = 0.0;
        float longitude = 0.0;
 
        // combine receiver GPS info
        if (gps.location.isValid()) {
          latitude = gps.location.lat();
          longitude = gps.location.lng();
        }
 
        recv += ",";
        recv += String(latitude, 6);
 
        recv += ",";
        recv += String(longitude, 6);
 
        // print RSSI of packet
        recv += ",";
        recv += LoRa.packetRssi();
 
        // Convert recv string to JSON format and output to serial.
        // FIXME HERE

        Serial.println(recv);
        
    }
}
 