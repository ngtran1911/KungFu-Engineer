
#include <Wire.h>
#include <LiquidCrystal.h>

// LCD: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(37, 36, 35, 34, 33, 32);

// COMPASS
#define COMPASS_ADDRESS 0x60
#define BEARING_REGISTER 1

// MOTOR DIRECTIONS
#define Motor_forward    0
#define Motor_backward   1

// MOTOR PINS
#define Motor_L_dir_pin  7
#define Motor_R_dir_pin  8
#define Motor_L_pwm_pin  9
#define Motor_R_pwm_pin  10

// ENCODER
const int ENC_PIN = 2;
const float PULSES_PER_CM = 9.5;

// MOTOR SPEEDS
const int DRIVE_SPEED = 128;
const int TURN_SPEED = 100;

// HEADING SETTINGS
const int tolerance = 4;

volatile unsigned long pulses = 0;

int wanted_heading = 90;
bool heading_received = false;

// FUNCTION DECLARATIONS
void countPulse();
void driveDistance(int cm);
void find_heading(int given_degree);
void turn_left();
void turn_right();
void stop_car();
void processCommand(String message);


void setup() {

  Serial.begin(9600);
  Wire.begin();

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Waiting command");

  // MOTOR PIN SETUP
  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);
  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);

  // ENCODER SETUP
  pinMode(ENC_PIN, INPUT);

  attachInterrupt(
    digitalPinToInterrupt(ENC_PIN),
    countPulse,
    RISING
  );

  stop_car();

  Serial.println("Available commands:");
  Serial.println("drive:15");
  Serial.println("drive:-8");
  Serial.println("degree:90");
  Serial.println("lcd:Hello");
}


// MAIN LOOP
void loop() {

  // READ SERIAL COMMAND
  if (Serial.available() > 0) {

    String message = Serial.readStringUntil('\n');
    message.trim();

    Serial.print("Command received: ");
    Serial.println(message);

    processCommand(message);
  }

  // CONTINUOUSLY CHECK COMPASS HEADING
  if (heading_received) {
    find_heading(wanted_heading);
  }
}


// ENCODER INTERRUPT
void countPulse() {
  pulses++;
}


// PROCESS SERIAL COMMANDS
void processCommand(String message) {

  // DISPLAY COMMAND ON LCD
  lcd.clear();
  lcd.setCursor(0, 0);

  if (message.startsWith("drive:")) {

    lcd.print(message);

    String value = message.substring(6);

    if (value.length() == 0) {
      Serial.println("Invalid distance!");
      stop_car();
      return;
    }

    // VALIDATE NUMBER
    bool valid = true;

    int start = 0;

    if (value.charAt(0) == '-') {
      start = 1;
    }

    if (start == value.length()) {
      valid = false;
    }

    for (int i = start; i < value.length(); i++) {
      if (!isDigit(value.charAt(i))) {
        valid = false;
        break;
      }
    }

    if (!valid) {
      Serial.println("Invalid distance!");
      stop_car();
      return;
    }

    int cm = value.toInt();

    if (cm < -20 || cm > 20) {
      Serial.println("Distance must be between -20 and 20!");
      stop_car();
      return;
    }

    heading_received = false;

    driveDistance(cm);
  }


  // HEADING COMMAND
  else if (message.startsWith("degree:")) {

    lcd.print(message);

    String value = message.substring(7);

    bool valid = value.length() > 0;

    for (unsigned int i = 0; i < value.length(); i++) {
      if (!isDigit(value.charAt(i))) {
        valid = false;
        break;
      }
    }

    if (!valid) {
      Serial.println("Invalid degree!");
      stop_car();
      heading_received = false;
      return;
    }

    int new_degree = value.toInt();

    if (new_degree >= 0 && new_degree <= 359) {

      wanted_heading = new_degree;
      heading_received = true;

      Serial.print("Target heading: ");
      Serial.println(wanted_heading);

    } else {

      Serial.println("Degree must be between 0 and 359!");

      stop_car();
      heading_received = false;
    }
  }


  // LCD COMMAND
  else if (message.startsWith("lcd:")) {

    String display_text = message.substring(4);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(display_text);

    Serial.println("LCD updated");
  }


  // INVALID COMMAND
  else {

    Serial.println("Unknown command!");

    stop_car();
    heading_received = false;
  }
}


