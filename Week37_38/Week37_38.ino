#include <LiquidCrystal.h>

// LCD: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(37, 36, 35, 34, 33, 32);

// Joystick
const int joystickX = A8;
const int joystickY = A9;

void setup() {
  lcd.begin(16, 2);
  lcd.clear();
}

void loop() {
  // Read analog values (0-1023)
  int xValue = analogRead(joystickX);
  int yValue = analogRead(joystickY);

  // Convert to percentage of maximum value
  int xPercent = (xValue * 100L) / 1023;
  int yPercent = (yValue * 100L) / 1023;

  // First row: Y value
  lcd.setCursor(0, 0);
  lcd.print("Y:");
  lcd.print(yValue);
  lcd.print(" ");
  lcd.print(yPercent);
  lcd.print("%   ");

  // Second row: X value
  lcd.setCursor(0, 1);
  lcd.print("X:");
  lcd.print(xValue);
  lcd.print(" ");
  lcd.print(xPercent);
  lcd.print("%   ");

  delay(100);
}