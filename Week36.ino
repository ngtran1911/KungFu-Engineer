
//TASK 2

const int sensorPin = A0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  int sensorValue = analogRead(sensorPin);

  float VOUT = sensorValue * (5.0 / 1023.0);

  Serial.print("ADC = ");
  Serial.print(sensorValue);

  Serial.print("    VOUT = ");
  Serial.print(VOUT, 3);
  Serial.println(" V");

  delay(1000);
}