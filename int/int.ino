#include <Wire.h>
#include <axp20x.h>  // Ensure this library is installed for battery management
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <time.h>    // For ESP32 RTC
#include "boards.h"
#include "esp_sleep.h"


#include <LoRa.h>
#include "LoRaBoards.h"

#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           915.0
#endif
#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   17
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             125.0
#endif

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

// Timing variables for RTC time print delay
unsigned long lastRTCPrintTime = 0;
unsigned long rtcPrintInterval = 5000;  // 5-second interval between RTC readings

// Expand the gpsData array to store date as well
float gpsData[5];  // [0]=Latitude, [1]=Longitude, [2]=Battery percentage, [3]=Time (in hhmmss format), [4]=Date (in yyyymmdd format)

// Variables to check if time is synced and if deep sleep has been entered
bool timeSynced = false;

// Sensor board ID and depths
const char* sensor_board_id = "sb1";

const int depth_1 = 10;  // Depth at 10cm
const int depth_2 = 20;  // Depth at 20cm
const int depth_3 = 30;  // Depth at 30cm

// Soil moisture sensor pins
const int sensorPin1 = 14;  // Sensor at 10cm
const int sensorPin2 = 13;  // Sensor at 20cm
const int sensorPin3 = 15;  // Sensor at 30cm

// Soil moisture calibration values
const int dry = 2935;  // Value for dry sensor
const int wet = 1350;  // Value for wet sensor

// Sleep duration in seconds (initially set to 60 for testing purposes)
const time_t sleepDuration = 60;  // Sleep duration in seconds

// Variables to store soil moisture readings
RTC_DATA_ATTR int percentageHumidity1 = 0;
RTC_DATA_ATTR int percentageHumidity2 = 0;
RTC_DATA_ATTR int percentageHumidity3 = 0;

// Variables to store last known GPS coordinates
RTC_DATA_ATTR float lastLatitude = 0.0;
RTC_DATA_ATTR float lastLongitude = 0.0;

// Variables to track last time synchronization
RTC_DATA_ATTR time_t lastSyncTime = 0;
const time_t syncInterval = 86400;  // Sync time every 24 hours

// Scheduled times for soil moisture readings (in minutes since midnight)
const int scheduledTimes[] = { (15 * 60) + 07, (15 * 60) + 10 }; // 17:10  and 17:13 

