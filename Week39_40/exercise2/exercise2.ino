#include <LiquidCrystal.h>

// LCD: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(37, 36, 35, 34, 33, 32);

// Joystick
const int joystickX = A5;
const int joystickY = A6;
const int joystickButton = 19;

// Motor
#define Motor_forward    0
#define Motor_return     1
#define Motor_L_dir_pin  7
#define Motor_R_dir_pin  8
#define Motor_L_pwm_pin  9
#define Motor_R_pwm_pin  10

// Joystick center values
const int X_CENTER = 49;
const int Y_CENTER = 50;

// Dead zone around joystick center
const int DEAD_ZONE = 5;

void setup() {
  Serial.begin(9600);

  lcd.begin(16, 2);

  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);
  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);

  // Start stopped
  analogWrite(Motor_L_pwm_pin, 0);
  analogWrite(Motor_R_pwm_pin, 0);
}

void loop() {

  // Read joystick
  int xValue = analogRead(joystickX);
  int yValue = analogRead(joystickY);

  // Convert to percentage
  int xPercent = (xValue * 100L) / 1023;
  int yPercent = (yValue * 100L) / 1023;

  // ------------------------------------------------
  // X = forward/backward
  // Y = left/right
  // ------------------------------------------------

  // Calculate speed from X
  int speed = 0;

  if (xPercent < X_CENTER - DEAD_ZONE) {
    // Forward
    speed = map(xPercent,
                0,
                X_CENTER - DEAD_ZONE,
                255,
                0);

  } else if (xPercent > X_CENTER + DEAD_ZONE) {
    // Backward
    speed = map(xPercent,
                X_CENTER + DEAD_ZONE,
                100,
                0,
                255);
  }

  // Calculate turning
  int turn = 0;

  if (yPercent < Y_CENTER - DEAD_ZONE) {
    // Turn right
    turn = map(yPercent,
               0,
               Y_CENTER - DEAD_ZONE,
               255,
               0);

  } else if (yPercent > Y_CENTER + DEAD_ZONE) {
    // Turn left
    turn = map(yPercent,
               Y_CENTER + DEAD_ZONE,
               100,
               0,
               255);
  }

  // ------------------------------------------------
  // Calculate individual motor speeds
  // ------------------------------------------------

  int leftMotor = speed;
  int rightMotor = speed;

  // Turning
  if (turn > 0) {

    if (yPercent < Y_CENTER) {
      // Right turn
      leftMotor = speed + turn;
      rightMotor = speed - turn;
    } else {
      // Left turn
      leftMotor = speed - turn;
      rightMotor = speed + turn;
    }
  }

  // Limit motor values
  leftMotor = constrain(leftMotor, 0, 255);
  rightMotor = constrain(rightMotor, 0, 255);

  // ------------------------------------------------
  // Direction
  // ------------------------------------------------

  if (speed > 0) {

    if (xPercent < X_CENTER) {
      // Forward
      digitalWrite(Motor_L_dir_pin, Motor_forward);
      digitalWrite(Motor_R_dir_pin, Motor_forward);

    } else {
      // Backward
      digitalWrite(Motor_L_dir_pin, Motor_return);
      digitalWrite(Motor_R_dir_pin, Motor_return);
    }

  } else {
    // Stop
    leftMotor = 0;
    rightMotor = 0;
  }

  // Write motor speeds
  analogWrite(Motor_L_pwm_pin, leftMotor);
  analogWrite(Motor_R_pwm_pin, rightMotor);

  // ------------------------------------------------
  // LCD
  // ------------------------------------------------

  lcd.setCursor(0, 0);
  lcd.print("X:");
  lcd.print(xPercent);
  lcd.print("%   ");

  lcd.setCursor(0, 1);
  lcd.print("Y:");
  lcd.print(yPercent);
  lcd.print("%   ");

  delay(50);
}

