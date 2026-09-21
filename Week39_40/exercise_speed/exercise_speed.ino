// -----------------------------
// Joystick
// -----------------------------
const int joystickX = A5;
const int joystickY = A6;

// -----------------------------
// Motors
// -----------------------------
#define Motor_forward    0
#define Motor_return     1

#define Motor_L_dir_pin  7
#define Motor_R_dir_pin  8

#define Motor_L_pwm_pin  9
#define Motor_R_pwm_pin  10

// -----------------------------
// Joystick calibration
// -----------------------------
const int X_CENTER = 49;   // Forward/backward center
const int Y_CENTER = 50;   // Left/right center

// Small area around center where car stays stopped
const int DEAD_ZONE = 5;

// Steering strength
// 100 = normal
// >100 = stronger steering
const int STEERING_GAIN = 100;


// -----------------------------
// Setup
// -----------------------------
void setup() {

  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);

  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);

  // Start stopped
  analogWrite(Motor_L_pwm_pin, 0);
  analogWrite(Motor_R_pwm_pin, 0);
}


// -----------------------------
// Main loop
// -----------------------------
void loop() {

  // Read joystick
  int xRaw = analogRead(joystickX);
  int yRaw = analogRead(joystickY);

  // Convert to 0-100%
  int x = (xRaw * 100L) / 1023;
  int y = (yRaw * 100L) / 1023;


  // -----------------------------------------
  // X axis = forward / backward
  // -----------------------------------------

  int throttle = 0;

  if (x < X_CENTER - DEAD_ZONE) {

    // Forward
    throttle = map(
      x,
      0,
      X_CENTER - DEAD_ZONE,
      255,
      0
    );

  }
  else if (x > X_CENTER + DEAD_ZONE) {

    // Backward
    throttle = map(
      x,
      X_CENTER + DEAD_ZONE,
      100,
      0,
      255
    );
  }


  // -----------------------------------------
  // Y axis = steering
  // -----------------------------------------

  int steering = 0;

  if (y < Y_CENTER - DEAD_ZONE) {

    // Right
    steering = map(
      y,
      0,
      Y_CENTER - DEAD_ZONE,
      255,
      0
    );

  }
  else if (y > Y_CENTER + DEAD_ZONE) {

    // Left
    steering = map(
      y,
      Y_CENTER + DEAD_ZONE,
      100,
      0,
      255
    );
  }


  // -----------------------------------------
  // Determine direction
  // -----------------------------------------

  bool forward = (x < X_CENTER);


  if (throttle == 0) {

    // Joystick centered -> STOP
    analogWrite(Motor_L_pwm_pin, 0);
    analogWrite(Motor_R_pwm_pin, 0);

  }
  else {

    // ---------------------------------------
    // Set direction
    // ---------------------------------------

    if (forward) {

      digitalWrite(Motor_L_dir_pin, Motor_forward);
      digitalWrite(Motor_R_dir_pin, Motor_forward);

    }
    else {

      digitalWrite(Motor_L_dir_pin, Motor_return);
      digitalWrite(Motor_R_dir_pin, Motor_return);
    }


    // ---------------------------------------
    // Differential steering
    // ---------------------------------------

    int leftSpeed = throttle;
    int rightSpeed = throttle;


    if (steering > 0) {

      steering = steering * STEERING_GAIN / 100;
      steering = constrain(steering, 0, 255);


      if (y < Y_CENTER) {

        // RIGHT TURN
        leftSpeed  = throttle + steering;
        rightSpeed = throttle - steering;

      }
      else {

        // LEFT TURN
        leftSpeed  = throttle - steering;
        rightSpeed = throttle + steering;
      }
    }


    // ---------------------------------------
    // Limit PWM
    // ---------------------------------------

    leftSpeed = constrain(leftSpeed, 0, 255);
    rightSpeed = constrain(rightSpeed, 0, 255);


    // ---------------------------------------
    // Drive motors
    // ---------------------------------------

    analogWrite(Motor_L_pwm_pin, leftSpeed);
    analogWrite(Motor_R_pwm_pin, rightSpeed);
  }
}

