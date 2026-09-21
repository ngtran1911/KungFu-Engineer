#define Motor_forward    0
#define Motor_return     1
#define Motor_L_dir_pin  7
#define Motor_R_dir_pin  8
#define Motor_L_pwm_pin  9
#define Motor_R_pwm_pin  10

void setup() {
    pinMode(Motor_L_dir_pin, OUTPUT);
    pinMode(Motor_R_dir_pin, OUTPUT);
    pinMode(Motor_L_pwm_pin, OUTPUT);
    pinMode(Motor_R_pwm_pin, OUTPUT);

    // 1. Left wheel forward at 30% for 2 seconds
    digitalWrite(Motor_L_dir_pin, Motor_forward);
    analogWrite(Motor_L_pwm_pin, 77);

    digitalWrite(Motor_R_dir_pin, Motor_forward);
    analogWrite(Motor_R_pwm_pin, 0);

    delay(2000);

    // 2. Right wheel forward at 30% for 10 seconds
    analogWrite(Motor_L_pwm_pin, 0);
    analogWrite(Motor_R_pwm_pin, 77);

    delay(10000);

    // 3. Both wheels backwards at 75% for 4 seconds
    digitalWrite(Motor_L_dir_pin, Motor_return);
    digitalWrite(Motor_R_dir_pin, Motor_return);

    analogWrite(Motor_L_pwm_pin, 191);
    analogWrite(Motor_R_pwm_pin, 191);

    delay(4000);

    // 4. Stop
    analogWrite(Motor_L_pwm_pin, 0);
    analogWrite(Motor_R_pwm_pin, 0);
}

void loop() {
    // Do nothing - sequence runs only once
}

