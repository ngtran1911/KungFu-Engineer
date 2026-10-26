#include <LiquidCrystal.h>

#define Motor_forward    0
#define Motor_backward   1
#define Motor_L_dir_pin  7
#define Motor_R_dir_pin  8
#define Motor_L_pwm_pin  9
#define Motor_R_pwm_pin  10

#define ENCA_LEFT        3   // Interrupt pin for left encoder
#define ENCA_RIGHT       2   // Interrupt pin for right encoder

// Set LCD pins matching your Exercise 4 configuration
LiquidCrystal lcd(37, 36, 35, 34, 33, 32);

// Exercise 5 parameters
const float TARGET_CM = 50.0;       // Target distance in cm[cite: 6]
const float PULSES_PER_CM = 19.2;   // Replace with your calculated pulses/cm value from Ex 4

const unsigned long TARGET_PULSES = (unsigned long)(TARGET_CM * PULSES_PER_CM);
const int SPEED = 128;               // Motor speed (~50%)

volatile unsigned long pulses_left = 0;
volatile unsigned long pulses_right = 0;

void isrEncoderLeft()  { pulses_left++; }
void isrEncoderRight() { pulses_right++; }

void setMotor(int pwmPin, int dirPin, int speed) {
  digitalWrite(dirPin, speed >= 0 ? Motor_forward : Motor_backward);
  analogWrite(pwmPin, abs(speed));
}

void setMotors(int speed) {
  setMotor(Motor_L_pwm_pin, Motor_L_dir_pin, speed);
  setMotor(Motor_R_pwm_pin, Motor_R_dir_pin, speed);
}

void setup() {
  Serial.begin(9600);
  
  // Initialize LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Ex5: Distance");

  // Motor pin modes
  pinMode(Motor_L_dir_pin, OUTPUT); 
  pinMode(Motor_R_dir_pin, OUTPUT);
  pinMode(Motor_L_pwm_pin, OUTPUT); 
  pinMode(Motor_R_pwm_pin, OUTPUT);

  // Encoder pin modes and ISR attachments
  pinMode(ENCA_LEFT, INPUT_PULLUP);
  pinMode(ENCA_RIGHT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCA_LEFT), isrEncoderLeft, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCA_RIGHT), isrEncoderRight, RISING);

  setMotors(0);
  delay(2000); // Initial delay before starting

  // Reset counters and drive forward
  noInterrupts();
  pulses_left = 0;
  pulses_right = 0;
  interrupts();

  setMotors(SPEED);

  // Drive until average pulses reach target pulse count
  while (true) {
    noInterrupts();
    unsigned long pL = pulses_left;
    unsigned long pR = pulses_right;
    interrupts();

    // Check if target reached (using average or left encoder)
    if ((pL + pR) / 2 >= TARGET_PULSES) {
      break;
    }
  }

  // Stop motors
  setMotors(0);
  delay(300); // Allow vehicle to settle

  // Read final count safely
  noInterrupts();
  unsigned long final_left = pulses_left;
  unsigned long final_right = pulses_right;
  interrupts();

  // Print results to LCD[cite: 6]
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("L:"); lcd.print(final_left);
  lcd.print(" R:"); lcd.print(final_right);

  lcd.setCursor(0, 1);
  lcd.print("Dist: "); 
  lcd.print((final_left / PULSES_PER_CM), 1);
  lcd.print("cm");

  // Serial debug print
  Serial.print("Target cm: "); Serial.println(TARGET_CM);
  Serial.print("Left Pulses: "); Serial.println(final_left);
  Serial.print("Right Pulses: "); Serial.println(final_right);
}

void loop() {
  // Task completes in setup()
}
