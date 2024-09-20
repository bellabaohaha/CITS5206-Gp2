#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <esp_wifi.h>
#include <axp20x.h>
 
TinyGPSPlus gps;
HardwareSerial GPS(1);
AXP20X_Class axp;

 
// Define GPIO pins used to control power to GPS and LoRa modules
const int gpsPowerPin1 = 12;  // Example: GPIO 14 for GPS power control
const int gpsPowerPin2 = 15;  // Example: GPIO 14 for GPS power control
const int loraPowerPin1 = 23;  // Example: GPIO 25 for LoRa power control
const int loraPowerPin2 = 26;  // Example: GPIO 25 for LoRa power control
const int loraPowerPin3 = 27;  // Example: GPIO 25 for LoRa power control
 
void setup() {
  Serial.begin(115200);
  axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
  axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
  GPS.begin(9600, SERIAL_8N1, 34, 12); // GPS uses TX = 12, RX = 34
 
  // Set power control GPIOs as OUTPUT
  pinMode(gpsPowerPin1, OUTPUT);
  pinMode(gpsPowerPin2, OUTPUT);
  pinMode(loraPowerPin1, OUTPUT);
  pinMode(loraPowerPin2, OUTPUT);
  pinMode(loraPowerPin3, OUTPUT);
 
  // Turn on GPS and LoRa modules
  turnOnGPS();
  Serial.println("After turning on GPS:");
  while(1)
  {
    if(gps.satellites.value() != 0)
    {
      break;
    }
    else
    {
      Serial.print("Should Work  : ");
      Serial.print("Latitude  : ");
      Serial.println(gps.location.lat(), 5);
      Serial.print("Longitude : ");
      Serial.println(gps.location.lng(), 4);
      Serial.print("Satellites: ");
      Serial.println(gps.satellites.value());
      Serial.print("Altitude  : ");
      Serial.print(gps.altitude.feet() / 3.2808);
      Serial.println("M");
      Serial.print("Time      : ");
      Serial.print(gps.time.hour());
      Serial.print(":");
      Serial.print(gps.time.minute());
      Serial.print(":");
      Serial.println(gps.time.second());
      Serial.print("Speed     : ");
      Serial.println(gps.speed.kmph()); 
      Serial.println("**********************");
      smartDelay(1000);
    }
  }
  
  turnOnLoRa();
 
  Serial.println("Modules turned ON. Performing task...");
  // Simulate performing some task
  // delay(5000);  // Keep the modules powered on for 5 seconds
  // Turn off GPS and LoRa modules
  turnOffGPS();

  Serial.println("Checking after turning off GPS ");

  smartDelay(1000);

  Serial.println("Printing Message after smartDelay ");

  Serial.print("Should not  Work  : ");
  Serial.print("Latitude  : ");
  Serial.println(gps.location.lat(), 5);
  Serial.print("Longitude : ");
  Serial.println(gps.location.lng(), 4);
  Serial.print("Satellites: ");
  Serial.println(gps.satellites.value());
  Serial.print("Altitude  : ");
  Serial.print(gps.altitude.feet() / 3.2808);
  Serial.println("M");
  Serial.print("Time      : ");
  Serial.print(gps.time.hour());
  Serial.print(":");
  Serial.print(gps.time.minute());
  Serial.print(":");
  Serial.println(gps.time.second());
  Serial.print("Speed     : ");
  Serial.println(gps.speed.kmph()); 
  Serial.println("**********************");

  smartDelay(1000);

  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data received: check wiring"));

 

  turnOffLoRa();
 
  Serial.println("Modules turned OFF. Going to deep sleep...");
 
  // Enter deep sleep
  esp_sleep_enable_timer_wakeup(30 * 1000000);  // Sleep for 30 seconds
  esp_deep_sleep_start();
}
 
void loop() {
  // No code needed here, the device will enter deep sleep in the setup
}
 
// Function to turn on GPS module
void turnOnGPS() {
  digitalWrite(gpsPowerPin1, HIGH);  // Set GPIO high to power on GPS
  digitalWrite(gpsPowerPin2, HIGH);  // Set GPIO high to power on GPS
  Serial.println("GPS module powered ON.");
}
 
// Function to turn off GPS module
void turnOffGPS() {
  digitalWrite(gpsPowerPin1, LOW);  // Set GPIO low to power off GPS
  digitalWrite(gpsPowerPin2, LOW);  // Set GPIO low to power off GPS
  Serial.println("GPS module powered OFF.");
}
 
// Function to turn on LoRa module
void turnOnLoRa() {
  digitalWrite(loraPowerPin1, HIGH);  // Set GPIO high to power on LoRa
  digitalWrite(loraPowerPin2, HIGH);
  digitalWrite(loraPowerPin3, HIGH);
  Serial.println("LoRa module powered ON.");
}
 
// Function to turn off LoRa module
void turnOffLoRa() {
  digitalWrite(loraPowerPin1, LOW);  // Set GPIO low to power off LoRa
  digitalWrite(loraPowerPin2, LOW);  
  digitalWrite(loraPowerPin3, LOW);  
  Serial.println("LoRa module powered OFF.");
}


static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (GPS.available())
      gps.encode(GPS.read());
  } while (millis() - start < ms);
}

 
