#include <Wire.h>
#include <axp20x.h>  // Ensure this library is installed for battery management
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <time.h>    // For ESP32 RTC
#include "boards.h"
#include "esp_sleep.h"

// GPS object
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);  // Renamed from GPS to gpsSerial

// Define pins for the button and LED
const int buttonPin = 38; // GPIO pin connected to the button
const int ledPin = 25;    // GPIO pin connected to the external LED

// Variables to track GPS state, button state, press count, and data received
bool gpsOn = true;             // GPS is on by default
bool gpsDataReceived = false;  // Flag to indicate if GPS data has been received
int buttonState = 0;
int lastButtonState = 0;
int pressCount = 0;
unsigned long lastDebounceTime = 0;  // The last time the button state was toggled
unsigned long debounceDelay = 50;    // The debounce time; increase if needed

// Array to store GPS and battery data
float gpsData[4];  // [0] = Latitude, [1] = Longitude, [2] = Battery percentage, [3] = Time (in hhmmss format)

// Scheduled times for soil moisture readings (in minutes since midnight)
const int scheduledTimes[2] = { (14 * 60) + 10, (14 * 60) + 15 }; // 14:10 and 14:15

// Soil moisture sensor pins
const int sensorPin1 = 14;  // Sensor at 10cm
const int sensorPin2 = 13;  // Sensor at 20cm
const int sensorPin3 = 15;  // Sensor at 30cm

// Soil moisture calibration values
const int dry = 2935;  // Value for dry sensor
const int wet = 1350;  // Value for wet sensor

void setup() {
  Serial.begin(115200);

  // Initialize PMU and Board
  initBoard();  // Ensure the board is initialized before checking wake-up reason

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    // Woke up from deep sleep
    Serial.println("Woke up from deep sleep");

    // Initialize peripherals
    initPeripheralsForReadings();

    // Take readings
    takeSoilMoistureReadings();

    // Compute time until next scheduled reading
    time_t sleepDuration = getTimeUntilNextReading();
    Serial.print("Going to sleep for ");
    Serial.print(sleepDuration);
    Serial.println(" seconds.");
    esp_sleep_enable_timer_wakeup(sleepDuration * 1000000LL);
    esp_deep_sleep_start();
  } else {
    // Power-on reset
    Serial.println("Booting up...");
    // Initialize everything

    // Initialize PMU first
    initializePMU();  // Initialize PMU for battery management

    // Initialize pins
    pinMode(buttonPin, INPUT_PULLUP);    // Use internal pull-up resistor for the button
    pinMode(ledPin, OUTPUT);             // Set external LED pin as output

    // Initialize I2C
    Wire.begin(21, 22);  // Initialize I2C for battery management

    // Initialize GPS Serial
    gpsSerial.begin(9600, SERIAL_8N1, 34, 12); // GPS uses TX=12, RX=34

    // Turn on the LED initially
    digitalWrite(ledPin, HIGH);

    // GPS is on by default
    Serial.println("GPS module powered ON.");
  }
}

void loop() {
  // Handle button press to toggle GPS on and off
  handleButtonPress();

  // If GPS is on and data has not been received, read and print GPS data
  if (gpsOn && !gpsDataReceived) {
    while (gpsSerial.available()) {  // Check if data is available
      gps.encode(gpsSerial.read());  // Process GPS data
    }

    // Print GPS data and check if it has valid location data
    if (gps.location.isValid()) {
      storeGPSData();   // Store GPS data in the array
      printGPSData();   // Print the GPS data
      gpsDataReceived = true;  // Mark data as received

      // Sync GPS time with ESP32 RTC
      syncTimeWithRTC();  // Sync with RTC
    }
  }

  // After GPS is turned off and time is synced, enter deep sleep
  if (!gpsOn && gpsDataReceived) {
    // GPS is off, and we have GPS data (including time synced)
    time_t sleepDuration = getTimeUntilNextReading();
    Serial.print("Going to sleep for ");
    Serial.print(sleepDuration);
    Serial.println(" seconds.");
    esp_sleep_enable_timer_wakeup(sleepDuration * 1000000LL);
    esp_deep_sleep_start();
  }

  // Continuously check the battery status and update the placeholder battery percentage in gpsData
  checkBatteryStatus();
}

void handleButtonPress() {
  int reading = digitalRead(buttonPin);

  // Check if the button state has changed
  if (reading != lastButtonState) {
    lastDebounceTime = millis();  // Reset the debounce timer
  }

  // Check if sufficient time has passed to consider the button state stable
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // If the button state has changed and is now LOW (pressed)
    if (reading == LOW && buttonState == HIGH) {
      pressCount++;  // Increase the press count

      if (pressCount == 3) {  // Toggle GPS state after three presses
        gpsOn = !gpsOn;  // Toggle GPS state

        if (gpsOn) {
          turnOnGPS();
        } else {
          turnOffGPS();
        }

        pressCount = 0;  // Reset the press count after toggling
      }
    }
    buttonState = reading;  // Update the button state
  }

  // Store the last button state
  lastButtonState = reading;
}

void turnOnGPS() {
  Serial.println("GPS module powered ON.");
  digitalWrite(ledPin, HIGH);  // Turn on LED to indicate GPS is on
  gpsDataReceived = false;     // Reset the flag to allow new data collection
}

