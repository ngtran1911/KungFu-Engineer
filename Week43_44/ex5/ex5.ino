#include <Wire.h>

// ----- Compass -----
#define COMPASS_ADDRESS 0x60
#define BEARING_REGISTER 1

// ----- Motor pins -----
#define Motor_forward    0
#define Motor_backward   1

#define Motor_L_dir_pin  7
#define Motor_R_dir_pin  8
#define Motor_L_pwm_pin  9
#define Motor_R_pwm_pin  10

// ----- Encoder pins -----
#define ENCB_LEFT  3
#define ENCB_RIGHT 2

// ----- Joystick button -----
// Change pin 11 if your joystick button uses another pin.
#define JOYSTICK_BUTTON_PIN 19

// ----- Calibration: MUST be measured -----
const float PULSES_PER_CM = 8.60;

// ----- Route settings -----
const int turn_angle = 110;
const int turn_tolerance = 3;
const int drive_tolerance = 3;

volatile unsigned long left_pulses = 0;
volatile unsigned long right_pulses = 0;

bool previous_button = HIGH;

// ---------- Encoder interrupts ----------
void left_encoder() {
  left_pulses++;
}

void right_encoder() {
  right_pulses++;
}

// ---------- Motor control ----------
void set_motors(int left_speed, int right_speed) {
  left_speed = constrain(left_speed, -255, 255);
  right_speed = constrain(right_speed, -255, 255);

  digitalWrite(Motor_L_dir_pin,
    left_speed >= 0 ? Motor_forward : Motor_backward);
  digitalWrite(Motor_R_dir_pin,
    right_speed >= 0 ? Motor_forward : Motor_backward);

  analogWrite(Motor_L_pwm_pin, abs(left_speed));
  analogWrite(Motor_R_pwm_pin, abs(right_speed));
}

void stop_car() {
  analogWrite(Motor_L_pwm_pin, 0);
  analogWrite(Motor_R_pwm_pin, 0);
}

void reset_encoders() {
  noInterrupts();
  left_pulses = 0;
  right_pulses = 0;
  interrupts();
}

unsigned long average_pulses() {
  unsigned long left, right;

  noInterrupts();
  left = left_pulses;
  right = right_pulses;
  interrupts();

  return (left + right) / 2;
}

// ---------- Compass ----------
bool read_heading(float &heading) {
  Wire.beginTransmission(COMPASS_ADDRESS);
  Wire.write(BEARING_REGISTER);

  if (Wire.endTransmission() != 0) {
    return false;
  }

  if (Wire.requestFrom(COMPASS_ADDRESS, 1) != 1) {
    return false;
  }

  byte raw = Wire.read();
  heading = raw * 360.0 / 256.0;

  return true;
}

// Signed shortest difference: target minus current
float heading_error(float target, float current) {
  float error = target - current;

  while (error >= 180) error -= 360;
  while (error < -180) error += 360;

  return error;
}

float normalize_heading(float heading) {
  while (heading >= 360) heading -= 360;
  while (heading < 0) heading += 360;
  return heading;
}

// ---------- Drive a measured distance ----------
// Maintains the initial heading using the compass.
void drive_cm(float distance_cm, int speed_percent) {
  float target_heading;

  if (!read_heading(target_heading)) {
    stop_car();
    return;
  }

  reset_encoders();

  unsigned long target_pulses =
      (unsigned long)(distance_cm * PULSES_PER_CM);

  int base_speed = 255L * speed_percent / 100;
  unsigned long last_pulses = 0;
  unsigned long last_progress_time = millis();

  while (average_pulses() < target_pulses) {
    float current_heading;

    if (!read_heading(current_heading)) {
      stop_car();
      return;
    }

    float error = heading_error(target_heading,
                                current_heading);

    // Compass correction. Tune the gain for your car.
    int correction = (int)(error * 2.0);
    correction = constrain(correction, -35, 35);

    int left_speed = base_speed + correction;
    int right_speed = base_speed - correction;

    set_motors(left_speed, right_speed);

    unsigned long pulses = average_pulses();

    if (pulses > last_pulses) {
      last_pulses = pulses;
      last_progress_time = millis();
    }

    // Stop if the wheels are not producing encoder pulses.
    if (millis() - last_progress_time > 2000) {
      stop_car();
      Serial.println("Encoder timeout!");
      return;
    }

    delay(10);
  }

  stop_car();
  delay(300);

  Serial.print("Completed distance: ");
  Serial.print(distance_cm);
  Serial.println(" cm");
}

// ---------- Turn left by a relative angle ----------
void turn_left(int angle) {
  float current_heading;

  if (!read_heading(current_heading)) {
    stop_car();
    return;
  }

  // Compass degrees increase clockwise.
  float target_heading =
      normalize_heading(current_heading - angle);

  unsigned long start_time = millis();

  while (true) {
    if (!read_heading(current_heading)) {
      stop_car();
      return;
    }

    float error = heading_error(target_heading,
                                current_heading);

    if (abs((int)error) <= turn_tolerance) {
      break;
    }

    // Positive error means target is clockwise:
    // rotate right. Negative error: rotate left.
    int turn_speed = abs((int)error) * 3; // Коэффициент 3 можно настроить
    turn_speed = constrain(turn_speed, 70, 120); // Ограничиваем скорость (мин 70 для уверенного сдвига)

    if (error < 0) {
      set_motors(-turn_speed, turn_speed);  // Влево
    } else {
      set_motors(turn_speed, -turn_speed);  // Вправо (коррекция перелета)
    }

    if (millis() - start_time > 8000) {
      stop_car();
      Serial.println("Turn timeout!");
      return;
    }

    delay(15);
  }

  stop_car();
  delay(400);

  Serial.print("Turn completed. Target: ");
  Serial.println(target_heading);
}

// ---------- Complete route ----------
void run_route() {
  float starting_heading;

  if (!read_heading(starting_heading)) {
    Serial.println("Compass error. Route cancelled.");
    stop_car();
    return;
  }

  Serial.println("Starting route");

  // 1. Drive 20 cm at 75% speed
  drive_cm(20, 75);

  // 2. Turn left 110 degrees
  turn_left(110);

  // 3. Drive 13 cm at 25% speed
  drive_cm(13, 25);

  // 4. Turn left another 110 degrees
  turn_left(110);

  // 5. Drive 20 cm at 50% speed
  drive_cm(20, 50);

  stop_car();
  Serial.println("Route finished");
}

// ---------- Setup ----------
void setup() {
  Wire.begin();
  Serial.begin(9600);

  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);
  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);

  pinMode(ENCB_LEFT, INPUT_PULLUP);
  pinMode(ENCB_RIGHT, INPUT_PULLUP);

  pinMode(JOYSTICK_BUTTON_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENCB_LEFT),
                  left_encoder, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCB_RIGHT),
                  right_encoder, RISING);

  stop_car();
  Serial.println("Ready. Press joystick button.");
}

// ---------- Main loop ----------
void loop() {
  bool button = digitalRead(JOYSTICK_BUTTON_PIN);

  // Run once when the button is pressed.
  if (previous_button == HIGH && button == LOW) {
    delay(30);  // Debounce

    if (digitalRead(JOYSTICK_BUTTON_PIN) == LOW) {
      run_route();

      // Wait for release to prevent repeated execution.
      while (digitalRead(JOYSTICK_BUTTON_PIN) == LOW) {
        delay(10);
      }
    }
  }

  previous_button = digitalRead(JOYSTICK_BUTTON_PIN);
}
