#include <TinyGPS++.h>
#include <HardwareSerial.h>

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

// Array to store GPS and battery data
float gpsData[4]; // [0] = Latitude, [1] = Longitude, [2] = Battery percentage, [3] = Time (in hhmmss format)

void setup() {
  Serial.begin(115200);
  delay(5000);
  
  pinMode(buttonPin, INPUT_PULLUP);    // Use internal pull-up resistor for the button
  pinMode(ledPin, OUTPUT);             // Set external LED pin as output

  GPS.begin(9600, SERIAL_8N1, 34, 12); // GPS uses TX=12, RX=34
  
  // Turn on the LED initially
  digitalWrite(ledPin, HIGH);

  // GPS is on by default
  Serial.println("GPS module powered ON.");
  printGPSData();
}

void loop() {
  // Read the button state
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
    }
  }
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
  
  // Assuming battery percentage is a placeholder (replace with actual code to read battery)
  gpsData[2] = 75.0; // Placeholder for battery percentage

  if (gps.time.isValid()) {
    // Store time as hhmmss (example: 145530 for 14:55:30)
    gpsData[3] = gps.time.hour() * 10000 + gps.time.minute() * 100 + gps.time.second();
  } else {
    gpsData[3] = 0; // Set to 0 if time is not valid
  }
}

void printGPSData() {
  // Print the stored data from the array
  Serial.print("Latitude  : "); Serial.println(gpsData[0], 6);
  Serial.print("Longitude : "); Serial.println(gpsData[1], 6);
  Serial.print("Battery % : "); Serial.println(gpsData[2]);
  Serial.print("Time (hhmmss) : "); Serial.println(gpsData[3]);

  Serial.print("Satellites: "); Serial.println(gps.satellites.value());
  Serial.print("Altitude  : "); Serial.println(gps.altitude.meters());
  
  Serial.print("Speed     : "); Serial.println(gps.speed.kmph());
  Serial.println("**********************");
}
