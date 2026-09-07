#include <LiquidCrystal.h>

// LCD: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(37, 36, 35, 34, 33, 32);

// Joystick
const int joystickX = A8;
const int joystickY = A9;
const int joystickButton = 19;

// Variables used by the interrupt
volatile int buttonPresses = 0;
volatile bool buttonPressed = false;
volatile unsigned long lastDebounceTime = 0;

// Display mode control (only used in loop, not in ISR)
bool showButtonCount = false;

void setup() {
  Serial.begin(9600);

  lcd.begin(16, 2);

  pinMode(joystickButton, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(joystickButton), buttonISR, FALLING);
}

void loop() {

  // Check if the button was pressed
  if (buttonPressed) {

    // Change display mode
    showButtonCount = !showButtonCount;

    // Reset the flag
    buttonPressed = false;

    // Clear LCD before changing display
    lcd.clear();

    // Print once, right when the press is registered
    Serial.print("Push counter: ");
    Serial.println(buttonPresses);
  }

  // -----------------------------
  // Display button press count
  // -----------------------------
  if (showButtonCount) {

    lcd.setCursor(0, 0);
    lcd.print("Push counter: ");

    lcd.setCursor(0, 1);
    lcd.print(buttonPresses);
    lcd.print("        ");

  } else {
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
  }

  delay(100);
}

// -----------------------------
// ISR: keep it short, no delay(), no lcd.print() here
// -----------------------------
void buttonISR() {
  unsigned long currentTime = millis();

  if (currentTime - lastDebounceTime > 200) {
    buttonPresses++;
    buttonPressed = true;
    lastDebounceTime = currentTime;
  }
}