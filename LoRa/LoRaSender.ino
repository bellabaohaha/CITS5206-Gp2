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
 
TinyGPSPlus gps;
HardwareSerial GPS(1);
int counter = 0;
 
// data structure to send over LoRa
struct SensorData
{
  int moisture_1;
  int moisture_2;
  int moisture_3;
};
 
 
struct BoardData
{
  int counter;
  float latitude;
  float longitude;
  String date;
  String time;
};
 
// function to read sensor data and put into structure
void readData(SensorData &sensorData, BoardData &boardData) {
    // ====== READ actual moisture values from sensors ====== //
    // Replace the following lines with actual sensor readings
 
    sensorData.moisture_1 = 123; // Example reading
    sensorData.moisture_2 = 321; // Example reading
    sensorData.moisture_3 = 456; // Example reading
 
 
    // Board Data: GPS, Date
    boardData.counter = counter;
 
    //  GPS
    // if (gps.location.isValid()) {
    //   boardData.latitude = gps.location.lat();
    //   boardData.longitude = gps.location.lng();
    // } else {
    //   boardData.latitude = 0.0;
    //   boardData.longitude = 0.0;
    // }
 
//     // Date
//     if (gps.date.isValid()) {
//       boardData.date = String(gps.date.month()) + "/" + String(gps.date.day()) + "/" + String(gps.date.year());
//     } else {
//       boardData.date = "INVALID";
//     }
 
//     // Time
//     if (gps.time.isValid()) {
//       boardData.time = (gps.time.hour() < 10 ? "0" : "") + String(gps.time.hour()) + ":"
//                     + (gps.time.minute() < 10 ? "0" : "") + String(gps.time.minute()) + ":"
//                     + (gps.time.second() < 10 ? "0" : "") + String(gps.time.second()) + "."
//                     + (gps.time.centisecond() < 10 ? "0" : "") + String(gps.time.centisecond());
//     } else {
//       boardData.time = "INVALID";
//     }
 
}
 
void setup()
{
    setupBoards();
    // When the power is turned on, a delay is required.
    delay(1500);

    GPS.begin(9600);  // 9600 is a common baud rate for GPS modules
    
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
  BoardData boardData;
 
  readData(sensorData, boardData);
 
  // Construct the data string
  String dataString = "";
    // dataString += boardData.counter;
    // dataString += ",";
    // dataString += String(boardData.latitude, 6);  // 6 decimal places for latitude
    // dataString += ",";
    // dataString += String(boardData.longitude, 6); // 6 decimal places for longitude
    // dataString += ",";
    dataString += sensorData.moisture_1;
    dataString += ",";
    dataString += sensorData.moisture_2;
    dataString += ",";
    dataString += sensorData.moisture_3;
    // dataString += ",";
    // dataString += boardData.date; // Add the formatted date
    // dataString += ",";
    // dataString += boardData.time; // Add the formatted time
 
  // Print the data string to the console
  Serial.print("Sending packet: ");
  Serial.println(dataString);
 
  // Send packet
  LoRa.beginPacket();
  LoRa.print(dataString);
  LoRa.endPacket();
 
  counter++;
  delay(2000);
}