// DRIVE A SPECIFIC DISTANCE USING ENCODER
void driveDistance(int cm) {

  if (cm == 0) {
    stop_car();
    Serial.println("Distance is zero.");
    return;
  }

  unsigned long target =
    (unsigned long)(abs(cm) * PULSES_PER_CM);

  int dir;

  if (cm > 0) {
    dir = Motor_forward;
  } else {
    dir = Motor_backward;
  }

  // RESET ENCODER PULSES
  noInterrupts();
  pulses = 0;
  interrupts();

  // SET MOTOR DIRECTION
  digitalWrite(Motor_L_dir_pin, dir);
  digitalWrite(Motor_R_dir_pin, dir);

  // START MOTORS
  analogWrite(Motor_L_pwm_pin, DRIVE_SPEED);
  analogWrite(Motor_R_pwm_pin, DRIVE_SPEED);

  unsigned long p = 0;

  // WAIT UNTIL TARGET DISTANCE IS REACHED
  while (p < target) {

    noInterrupts();
    p = pulses;
    interrupts();
  }

  // STOP MOTORS
  stop_car();

  Serial.print("Distance completed. Pulses: ");
  Serial.println(p);

  lcd.setCursor(0, 1);
  lcd.print("Drive completed");
}


// FIND REQUESTED COMPASS HEADING
void find_heading(int given_degree) {

  Wire.beginTransmission(COMPASS_ADDRESS);
  Wire.write(BEARING_REGISTER);

  if (Wire.endTransmission() != 0) {
    stop_car();
    heading_received = false;
    Serial.println("Compass communication error!");
    return;
  }

  if (Wire.requestFrom(COMPASS_ADDRESS, 1) != 1) {
    stop_car();
    heading_received = false;
    Serial.println("Compass reading failed!");
    return;
  }

  byte raw = Wire.read();

  // CONVERT COMPASS VALUE TO DEGREES
  int current_heading = (raw * 360L) / 256;

  // CALCULATE SHORTEST TURNING DIRECTION
  int error =
    (given_degree - current_heading + 540) % 360 - 180;

  Serial.print("Current heading: ");
  Serial.print(current_heading);

  Serial.print(" Target: ");
  Serial.print(given_degree);

  Serial.print(" Error: ");
  Serial.println(error);


  // TARGET HEADING REACHED
  if (abs(error) <= tolerance) {

    stop_car();
    heading_received = false;

    Serial.println("Target heading reached!");

    lcd.setCursor(0, 1);
    lcd.print("Heading reached");
  }

  // TURN RIGHT
  else if (error > 0) {

    turn_right();
  }

  // TURN LEFT
  else {

    turn_left();
  }

  delay(20);
}


// TURN LEFT
void turn_left() {

  digitalWrite(Motor_L_dir_pin, Motor_backward);
  digitalWrite(Motor_R_dir_pin, Motor_forward);

  analogWrite(Motor_L_pwm_pin, TURN_SPEED);
  analogWrite(Motor_R_pwm_pin, TURN_SPEED);
}


// TURN RIGHT
void turn_right() {

  digitalWrite(Motor_L_dir_pin, Motor_forward);
  digitalWrite(Motor_R_dir_pin, Motor_backward);

  analogWrite(Motor_L_pwm_pin, TURN_SPEED);
  analogWrite(Motor_R_pwm_pin, TURN_SPEED);
}


// STOP BOTH MOTORS
void stop_car() {

  analogWrite(Motor_L_pwm_pin, 0);
  analogWrite(Motor_R_pwm_pin, 0);
}

