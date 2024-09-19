#include <LoRa.h>
#include "boards.h"

#define PREFIX '!'

// Define a structure for packet communication
struct Packet {
  int type;       // 0 is rtc, 1 is data, 2 is error nack
  int num;        // packet number or sequence number
  char prefix;    // prefix for communication and filtering
  char value[100];  // array to hold the formatted string
};

const int dry = 2935; // value for dry sensor
const int wet = 1350; // value for wet sensor
static String payload = "My payload";


// Function to send formatted moisture data at three depths
void sendMoistureData(String data) {
  LoRa.beginPacket();
  Serial.println("Transmitting soil moisture data:");

  Packet packet = {
    1,                // Data packet type
    0,                // Packet number or sequence
    PREFIX,           // Communication prefix
    {0}               // Empty for now
  };

  // Copy the formatted data into the packet's value field
  data.toCharArray(packet.value, 100);  // Convert String to char array

  LoRa.write((uint8_t*)&packet, sizeof(Packet));  // Write the entire packet
  LoRa.endPacket();
  delay(1000);  // Delay between transmissions
}

void setup() {
  initBoard();
  // When the power is turned on, a delay is required.
  delay(1500);
  Serial.begin(115200);  // Initialize serial communication

  Serial.println("LoRa Sender");
  LoRa.setPins(RADIO_CS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN);
  if (!LoRa.begin(LoRa_frequency)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

 // rtc(payload.length());  // Send RTC packet
}

void loop() {
  // Read the sensor values for 3 depths
  int sensorReading1 = analogRead(14);  // Sensor at 10cm
  int sensorReading2 = analogRead(13);  // Sensor at 20cm
  int sensorReading3 = analogRead(15);  // Sensor at 30cm

  // Map the sensor readings to percentages
  int percentageHumidity1 = map(sensorReading1, wet, dry, 100, 0);
  int percentageHumidity2 = map(sensorReading2, wet, dry, 100, 0);
  int percentageHumidity3 = map(sensorReading3, wet, dry, 100, 0);

  // Create a formatted string with the moisture values
  String moistureData = "Moisture at 10cm: " + String(percentageHumidity1) + 
                        "%, Moisture at 20cm: " + String(percentageHumidity2) + 
                        "%, Moisture at 30cm: " + String(percentageHumidity3) + "%";

  // Print the formatted string to the Serial Monitor
  Serial.println(moistureData);
  Serial.println("<------------------------->");
  // Send the formatted string as a packet
  sendMoistureData(moistureData);

  delay(5000);  // Delay between transmissions
}
