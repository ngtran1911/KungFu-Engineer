#include <LiquidCrystal.h>

// LCD: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(37, 36, 35, 34, 33, 32);

// Joystick
const int joystickX = A5;
const int joystickY = A6;
const int joystickButton = 19;

//Motor 
#define Motor_forward    0
#define Motor_return     1
#define Motor_L_dir_pin  7
#define Motor_R_dir_pin  8
#define Motor_L_pwm_pin  9
#define Motor_R_pwm_pin  10

// Variables used by the interrupt
volatile bool buttonPressed = false;
volatile unsigned long lastDebounceTime = 0;

// Display mode control (only used in loop, not in ISR)
bool showButtonCount = false;

void setup() {
  Serial.begin(9600);

  lcd.begin(16, 2);

  pinMode(joystickButton, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(joystickButton), buttonISR, FALLING);
  
  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);
  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);
}

void loop() {

  // Check if the button was pressed
  if (buttonPressed) {

    // Reset the flag
    buttonPressed = false;

    // Clear LCD before changing display
    runMotorTest();
    lcd.clear();

    // Print once, right when the press is registered
    Serial.print("Push counter: ");
   
  }

    // -----------------------------
    // Display joystick X/Y values
    // -----------------------------
    int xValue = analogRead(joystickX);
    int yValue = analogRead(joystickY);

    int xPercent = (xValue * 100L) / 1023;
    int yPercent = (yValue * 100L) / 1023;

    lcd.setCursor(0, 0);
    lcd.print("Y:");
    lcd.print(yValue);
    lcd.print(" ");
    lcd.print(yPercent);
    lcd.print("%   ");

    lcd.setCursor(0, 1);
    lcd.print("X:");
    lcd.print(xValue);
    lcd.print(" ");
    lcd.print(xPercent);
    lcd.print("%   ");

    delay(100);
}

// -----------------------------
// ISR: keep it short, no delay(), no lcd.print() here
// -----------------------------
void buttonISR() {
  unsigned long currentTime = millis();

  if (currentTime - lastDebounceTime > 200) {
    buttonPressed = true;
    lastDebounceTime = currentTime;
  }
}
void runMotorTest() {
  digitalWrite(Motor_R_dir_pin, Motor_return);
  digitalWrite(Motor_L_dir_pin, Motor_return);

  for (int pwm = 150; pwm > 50; pwm--) {
    analogWrite(Motor_L_pwm_pin, pwm);
    analogWrite(Motor_R_pwm_pin, pwm);
    delay(50);
  }

  digitalWrite(Motor_R_dir_pin, Motor_forward);
  digitalWrite(Motor_L_dir_pin, Motor_forward);

  for (int pwm = 50; pwm < 150; pwm++) {
    analogWrite(Motor_L_pwm_pin, pwm);
    analogWrite(Motor_R_pwm_pin, pwm);
    delay(50);
  }

  analogWrite(Motor_L_pwm_pin, 0);
    analogWrite(Motor_R_pwm_pin, 0);

    Serial.println("Motor test finished");
}
