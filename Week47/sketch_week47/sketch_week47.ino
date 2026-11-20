 
#include <Wire.h>
#include <stdlib.h>
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
 
// The compass reports 256 counts per revolution.
const int COMPASS_COUNTS = 256;
const unsigned long TURN_TIMEOUT_MS = 15000;
const byte TURN_QUEUE_SIZE = 8;
int pendingTurns[TURN_QUEUE_SIZE];
byte nextTurn = 0;
byte pendingCount = 0;

volatile unsigned long pulses = 0;

bool turning = false;
int previousBearing;
int turnProgress;
int targetCounts;
int turnDirection;
unsigned long turnStarted;

void countPulse();
void driveDistance(int cm);
int readBearing();
void startTurn(int degrees);
void updateTurn();
void cancelTurns();
void turn_left();
void turn_right();
void stop_car();
void processCommand(String message);
 
 
void setup() {
 
  Serial.begin(9600);
  Wire.begin();
  Wire.setWireTimeout(25000, true);  // 25 ms, reset I²C hardware on timeout
 
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
  Serial.println("degree:10");
  Serial.println("degree:-90");
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
 
  if (turning) {
    updateTurn();
  } else if (pendingCount > 0) {
    int degrees = pendingTurns[nextTurn];
    nextTurn = (nextTurn + 1) % TURN_QUEUE_SIZE;
    pendingCount--;
    startTurn(degrees);
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
    cancelTurns();
 
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
 
    driveDistance(cm);
  }
 
 
  // RELATIVE HEADING COMMAND
  else if (message.startsWith("degree:")) {
    lcd.print(message);

    String value = message.substring(7);
    value.trim();
    const char *start = value.c_str();
    char *end;
    long degrees = strtol(start, &end, 10);

    if (end == start || *end != '\0' || degrees < -360 || degrees > 360) {
      cancelTurns();
      Serial.println("Enter a whole number from -360 to 360.");
      return;
    }

    if (pendingCount == TURN_QUEUE_SIZE) {
      Serial.println("Turn queue is full.");
      return;
    }

    pendingTurns[(nextTurn + pendingCount) % TURN_QUEUE_SIZE] = degrees;
    pendingCount++;
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
 
    cancelTurns();
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
 
 
int readBearing() {
  Wire.beginTransmission(COMPASS_ADDRESS);
  Wire.write(BEARING_REGISTER);

  if (Wire.endTransmission() != 0 ||
      Wire.requestFrom(COMPASS_ADDRESS, 1) != 1) {
    return -1;
  }
  return Wire.read();
}

void cancelTurns() {
  stop_car();
  turning = false;
  pendingCount = 0;
}

void startTurn(int degrees) {
  if (degrees == 0) {
    Serial.println("Turn completed: 0 degrees.");
    return;
  }

  previousBearing = readBearing();
  if (previousBearing < 0) {
    cancelTurns();
    Serial.println("Compass reading failed!");
    return;
  }

  targetCounts = (abs(degrees) * 256L + 180) / 360;
  turnDirection = degrees > 0 ? 1 : -1;
  turnProgress = 0;
  turnStarted = millis();
  turning = true;

  if (turnDirection > 0) {
    turn_right();
  } else {
    turn_left();
  }
}

void updateTurn() {
  if (millis() - turnStarted >= TURN_TIMEOUT_MS) {
    cancelTurns();
    Serial.println("Turn timed out.");
    return;
  }

  int bearing = readBearing();
  if (bearing < 0) {
    cancelTurns();
    Serial.println("Compass reading failed!");
    return;
  }

  // Unwrap each small change across north before adding it to the turn.
  int change = (bearing - previousBearing + 384) % COMPASS_COUNTS - 128;
  previousBearing = bearing;
  turnProgress += change * turnDirection;

  if (turnProgress >= targetCounts) {
    stop_car();
    turning = false;
    Serial.println("Turn completed.");
    lcd.setCursor(0, 1);
    lcd.print("Turn completed  ");
  }
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
 