void setup() {
  setupBoards();
  Serial.begin(115200);
  initializeLoRa();
  // Replace delay with a non-blocking wait
  unsigned long startTime = millis();
  while (millis() - startTime < 5000) {
    // Allow background tasks to run
    delay(10);
  }


  // Initialize PMU and Board
  initBoard();  // Ensure the board is initialized before checking wake-up reason

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    // Woke up from deep sleep
    Serial.println("Woke up from deep sleep");
    initPeripheralsForReadings();

    // Check if it's time to take a reading
    if (isScheduledTime()) {
      takeSoilMoistureReadings();
      createAndPrintPayload();  // Create and print the payload after readings
    } else {
      Serial.println("Not a scheduled time. Going back to sleep.");
    }

    // Calculate time until next scheduled reading
    time_t sleepTime = getTimeUntilNextScheduledReading();

    Serial.print("Going to sleep for ");
    Serial.print(sleepTime);
    Serial.println(" seconds.");

    esp_sleep_enable_timer_wakeup(sleepTime * 1000000LL);
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

      // Update last sync time
      lastSyncTime = time(NULL);

      // Do not go to sleep here; wait for GPS to be turned off manually
    }
    else{
      Serial.println("Waiting for GPS Data");
      delay(5000);
    }
  }

  // After GPS is turned off and time is synced, enter deep sleep
  if (!gpsOn && gpsDataReceived) {
    // GPS is off, and we have GPS data (including time synced)
    time_t sleepTime = getTimeUntilNextScheduledReading();

    Serial.print("Going to sleep for ");
    Serial.print(sleepTime);
    Serial.println(" seconds.");

    esp_sleep_enable_timer_wakeup(sleepTime * 1000000LL);
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

  // Also store in global variables for persistence
  lastLatitude = gpsData[0];
  lastLongitude = gpsData[1];

  // Update the battery percentage in gpsData[2]
  gpsData[2] = PMU->getBatteryPercent();

  if (gps.time.isValid()) {
    // Convert UTC to AWST (UTC+8) and store it in gpsData[3]
    int hour = gps.time.hour() + 8;
    if (hour >= 24) hour -= 24;  // Adjust for next day if needed

    gpsData[3] = hour * 10000 + gps.time.minute() * 100 + gps.time.second();
  } else {
    gpsData[3] = 0;  // Set to 0 if time is not valid
  }

  // Note: We will compute and store the date in gpsData[4] in printGPSData()
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

  // Compute and store AWST date
  if (gps.date.isValid() && gps.time.isValid()) {
    // Create a tm struct with UTC date and time
    struct tm utcTime;
    utcTime.tm_year = gps.date.year() - 1900;  // tm_year is years since 1900
    utcTime.tm_mon = gps.date.month() - 1;     // tm_mon is 0-based
    utcTime.tm_mday = gps.date.day();
    utcTime.tm_hour = gps.time.hour();
    utcTime.tm_min = gps.time.minute();
    utcTime.tm_sec = gps.time.second();
    utcTime.tm_isdst = 0;

    // Convert to time_t
    time_t utcTimestamp = mktime(&utcTime);

    // Add 8 hours for AWST
    time_t awstTimestamp = utcTimestamp + 8 * 3600;

    // Convert back to struct tm
    struct tm awstTime;
    localtime_r(&awstTimestamp, &awstTime);

    // Now awstTime contains the adjusted date and time

    // Store the AWST date in gpsData[4] as YYYYMMDD
    int awstYear = awstTime.tm_year + 1900;
    int awstMonth = awstTime.tm_mon + 1;
    int awstDay = awstTime.tm_mday;
    gpsData[4] = awstYear * 10000 + awstMonth * 100 + awstDay;

    // Print the AWST date
    Serial.print("Date (YYYY-MM-DD) : ");
    Serial.printf("%04d-%02d-%02d\n", awstYear, awstMonth, awstDay);

  } else {
    gpsData[4] = 0;  // Set to 0 if date is not valid
    Serial.println("Invalid GPS date/time");
  }

  Serial.println("**********************");
}

void syncTimeWithRTC() {
  // This function syncs the GPS time to the ESP32's internal RTC

  // Create a tm struct with UTC date and time
  struct tm utcTime;
  utcTime.tm_year = gps.date.year() - 1900;  // tm_year is years since 1900
  utcTime.tm_mon = gps.date.month() - 1;     // tm_mon is 0-based
  utcTime.tm_mday = gps.date.day();
  utcTime.tm_hour = gps.time.hour();
  utcTime.tm_min = gps.time.minute();
  utcTime.tm_sec = gps.time.second();
  utcTime.tm_isdst = 0;

  // Convert to time_t
  time_t utcTimestamp = mktime(&utcTime);

  // Add 8 hours for AWST
  time_t awstTimestamp = utcTimestamp + 8 * 3600;

  // Convert back to struct tm
  struct tm awstTime;
  localtime_r(&awstTimestamp, &awstTime);

  // Set the system time to the AWST time
  struct timeval now = {.tv_sec = mktime(&awstTime)};
  settimeofday(&now, NULL);  // Set the system time to the adjusted time

  Serial.println("Time synced with ESP32 RTC!");
  timeSynced = true;  // Set the flag to true
}

