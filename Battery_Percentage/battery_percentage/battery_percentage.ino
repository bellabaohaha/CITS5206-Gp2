#include <Wire.h>
#include <axp20x.h>  // Ensure this library is installed for battery management
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <time.h>  // For ESP32 RTC
#include "boards.h"

// GPS object
TinyGPSPlus gps;
HardwareSerial GPS(1);

// Define pins for the button and LED
const int buttonPin = 38; // GPIO pin connected to the button
const int ledPin = 25;    // GPIO pin connected to the external LED

// Variables to track GPS state, button state, press count, and data received
bool gpsOn = true;  // GPS is on by default
bool gpsDataReceived = false; // Flag to indicate if GPS data has been received
int buttonState = 0;
int lastButtonState = 0;
int pressCount = 0;
unsigned long lastDebounceTime = 0;  // the last time the button state was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if needed

// Timing variables for RTC time print delay
unsigned long lastRTCPrintTime = 0;
unsigned long rtcPrintInterval = 5000;  // 5-second interval between RTC readings

// Array to store GPS and battery data
float gpsData[4]; // [0] = Latitude, [1] = Longitude, [2] = Battery percentage, [3] = Time (in hhmmss format)

void setup() {
  Serial.begin(115200);
  delay(5000);
  
  pinMode(buttonPin, INPUT_PULLUP);    // Use internal pull-up resistor for the button
  pinMode(ledPin, OUTPUT);             // Set external LED pin as output

  Wire.begin(21, 22);  // Initialize I2C for battery management

  GPS.begin(9600, SERIAL_8N1, 34, 12); // GPS uses TX=12, RX=34
  
  initializePMU();  // Initialize PMU for battery management
  
  // Turn on the LED initially
  digitalWrite(ledPin, HIGH);

  // GPS is on by default
  Serial.println("GPS module powered ON.");
}

void loop() {
  // Handle button press to toggle GPS on and off
  handleButtonPress();
  
  // If GPS is on and data has not been received, read and print GPS data
  if (gpsOn && !gpsDataReceived) {
    while (GPS.available()) { // Check if data is available
      gps.encode(GPS.read()); // Process GPS data
    }

    // Print GPS data and check if it has valid location data
    if (gps.location.isValid()) {
      storeGPSData();  // Store GPS data in the array
      printGPSData();  // Print the GPS data
      gpsDataReceived = true; // Mark data as received

      // Sync GPS time with ESP32 RTC
      syncTimeWithRTC();  // New function to sync with RTC
    }
  }
  
  // After GPS is turned off, verify if RTC is keeping time
  if (!gpsOn) {
    // Print RTC time every 5 seconds (based on rtcPrintInterval)
    if (millis() - lastRTCPrintTime >= rtcPrintInterval) {
      printTimeFromRTC();  // Print time from ESP32's internal RTC
      lastRTCPrintTime = millis();  // Update last print time
    }
  }
  
  // Continuously check the battery status and update the placeholder battery percentage in gpsData
  checkBatteryStatus();
}

void handleButtonPress() {
  int reading = digitalRead(buttonPin);

  // Check if the button state has changed
  if (reading != lastButtonState) {
    lastDebounceTime = millis(); // Reset the debounce timer
  }

  // Check if sufficient time has passed to consider the button state stable
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // If the button state has changed and is now LOW (pressed)
    if (reading == LOW && buttonState == HIGH) {
      pressCount++; // Increase the press count

      if (pressCount == 3) { // Toggle GPS state after three presses
        gpsOn = !gpsOn; // Toggle GPS state

        if (gpsOn) {
          turnOnGPS();
        } else {
          turnOffGPS();
        }
        
        pressCount = 0; // Reset the press count after toggling
      }
    }
    buttonState = reading; // Update the button state
  }

  // Store the last button state
  lastButtonState = reading;
}

void turnOnGPS() {
  Serial.println("GPS module powered ON.");
  digitalWrite(ledPin, HIGH); // Turn on LED to indicate GPS is on
  gpsDataReceived = false; // Reset the flag to allow new data collection
}

void turnOffGPS() {
  Serial.println("GPS module powered OFF.");
  digitalWrite(ledPin, LOW);  // Turn off LED to indicate GPS is off

  // Clear the GPS data
  gps = TinyGPSPlus(); // Reset the gps object to clear stored data
}

void storeGPSData() {
  // Store GPS data into the array
  gpsData[0] = gps.location.lat();
  gpsData[1] = gps.location.lng();
  
  // Update the battery percentage in gpsData[2]
  gpsData[2] = PMU->getBatteryPercent();  

  if (gps.time.isValid()) 
  {
    // Convert UTC to AWST (UTC+8) and store it in gpsData[3]
    int hour = gps.time.hour() + 8;  // AWST is UTC+8
    if (hour >= 24) hour -= 24;      // Adjust for next day if needed
    
    gpsData[3] = hour * 10000 + gps.time.minute() * 100 + gps.time.second();
  } 
  else 
  {
    gpsData[3] = 0; // Set to 0 if time is not valid
  }
}

void printGPSData() {
  // Print the stored data from the array
  Serial.print("Latitude  : "); Serial.println(gpsData[0], 6);
  Serial.print("Longitude : "); Serial.println(gpsData[1], 6);
  Serial.print("Battery % : "); Serial.println(gpsData[2]);
  
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
  gpsTime.tm_year = gps.date.year() - 1900; // tm_year is years since 1900
  gpsTime.tm_mon = gps.date.month() - 1;    // tm_mon is 0-based
  gpsTime.tm_mday = gps.date.day();
  
  // Convert GPS UTC to AWST (UTC+8)
  gpsTime.tm_hour = gps.time.hour() + 8;
  if (gpsTime.tm_hour >= 24) gpsTime.tm_hour -= 24;
  
  gpsTime.tm_min = gps.time.minute();
  gpsTime.tm_sec = gps.time.second();
  
  time_t t = mktime(&gpsTime);  // Convert struct tm to time_t (epoch time)
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);     // Set the system time to the GPS time
  
  Serial.println("Time synced with ESP32 RTC!");
}

void printTimeFromRTC() {
  // This function prints the current time from the ESP32's RTC

  time_t now;
  struct tm timeinfo;

  time(&now);  // Get the current time from the RTC
  localtime_r(&now, &timeinfo);  // Convert to local time
  
  Serial.print("Current Time (from RTC): ");
  Serial.printf("%02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

void initializePMU() {
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