void turnOffGPS() {
  Serial.println("GPS module powered OFF.");
  digitalWrite(ledPin, LOW);  // Turn off LED to indicate GPS is off

  // Clear the GPS data
  gps = TinyGPSPlus();  // Reset the gps object to clear stored data
}

void storeGPSData() {
  // Store GPS data into the array
  gpsData[0] = gps.location.lat();
  gpsData[1] = gps.location.lng();

  // Update the battery percentage in gpsData[2]
  gpsData[2] = PMU->getBatteryPercent();

  if (gps.time.isValid()) {
    // Convert UTC to AWST (UTC+8) and store it in gpsData[3]
    int hour = gps.time.hour() + 8;  // AWST is UTC+8
    if (hour >= 24) hour -= 24;      // Adjust for next day if needed

    gpsData[3] = hour * 10000 + gps.time.minute() * 100 + gps.time.second();
  } else {
    gpsData[3] = 0;  // Set to 0 if time is not valid
  }
}

void printGPSData() {
  // Print the stored data from the array
  Serial.print("Latitude  : ");
  Serial.println(gpsData[0], 6);
  Serial.print("Longitude : ");
  Serial.println(gpsData[1], 6);
  Serial.print("Battery % : ");
  Serial.println(gpsData[2]);

  // Cast the time stored in gpsData[3] to an int and extract hh:mm:ss
  int timeValue = (int)gpsData[3];  // Cast the float to int for safe operations
  int hour = timeValue / 10000;
  int minute = (timeValue / 100) % 100;
  int second = timeValue % 100;
  Serial.print("Time (hh:mm:ss) : ");
  Serial.printf("%02d:%02d:%02d\n", hour, minute, second);

  Serial.println("**********************");
}

void syncTimeWithRTC() {
  // This function syncs the GPS time to the ESP32's internal RTC

  // Convert the GPS time into a tm struct
  struct tm gpsTime;
  gpsTime.tm_year = gps.date.year() - 1900;  // tm_year is years since 1900
  gpsTime.tm_mon = gps.date.month() - 1;     // tm_mon is 0-based
  gpsTime.tm_mday = gps.date.day();

  // Convert GPS UTC to AWST (UTC+8)
  gpsTime.tm_hour = gps.time.hour() + 8;
  if (gpsTime.tm_hour >= 24) gpsTime.tm_hour -= 24;

  gpsTime.tm_min = gps.time.minute();
  gpsTime.tm_sec = gps.time.second();

  time_t t = mktime(&gpsTime);  // Convert struct tm to time_t (epoch time)
  struct timeval now = {.tv_sec = t};
  settimeofday(&now, NULL);  // Set the system time to the GPS time

  Serial.println("Time synced with ESP32 RTC!");
}

void initializePMU() {
  // Initialize PMU
  initBoard();
}

void checkBatteryStatus() {
  // Continuously check the battery status
  if (PMU->isBatteryConnect()) {
    gpsData[2] = PMU->getBatteryPercent();
  } else {
    Serial.println("Battery not connected.");
  }
}

void initPeripheralsForReadings() {
  // Initialize pins for soil moisture sensors
  pinMode(sensorPin1, INPUT);
  pinMode(sensorPin2, INPUT);
  pinMode(sensorPin3, INPUT);

  // Initialize PMU
  initializePMU();

  // Initialize I2C
  Wire.begin(21, 22);
}

void takeSoilMoistureReadings() {
  // Get current time
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  // Print current time
  Serial.print("Taking readings at: ");
  Serial.printf("%02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  // Read the sensor values for 3 depths
  int sensorReading1 = analogRead(sensorPin1);  // Sensor at 10cm
  int sensorReading2 = analogRead(sensorPin2);  // Sensor at 20cm
  int sensorReading3 = analogRead(sensorPin3);  // Sensor at 30cm

  // Clamp sensor readings to calibration range
  sensorReading1 = constrain(sensorReading1, wet, dry);
  sensorReading2 = constrain(sensorReading2, wet, dry);
  sensorReading3 = constrain(sensorReading3, wet, dry);

  // Map the sensor readings to percentages
  int percentageHumidity1 = map(sensorReading1, wet, dry, 100, 0);
  int percentageHumidity2 = map(sensorReading2, wet, dry, 100, 0);
  int percentageHumidity3 = map(sensorReading3, wet, dry, 100, 0);

  // Create a formatted string with the moisture values
  String moistureData = "Moisture at 10cm: " + String(percentageHumidity1) + "%" +
                        ", Moisture at 20cm: " + String(percentageHumidity2) + "%" +
                        ", Moisture at 30cm: " + String(percentageHumidity3) + "%";

  // Print the formatted string to the Serial Monitor
  Serial.println(moistureData);
  Serial.println("<------------------------->");

  // Optionally, you can send the data via LoRa or store it as needed
}

time_t getTimeUntilNextReading() {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  // Print current time
  Serial.print("Current time: ");
  Serial.printf("%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min);

  int currentMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;

  int minutesUntilNext = 24 * 60; // Max possible time until next reading
  for (int i = 0; i < 2; i++) {
    int diff = scheduledTimes[i] - currentMinutes;
    if (diff <= 0) {
      diff += 24 * 60; // Adjust for next day
    }
    if (diff < minutesUntilNext) {
      minutesUntilNext = diff;
    }
  }

  Serial.print("Minutes until next reading: ");
  Serial.println(minutesUntilNext);

  return (time_t)(minutesUntilNext * 60); // Return time in seconds
}
