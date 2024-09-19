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

// data structure to send over LoRa
struct SensorData
{
  int moisture_sensor_values[3];   // integer array of size 3
};
 
// function to read sensor data and put into structure
void readData(SensorData &sensorData) {
    // ====== READ actual moisture values from sensors ====== //
    // Replace with actual sensor readings in array format
    sensorData.moisture_sensor_values[0] = 123; // Example reading
    sensorData.moisture_sensor_values[1] = 321; // Example reading
    sensorData.moisture_sensor_values[2] = 456; // Example reading

}
 
void setup()
{
    setupBoards();
    // When the power is turned on, a delay is required.
    delay(1500);
    
    Serial.begin(115200);
 
#ifdef  RADIO_TCXO_ENABLE
    pinMode(RADIO_TCXO_ENABLE, OUTPUT);
    digitalWrite(RADIO_TCXO_ENABLE, HIGH);
#endif
 
    Serial.println("LoRa Sender");
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
}
 
void loop()
{
  SensorData sensorData;
  String dataString;
 
  // unpack sensor data and construct data string for sending over LoRa
  for (int i = 0; i < 3; i++) {
    if (i == 0) dataString += "["
    dataString += String(sensorData.moisture_sensor_values[i]);
    if (i == 2) dataString += "]";
    if (i < 2) dataString += ",";
  }

  readData(sensorData, boardData);

  // Print the data string to the console
  Serial.print("Sending packet: ");
  Serial.println(dataString);
 
  // Send packet
  LoRa.beginPacket();
  LoRa.print(dataString);
  LoRa.endPacket();

  delay(2000);
}