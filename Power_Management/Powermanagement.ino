#include <WiFi.h> 

void setup() {
  Serial.begin(115200);  // Starting the serial communication
  delay(1000);  // Waiting for  the serial monitor to initialize
  
  Serial.println("Disabling WiFi...");
  WiFi.mode(WIFI_OFF);  // Turn off WiFi
  WiFi.disconnect(true);  

  // Check if WiFi is turned off
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi module is off.");
  } else {
    Serial.println("WiFi module is on.");
  }

  //  To wake up every 10 seconds
  esp_sleep_enable_timer_wakeup(10 * 1000000);  // Time in microseconds

  // Going to  deep sleep
  Serial.println("Going to deep sleep now...");
  Serial.flush();  // Wait for serial output to complete
  esp_deep_sleep_start();
}

void loop() {
  
}
