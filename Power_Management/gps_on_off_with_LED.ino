#include <TinyGPS++.h>
#include <HardwareSerial.h>

TinyGPSPlus gps;
HardwareSerial GPS(1);

// Define pins for the button and LED
const int buttonPin = 38; // GPIO pin connected to the button
const int ledPin = 5;    // GPIO pin connected to the blue LED

// Variables to track GPS state, button state, and press count
bool gpsOn = true;  // GPS is on by default
int buttonState = 0;
int lastButtonState = 0;
int pressCount = 0;

void setup() {
  Serial.begin(115200);
  GPS.begin(9600, SERIAL_8N1, 34, 12); // GPS uses TX=12, RX=34
  pinMode(buttonPin, INPUT_PULLUP);    // Use internal pull-up resistor for the button
  pinMode(ledPin, OUTPUT);
  
  // Initialize GPS and LED
  turnOnGPS();           // GPS is on by default
}

void loop() {
  // Read the button state
  buttonState = digitalRead(buttonPin);

  // Check if the button was pressed
  if (buttonState == LOW && lastButtonState == HIGH) { // Button press detected
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

  // Store the button state for the next loop
  lastButtonState = buttonState;

  // If GPS is on, read and print GPS data
  if (gpsOn) {
    while (GPS.available()) { // Check if data is available
      gps.encode(GPS.read()); // Process GPS data
    }
    printGPSData(); // Print the GPS data
  }
}

void turnOnGPS() {
  // Code to turn on the GPS module, e.g., powering the GPS or sending commands
  Serial.println("GPS module powered ON.");
  digitalWrite(ledPin, HIGH); // Turn on LED to indicate GPS is on
}

void turnOffGPS() {
  // Code to turn off the GPS module to save power
  Serial.println("GPS module powered OFF.");
  digitalWrite(ledPin, LOW); // Turn off LED to indicate GPS is off
}

void printGPSData() {
  // Check if GPS data is valid before printing
  if (gps.location.isValid()) {
    Serial.print("Latitude  : "); Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude : "); Serial.println(gps.location.lng(), 6);
  } else {
    Serial.println("Location data not available.");
  }

  Serial.print("Satellites: "); Serial.println(gps.satellites.value());
  Serial.print("Altitude  : "); Serial.println(gps.altitude.meters());
  
  if (gps.time.isValid()) {
    Serial.print("Time      : "); 
    Serial.print(gps.time.hour()); Serial.print(":"); 
    Serial.print(gps.time.minute()); Serial.print(":"); 
    Serial.println(gps.time.second());
  } else {
    Serial.println("Time data not available.");
  }

  Serial.print("Speed     : "); Serial.println(gps.speed.kmph());
  Serial.println("**********************");
  smartDelay(2000);

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
