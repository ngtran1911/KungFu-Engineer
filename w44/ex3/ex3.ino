#include <Wire.h>
#include <LiquidCrystal.h>

// LCD: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(37, 36, 35, 34, 33, 32);

const byte COMPASS_ADDRESS = 0x60;
const byte BEARING_REGISTER = 1;   // 8-bit bearing (0-255)

const char* directionName(float deg) {
  if (deg >= 337.5 || deg < 22.5)  return "N";
  if (deg < 67.5)                  return "NE";
  if (deg < 112.5)                 return "E";
  if (deg < 157.5)                 return "SE";
  if (deg < 202.5)                 return "S";
  if (deg < 247.5)                 return "SW";
  if (deg < 292.5)                 return "W";
  return "NW";
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  lcd.begin(16, 2);
  lcd.print("Compass ready");
  delay(1000);
  lcd.clear();
}

void loop() {
  Wire.beginTransmission(COMPASS_ADDRESS);
  Wire.write(BEARING_REGISTER);
  Wire.endTransmission();
  Wire.requestFrom(COMPASS_ADDRESS, (byte)1);

  if (Wire.available() >= 1) {
    byte bearing = Wire.read();
    float degrees = bearing * 360.0 / 256.0;

    lcd.setCursor(0, 0);
    lcd.print("Deg: ");
    lcd.print((int)degrees);
    lcd.print("    ");

    lcd.setCursor(0, 1);
    lcd.print("Dir: ");
    lcd.print(directionName(degrees));
    lcd.print("    ");

    Serial.print(bearing);
    Serial.print(" -> ");
    Serial.print(degrees);
    Serial.print(" deg, ");
    Serial.println(directionName(degrees));
  } else {
    lcd.setCursor(0, 0);
    lcd.print("No compass     ");
  }

  delay(200);
}