void printTimeFromRTC() {
  // This function prints the current time from the ESP32's RTC

  time_t now;
  struct tm timeinfo;

  time(&now);             // Get the current time from the RTC
  localtime_r(&now, &timeinfo);  // Convert to local time

  Serial.print("Current Time (from RTC): ");
  Serial.printf("%02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
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

  // Map the sensor readings to percentages (use global variables)
  percentageHumidity1 = map(sensorReading1, wet, dry, 100, 0);
  percentageHumidity2 = map(sensorReading2, wet, dry, 100, 0);
  percentageHumidity3 = map(sensorReading3, wet, dry, 100, 0);

  // Create a formatted string with the moisture values
  String moistureData = "Moisture at 10cm: " + String(percentageHumidity1) + "%" +
                        ", Moisture at 20cm: " + String(percentageHumidity2) + "%" +
                        ", Moisture at 30cm: " + String(percentageHumidity3) + "%";

  // Print the formatted string to the Serial Monitor
  Serial.println(moistureData);
  Serial.println("<------------------------->");
}

void createAndPrintPayload() {
  // Get the date and time from RTC
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  char current_date[11]; // "YYYY-MM-DD" + null terminator
  char current_time[9];  // "hh:mm:ss" + null terminator
  snprintf(current_date, sizeof(current_date), "%04d-%02d-%02d",
           timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
  snprintf(current_time, sizeof(current_time), "%02d:%02d:%02d",
           timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  // Get battery percentage
  int battery_percentage = PMU->getBatteryPercent();

  // Get latitude and longitude
  char lat_lon[25];
  snprintf(lat_lon, sizeof(lat_lon), "%.6f, %.6f", lastLatitude, lastLongitude);

  // Create the payload string
  String payload = "[\"";
  payload += sensor_board_id;
  payload += "\", \"";
  payload += current_date;
  payload += "\", \"";
  payload += current_time;
  payload += "\", \"";
  payload += lat_lon;
  payload += "\", ";
  payload += battery_percentage;
  payload += ", ";
  payload += depth_1;
  payload += ", ";
  payload += depth_2;
  payload += ", ";
  payload += depth_3;
  payload += ", ";
  payload += percentageHumidity1;
  payload += ", ";
  payload += percentageHumidity2;
  payload += ", ";
  payload += percentageHumidity3;
  payload += "]";

  // Print the payload to Serial Monitor
  Serial.print("Payload: ");
  Serial.println(payload);
  initializeLoRa();

  Serial.println("Sending payload over LoRa...");
  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket();
  Serial.println("Sent payload succesfully");

}

bool isScheduledTime() {
  // Get current time
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  // Convert current time to minutes since midnight
  int currentMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;

  // Check if current time matches any scheduled time
  for (int i = 0; i < sizeof(scheduledTimes) / sizeof(scheduledTimes[0]); i++) {
    if (currentMinutes == scheduledTimes[i]) {
      return true;
    }
  }

  return false;
}

time_t getTimeUntilNextScheduledReading() {
  // Get current time
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  // Convert current time to minutes since midnight
  int currentMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;

  // Find the next scheduled time
  int minutesUntilNext = 0;
  bool found = false;
  for (int i = 0; i < sizeof(scheduledTimes) / sizeof(scheduledTimes[0]); i++) {
    if (scheduledTimes[i] > currentMinutes) {
      minutesUntilNext = scheduledTimes[i] - currentMinutes;
      found = true;
      break;
    }
  }

  // If no future scheduled time today, calculate time until the first scheduled time tomorrow
  if (!found) {
    minutesUntilNext = (24 * 60 - currentMinutes) + scheduledTimes[0];
  }

  // Calculate seconds until next scheduled time
  time_t secondsUntilNext = minutesUntilNext * 60;

  // Check if it's time to resync time via GPS (once a day)
  time_t timeSinceLastSync = time(NULL) - lastSyncTime;
  if (timeSinceLastSync >= syncInterval) {
    // Resync time via GPS
    gpsOn = true;
    gpsDataReceived = false;
    Serial.println("Time to resync time via GPS.");
    // Set sleep time to 10 seconds to allow GPS to acquire data
    return 10;
  }

  return secondsUntilNext;
}



void initializeLoRa() {
  #ifdef  RADIO_TCXO_ENABLE
    pinMode(RADIO_TCXO_ENABLE, OUTPUT);
    digitalWrite(RADIO_TCXO_ENABLE, HIGH);
  #endif

  Serial.println("Initializing LoRa....");

  LoRa.setPins(RADIO_CS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN);
  if (!LoRa.begin(CONFIG_RADIO_FREQ * 1000000)) 
  {
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
  Serial.println("LoRa Initialized succesfully");
}