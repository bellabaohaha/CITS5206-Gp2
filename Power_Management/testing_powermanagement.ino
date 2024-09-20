#include <TinyGPS++.h>
#include <axp20x.h>

TinyGPSPlus gps;
HardwareSerial GPS(1);
AXP20X_Class axp;


void setup()
{
  Serial.begin(115200);
  Wire.begin(21, 22);
  if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS)) {
    Serial.println("AXP192 Begin PASS");
  } else {
    Serial.println("AXP192 Begin FAIL");
  }
  axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
  axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
  GPS.begin(9600, SERIAL_8N1, 34, 12);   //17-TX 18-RX
}

void loop()
{
  updateGPS(); // Process GPS data

  // Calculate local time by adding 8 hours to UTC
  int localHour = gps.time.hour() + 8;
  if (localHour >= 24) {
    localHour -= 24; // Adjust for next day
  }

  Serial.print("Local Time: ");
  Serial.print(localHour);
  Serial.print(":");
  Serial.print(gps.time.minute(), DEC);
  Serial.print(":");
  Serial.println(gps.time.second(), DEC);
  Serial.println("**********************");

  smartDelay(1000);

  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println("No GPS data received: check wiring");
}

void updateGPS()
{
  while (GPS.available()) {
    char c = GPS.read();
    gps.encode(c);
  }
}

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do {
    while (GPS.available()) {
      gps.encode(GPS.read());
    }
  } while (millis() - start < ms);
}