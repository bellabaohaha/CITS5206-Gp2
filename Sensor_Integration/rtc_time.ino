#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <time.h> // For ESP32 time management functions
 
TinyGPSPlus gps;
HardwareSerial GPS(1); // GPS serial
 
const int sleepTimeSec = 30;  // Time to sleep in seconds
 
void setup() {
  Serial.begin(115200);
  ss.begin(9600, SERIAL_8N1, 34, 12); // GPS uses TX = 12, RX = 34
  Serial.println("GPS Time and Sleep/Wake Example");
 
  if (!syncTimeWithGPS()) {
    Serial.println("Failed to sync with GPS. Using internal clock.");
  }
}
 
void loop() {
  // Perform your main task here, e.g., reading sensors or other operations
  Serial.println("Main task running...");
  // Sync time with GPS again if available
  if (gps.time.isValid()) {
    syncTimeWithGPS();
  }
 
  // Print current internal time
  printCurrentTime();
 
  // Go to deep sleep
  Serial.println("Going to sleep...");



  esp_sleep_enable_timer_wakeup(sleepTimeSec * 1000000); // Set wakeup time in microseconds

  
  esp_deep_sleep_start();  // Enter deep sleep




}
 
bool syncTimeWithGPS() {
  unsigned long timeout = millis() + 10000; // 10-second timeout for getting GPS fix
  while (millis() < timeout) {
    if (ss.available() > 0) {
      gps.encode(ss.read());
      if (gps.time.isValid()) {
        struct tm gpsTime;
        gpsTime.tm_year = gps.date.year() - 1900;
        gpsTime.tm_mon = gps.date.month() - 1;
        gpsTime.tm_mday = gps.date.day();
        gpsTime.tm_hour = gps.time.hour();
        gpsTime.tm_min = gps.time.minute();
        gpsTime.tm_sec = gps.time.second();
        time_t t = mktime(&gpsTime);
        struct timeval now = { .tv_sec = t };
        settimeofday(&now, NULL);  // Set the system time to GPS time
        Serial.println("GPS time synced!");
        return true;
      }
    }
  }
  return false;
}
 
void printCurrentTime() {
  time_t now;
  time(&now);
  struct tm* timeinfo = localtime(&now);
  Serial.printf("Current Time: %02d:%02d:%02d\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}