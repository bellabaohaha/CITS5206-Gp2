#include <Wire.h>
// https://github.com/lewisxhe/AXP202X_Library
#include <axp20x.h>

AXP20X_Class axp;

void setup() {
  // ...
  Serial.begin(115200);
  delay(2000);
  Wire.begin(21, 22);
  if(axp.begin(Wire, AXP192_SLAVE_ADDRESS) == AXP_FAIL) {
    Serial.println(F("failed to initialize communication with AXP192"));
  }
}

void loop() {
  // ...
  Serial.println("Measuring battery voltage : ");
  Serial.print(axp.getBattVoltage()/1000.0);
  Serial.println(" mV");
  delay(5000);
}