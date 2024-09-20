#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <esp_wifi.h>
#include <driver/rtc_io.h>
 
// Setup GPS and Serial
TinyGPSPlus gps;
HardwareSerial ss(1);
 
const int sleepTimeSec = 30;  // Sleep time in seconds
 
void setup() {
  Serial.begin(115200);
  ss.begin(9600, SERIAL_8N1, 34, 12); // GPS pins TX = 12, RX = 34
  Serial.println("Disabling peripherals and entering deep sleep...");
 
  // Disable WiFi and Bluetooth
  disableWiFiAndBT();
 
  // Turn off GPS module
  turnOffGPS();
 
  // Set GPIOs to reduce power consumption
  configureGPIOsForSleep();
 
  // Print a message and go to sleep
  Serial.println("Going to deep sleep...");
  esp_sleep_enable_timer_wakeup(sleepTimeSec * 1000000);  // Set wake-up time in microseconds
  esp_deep_sleep_start();  // Enter deep sleep
}
 
void loop() {
  // No operation in loop, since the device will go to sleep in setup
}
 
// Function to disable WiFi and Bluetooth
void disableWiFiAndBT() {
  // Disable WiFi
  WiFi.mode(WIFI_OFF);
  esp_wifi_stop();
 
  // Disable Bluetooth
  btStop();
 
  Serial.println("WiFi and Bluetooth disabled.");
}
 
// Function to turn off GPS module
void turnOffGPS() {
  pinMode(14, OUTPUT);  // Use GPIO 14 to control GPS power (check your board's pinout if different)
  digitalWrite(14, LOW);  // Turn off GPS by setting power control pin low
 
  Serial.println("GPS module powered off.");
}
 
// Function to configure GPIO pins for low power consumption
void configureGPIOsForSleep() {
  // Set GPIOs to INPUT with no pull-up to reduce power consumption
  for (int i = 0; i < 40; i++) {  // ESP32 has GPIO 0 to 39
    if (i == 12 || i == 34) continue;  // Skip GPS pins, leave them in default state
    if (rtc_gpio_is_valid_gpio(i)) {   // Check if it's a valid GPIO for sleep mode
      rtc_gpio_deinit(i);  // Reset the pin's function
      pinMode(i, INPUT);   // Set as input
    }
  }
 
  Serial.println("GPIOs configured for low power.");
}