#include <LiquidCrystal.h>

// LCD: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(37, 36, 35, 34, 33, 32);

#define Motor_forward    0
#define Motor_backward   1
#define Motor_L_dir_pin  7
#define Motor_R_dir_pin  8
#define Motor_L_pwm_pin  9
#define Motor_R_pwm_pin  10

const int ENC_PIN = 2;                 // encoder signal (the one that works for you)
const float PULSES_PER_CM = 9.5;      // replace with your own calibration
const int DRIVE_SPEED = 128;

volatile unsigned long pulses = 0;

void countPulse() { pulses++; }

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);

  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);
  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);

  pinMode(ENC_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENC_PIN), countPulse, RISING);

  Serial.println("Type dist:<-20..20>, e.g. dist:15 or dist:-8");
}

void loop() {
  if (Serial.available() > 0) {
    String message = Serial.readStringUntil('\n');
    message.trim();

    if (message.startsWith("dist:")) {
      int cm = message.substring(5).toInt();   // number after "dist:"
      cm = constrain(cm, -20, 20);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(message);                      // entire command on the LCD

      driveDistance(cm);
    } else {
      Serial.println("Unknown command, use dist:<number>");
    }
  }
}

void driveDistance(int cm) {
  if (cm == 0) return;

  unsigned long target = (unsigned long)(abs(cm) * PULSES_PER_CM);
  int dir = (cm > 0) ? Motor_forward : Motor_backward;

  noInterrupts();
  pulses = 0;
  interrupts();

  digitalWrite(Motor_L_dir_pin, dir);
  digitalWrite(Motor_R_dir_pin, dir);
  analogWrite(Motor_L_pwm_pin, DRIVE_SPEED);
  analogWrite(Motor_R_pwm_pin, DRIVE_SPEED);

  unsigned long p = 0;
  while (p < target) {
    noInterrupts();
    p = pulses;
    interrupts();
  }

  analogWrite(Motor_L_pwm_pin, 0);
  analogWrite(Motor_R_pwm_pin, 0);

  Serial.print("Done, pulses: ");
  Serial.println(p);
}