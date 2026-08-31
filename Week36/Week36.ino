
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

//Ex 3
const int sensorPin = A0;
const int ledPin = 8;

const float threshold = 1.5;  // Voltage threshold

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // Read the LDR
  int adcValue = analogRead(sensorPin);

  // Convert ADC value to voltage
  float voltage = adcValue * (5.0 / 1023.0);

  // Print voltage
  Serial.print(voltage, 3);
  Serial.println(" V");

  // Turn LED on when LDR is covered
  if (voltage < threshold) {
    digitalWrite(ledPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
  }

  // Maximum measurement frequency = 1 Hz
  delay(1000);
}