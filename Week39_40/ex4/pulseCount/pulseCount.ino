const int encRmotorA = 2;
const int encLmotorA = 3;

#define Motor_forward    0
#define Motor_backward   1
#define Motor_L_dir_pin  7
#define Motor_R_dir_pin  8
#define Motor_L_pwm_pin  9
#define Motor_R_pwm_pin  10

volatile int pulseCount = 0;

void setup() {
  Serial.begin(9600);

  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);
  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);

  pinMode(encRmotorA, INPUT);
  attachInterrupt(digitalPinToInterrupt(encRmotorA), countPulse, RISING);

  // Give the board time to settle after upload/reset,
  // and give yourself time to position the car before it drives.
  Serial.println("Starting in 3 seconds...");
  delay(3000);

  runDistanceTest();
}

void loop() {
  // nothing needed here — the test already ran once in setup()
}

void runDistanceTest() {
  pulseCount = 0;  // reset counter before starting

  unsigned long startTime = millis();

  // Start driving forward
  digitalWrite(Motor_L_dir_pin, Motor_forward);
  digitalWrite(Motor_R_dir_pin, Motor_backward);  // flipped due to mirrored mounting
  analogWrite(Motor_L_pwm_pin, 200);
  analogWrite(Motor_R_pwm_pin, 200);

  // Wait until 4 seconds have passed
  while (millis() - startTime < 4000) {
    // do nothing, just wait — pulseCount updates itself via the ISR
  }

  // Stop the motors
  analogWrite(Motor_L_pwm_pin, 0);
  analogWrite(Motor_R_pwm_pin, 0);

  // Report results
  Serial.print("Pulses counted: ");
  Serial.println(pulseCount);
}

void countPulse() {
  pulseCount = pulseCount + 1;
}