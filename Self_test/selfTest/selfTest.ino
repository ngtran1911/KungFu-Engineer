
const int joystickX = A5;
const int joystickY = A6;
const int deadzone = 5;


#define Motor_forward    0
#define Motor_backward   1
#define Motor_L_dir_pin  7
#define Motor_R_dir_pin  8

#define Motor_L_pwm_pin  9
#define Motor_R_pwm_pin  10

void setup() {
  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);

  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);
}

void loop() {
  int xValue = analogRead(joystickX);
  int yValue = analogRead(joystickY);
  
  int speedforward = map(yValue,0,1023, -255, 255);
  int turnspeed = map(xValue, 0,1023, -255, 255);

  if (abs(speedforward) < deadzone) speedforward = 0;
  if ( abs(turnspeed) < deadzone) turnspeed = 0;

  int rightspeed = speedforward + turnspeed;
  int leftspeed  = speedforward - turnspeed;

  // constrain func - force a number to stay within a range you specify
  // because rightspeed and leftspeed can go to 512-
  // if numb go that high will be force to the maxvalue 255
  rightspeed = constrain(rightspeed, -255, 255);
  leftspeed  = constrain(leftspeed, -255, 255);

  //control the direction of motor
  // Right motor
  if (rightspeed < 0) {
    digitalWrite(Motor_R_dir_pin, Motor_backward);
  } else {
    digitalWrite(Motor_R_dir_pin, Motor_forward);
  }
  analogWrite(Motor_R_pwm_pin, abs(rightspeed));

  // Left motor
  if (leftspeed < 0) {
    digitalWrite(Motor_L_dir_pin, Motor_backward);
  } else {
    digitalWrite(Motor_L_dir_pin, Motor_forward);
  }
  analogWrite(Motor_L_pwm_pin, abs(leftspeed));
}



