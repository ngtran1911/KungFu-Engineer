
#include <Wire.h>
#include <LiquidCrystal.h>

#define COMPASS_ADDRESS 0x60
#define BEARING_REGISTER 1

#define Motor_forward  0
#define Motor_backward 1

#define Motor_L_dir_pin  7
#define Motor_R_dir_pin  8
#define Motor_L_pwm_pin  9
#define Motor_R_pwm_pin  10

int wanted_heading = 90;
const int tolerance = 4;
const int turn_speed = 100;

LiquidCrystal lcd(37, 36, 35, 34, 33, 32);

bool heading_received = false;

void turn_left();
void turn_right();
void stop_car();
void find_heading(int given_degree);

void setup() {
  Wire.begin();
  Serial.begin(9600);

  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);
  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Waiting command");

  stop_car();

  Serial.println("Enter command: degree:90");
}

void loop() {

  // SERIAL RECEPTION
  if (Serial.available() > 0) {

    String message = Serial.readStringUntil('\n');
    message.trim();

    Serial.print("Command received: ");
    Serial.println(message);

    // Display entire command on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(message);

    // Check degree command
    if (message.startsWith("degree:")) {

      int colon_pos = message.indexOf(':');
      int new_degree = message.substring(colon_pos + 1).toInt();

      if (new_degree >= 0 && new_degree <= 359) {

        wanted_heading = new_degree;
        heading_received = true;

        Serial.print("Target heading: ");
        Serial.println(wanted_heading);

      } else {
        Serial.println("Invalid degree!");
        stop_car();
        heading_received = false;
      }

    } else {
      Serial.println("Invalid command!");
      stop_car();
      heading_received = false;
    }
  }

  // FIND THE REQUESTED HEADING
  if (heading_received) {
    find_heading(wanted_heading);
  }
}

void find_heading(int given_degree) {

  Wire.beginTransmission(COMPASS_ADDRESS);
  Wire.write(BEARING_REGISTER);

  if (Wire.endTransmission() != 0) {
    stop_car();
    return;
  }

  if (Wire.requestFrom(COMPASS_ADDRESS, 1) != 1) {
    stop_car();
    return;
  }

  byte raw = Wire.read();

  int current_heading = (raw * 360L) / 256;

  int error = (given_degree - current_heading + 540) % 360 - 180;

  Serial.print("Heading: ");
  Serial.print(current_heading);
  Serial.print(" Target: ");
  Serial.print(given_degree);
  Serial.print(" Error: ");
  Serial.println(error);

  if (abs(error) <= tolerance) {
    stop_car();
    heading_received = false;

    Serial.println("Target heading reached!");

    lcd.setCursor(0, 1);
    lcd.print("Heading reached");

  }
  else if (error > 0) {
    turn_right();
  }
  else {
    turn_left();
  }

  delay(20);
}

void turn_left() {

  digitalWrite(Motor_L_dir_pin, Motor_backward);
  digitalWrite(Motor_R_dir_pin, Motor_forward);

  analogWrite(Motor_L_pwm_pin, turn_speed);
  analogWrite(Motor_R_pwm_pin, turn_speed);
}

void turn_right() {

  digitalWrite(Motor_L_dir_pin, Motor_forward);
  digitalWrite(Motor_R_dir_pin, Motor_backward);

  analogWrite(Motor_L_pwm_pin, turn_speed);
  analogWrite(Motor_R_pwm_pin, turn_speed);
}

void stop_car() {

  analogWrite(Motor_L_pwm_pin, 0);
  analogWrite(Motor_R_pwm_pin, 0);
}
