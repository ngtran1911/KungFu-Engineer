#include <Wire.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(37, 36, 35, 34, 33, 32);

// ---- Hard-coded settings ----
const int TARGET_HEADING = 150  ;   // 0-360 degrees (90 = East)
const int TOLERANCE      = 4;    // 86-94 counts as "aligned"
const int TURN_SPEED     = 90;   // low speed so it doesn't overshoot

// ---- Compass ----
const byte COMPASS_ADDRESS  = 0x60;
const byte BEARING_REGISTER = 1;

// ---- Motors ----
#define Motor_L_dir_pin  7
#define Motor_R_dir_pin  8
#define Motor_L_pwm_pin  9
#define Motor_R_pwm_pin  10

// Your forward drive was L=0, R=1. So the same value on both wheels
// makes them spin opposite ways, which turns the car on the spot.
// If the car turns the wrong way, swap these two values (0 <-> 1).
const int CLOCKWISE_DIR = 0;
const int COUNTER_DIR   = 1;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  lcd.begin(16, 2);

  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);
  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);
  stopMotors();
}

void loop() {
  find_heading(TARGET_HEADING);   // runs every cycle, so it re-corrects if turned by hand
}

// Returns the heading in degrees (0-359), or -1 if the compass doesn't answer
int readHeading() {
  Wire.beginTransmission(COMPASS_ADDRESS);
  Wire.write(BEARING_REGISTER);
  Wire.endTransmission();
  Wire.requestFrom(COMPASS_ADDRESS, (byte)1);

  if (Wire.available() >= 1) {
    byte bearing = Wire.read();
    return (int)(bearing * 360L / 256);   // 360L avoids int overflow
  }
  return -1;
}

void find_heading(int given_degree) {
  int current = readHeading();

  if (current < 0) {          // no compass reading: don't drive blind
    stopMotors();
    return;
  }

  // Shortest signed error, from -180 to +180
  // positive = target is clockwise (turn right), negative = turn left
  int error = ((given_degree - current + 540) % 360) - 180;

  lcd.setCursor(0, 0);
  lcd.print("Now:");
  lcd.print(current);
  lcd.print(" Goal:");
  lcd.print(given_degree);
  lcd.print("  ");
  lcd.setCursor(0, 1);
  lcd.print("Err:");
  lcd.print(error);
  lcd.print("    ");

  if (abs(error) <= TOLERANCE) {
    stopMotors();                       // aligned
  } else if (error > 0) {
    spin(CLOCKWISE_DIR);                // turn right
  } else {
    spin(COUNTER_DIR);                  // turn left
  }

  delay(50);
}

void spin(int dir) {
  digitalWrite(Motor_L_dir_pin, dir);
  digitalWrite(Motor_R_dir_pin, 1 - dir);   // opposite of the left wheel
  analogWrite(Motor_L_pwm_pin, TURN_SPEED);
  analogWrite(Motor_R_pwm_pin, TURN_SPEED);
}

void stopMotors() {
  analogWrite(Motor_L_pwm_pin, 0);
  analogWrite(Motor_R_pwm_pin, 0);
}