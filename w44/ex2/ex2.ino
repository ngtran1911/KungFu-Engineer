#include <Wire.h>

const byte COMPASS_ADDRESS = 0x60;  // 7-bit I2C address (check your slides/label)
const byte BEARING_REGISTER = 1;    // register 1 = 8-bit bearing (0-255)

void setup() {
  Serial.begin(9600);
  Wire.begin();   // start I2C as master
}

void loop() {
  // 1. Tell the compass which register we want
  Wire.beginTransmission(COMPASS_ADDRESS);
  Wire.write(BEARING_REGISTER);
  Wire.endTransmission();

  // 2. Ask for 1 byte back
  Wire.requestFrom(COMPASS_ADDRESS, (byte)1);

  // 3. Read it and print it
  if (Wire.available() >= 1) {
    byte bearing = Wire.read();
    Serial.print("Bearing: ");
    Serial.println(bearing);
  } else {
    Serial.println("No response from compass");
  }

  delay(200);
}