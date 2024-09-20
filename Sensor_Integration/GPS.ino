#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <axp20x.h>

TinyGPSPlus gps;
HardwareSerial GPS(1);
AXP20X_Class axp;

// Define GPIO pins used to control power to GPS and LoRa modules
const int gpsPowerPin1 = 12;
const int gpsPowerPin2 = 15;
const int loraPowerPin1 = 23;
const int loraPowerPin2 = 26;
const int loraPowerPin3 = 27;

void setup() {
    Serial.begin(115200);
    GPS.begin(9600, SERIAL_8N1, 34, 12); // GPS uses TX=12, RX=34

    // Initialize power management
    axp.begin(Wire, AXP192_SLAVE_ADDRESS);
    axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
    axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
    axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
    axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
    axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);

    // Set power control GPIOs as OUTPUT
    pinMode(gpsPowerPin1, OUTPUT);
    pinMode(gpsPowerPin2, OUTPUT);
    pinMode(loraPowerPin1, OUTPUT);
    pinMode(loraPowerPin2, OUTPUT);
    pinMode(loraPowerPin3, OUTPUT);

    // Turn on GPS and LoRa modules
    turnOnGPS();
    turnOnLoRa();
    
    if (!waitForGPSLock(30000)) { // Wait up to 30 seconds for GPS lock
        Serial.println("Failed to get GPS lock.");
    }
}

void loop() {
    // Print GPS data if available
    if (gps.location.isValid()) {
        printGPSData();
    } else {
        Serial.println("No valid GPS data.");
    }

    // Simulate task and then go to sleep
    Serial.println("Going to sleep...");
    turnOffGPS();
    delay(2000); // Wait a bit before testing for post-off lock
    checkForPostOffLock();

    turnOffLoRa();
    esp_sleep_enable_timer_wakeup(30 * 1000000); // Sleep for 30 seconds
    esp_deep_sleep_start();
}

bool waitForGPSLock(unsigned long timeout) {
    unsigned long startTime = millis();
    while (millis() - startTime < timeout) {
        while (GPS.available()) {
            char c = GPS.read();
            if (gps.encode(c) && gps.satellites.value() > 0) {
                Serial.println("GPS lock acquired!");
                return true;
            }
        }
    }
    return false;
}

void printGPSData() {
    Serial.print("Latitude  : "); Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude : "); Serial.println(gps.location.lng(), 6);
    Serial.print("Satellites: "); Serial.println(gps.satellites.value());
    Serial.print("Altitude  : "); Serial.println(gps.altitude.meters());
    Serial.print("Time      : "); Serial.print(gps.time.hour()); Serial.print(":"); Serial.print(gps.time.minute()); Serial.print(":"); Serial.println(gps.time.second());
    Serial.print("Speed     : "); Serial.println(gps.speed.kmph());
    Serial.println("**********************");
}

void turnOnGPS() {
    digitalWrite(gpsPowerPin1, HIGH);
    digitalWrite(gpsPowerPin2, HIGH);
    Serial.println("GPS module powered ON.");
}

void turnOffGPS() {
    digitalWrite(gpsPowerPin1, LOW);
    digitalWrite(gpsPowerPin2, LOW);
    Serial.print("GPS Power Pin 1 State: "); Serial.println(digitalRead(gpsPowerPin1));
    Serial.print("GPS Power Pin 2 State: "); Serial.println(digitalRead(gpsPowerPin2));
    Serial.println("GPS module powered OFF.");
    axp.setPowerOutPut(AXP192_LDO2, AXP202_OFF);
    axp.setPowerOutPut(AXP192_LDO3, AXP202_OFF);
    axp.setPowerOutPut(AXP192_DCDC2, AXP202_OFF);
    axp.setPowerOutPut(AXP192_EXTEN, AXP202_OFF);
    axp.setPowerOutPut(AXP192_DCDC1, AXP202_OFF);
    gps = TinyGPSPlus(); // Reset the GPS parser to clear old data
    
}

void turnOnLoRa() {
    digitalWrite(loraPowerPin1, HIGH);
    digitalWrite(loraPowerPin2, HIGH);
    digitalWrite(loraPowerPin3, HIGH);
    Serial.println("LoRa module powered ON.");
}

void turnOffLoRa() {
    digitalWrite(loraPowerPin1, LOW);
    digitalWrite(loraPowerPin2, LOW);
    digitalWrite(loraPowerPin3, LOW);
    Serial.println("LoRa module powered OFF.");
}

void checkForPostOffLock() {
    Serial.println("Checking for GPS lock after power off...");
    if (waitForGPSLock(30000)) {
        Serial.println("GPS lock acquired after power off - Check power control.");
    } else {
        Serial.println("No GPS lock after power off - GPS is correctly powered off.");
    }
}
