#include <LiquidCrystal.h> 

// ---------------- PIN DEFINITIONS ----------------

// Buttons (external 10kΩ pull-down → GND, button to +5V)
const int ledButtonPin  = 2;
const int buzzButtonPin = 3;

// Outputs
const int ledPin    = 9;
const int buzzerPin = 8;

// LCD: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, 5, 4, 7, 6);

// ---------------- STATE VARIABLES ----------------

// 0 = OFF, 1 = ON, 2 = PATTERN
int ledState = 0;
int buzzerState = 0;

int lastLedState = -1;
int lastBuzzerState = -1;

// ---------------- TIMING CONSTANTS ----------------

const unsigned long debounceDelay = 50;
const unsigned long longPressThreshold = 1000;
const unsigned long ledBlinkInterval = 250;

// ---------------- BUTTON TRACKING ----------------

// LED button
int ledStableState = LOW;
int ledLastReading = LOW;
unsigned long ledDebounceTime = 0;
unsigned long ledPressStart = 0;

// Buzzer button
int buzzStableState = LOW;
int buzzLastReading = LOW;
unsigned long buzzDebounceTime = 0;
unsigned long buzzPressStart = 0;

// ---------------- BLINK TIMING ----------------

unsigned long lastBlinkToggleTime = 0;

// ------------------------------------------------
// SETUP
// ------------------------------------------------

void setup() {
  pinMode(ledButtonPin, INPUT);
  pinMode(buzzButtonPin, INPUT);

  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  lcd.begin(16, 2);
  lcd.print("CP1VB: Dual-Mode");
  lcd.setCursor(0, 1);
  lcd.print("Pull-Down Ready");

  delay(1500);
}

// ------------------------------------------------
// MAIN LOOP
// ------------------------------------------------

void loop() {
  handleButton(
    ledButtonPin,
    &ledLastReading,
    &ledStableState,
    &ledDebounceTime,
    &ledPressStart,
    &ledState
  );

  handleButton(
    buzzButtonPin,
    &buzzLastReading,
    &buzzStableState,
    &buzzDebounceTime,
    &buzzPressStart,
    &buzzerState
  );

  updateOutputs();
  updateDisplay();
}

// ------------------------------------------------
// BUTTON HANDLER (EXTERNAL PULL-DOWN)
// ------------------------------------------------

void handleButton(
  int pin,
  int *lastReading,
  int *stableState,
  unsigned long *debounceTime,
  unsigned long *pressStart,
  int *state
) {
  int reading = digitalRead(pin);

  if (reading != *lastReading) {
    *debounceTime = millis();
  }

  if (millis() - *debounceTime > debounceDelay) {

    if (reading != *stableState) {
      *stableState = reading;

      // Press detected (LOW → HIGH)
      if (*stableState == HIGH) {
        *pressStart = millis();
      }
      // Release detected (HIGH → LOW)
      else {
        unsigned long pressDuration = millis() - *pressStart;

        if (pressDuration < longPressThreshold) {
          if (*state == 0) *state = 1;
          else if (*state == 1 || *state == 2) *state = 0;
        } else {
          if (*state == 1) *state = 2;
        }
      }
    }
  }

  *lastReading = reading;
}

// ------------------------------------------------
// OUTPUT CONTROL
// ------------------------------------------------

void updateOutputs() {
  // LED
  if (ledState == 0) digitalWrite(ledPin, LOW);
  else if (ledState == 1) digitalWrite(ledPin, HIGH);
  else handleLedBlink();

  // Buzzer
  if (buzzerState == 0) noTone(buzzerPin);
  else if (buzzerState == 1) tone(buzzerPin, 1000);
  else handleBuzzerPattern();
}

// ------------------------------------------------
// LED BLINK HANDLER
// ------------------------------------------------

void handleLedBlink() {
  if (millis() - lastBlinkToggleTime >= ledBlinkInterval) {
    lastBlinkToggleTime = millis();
    digitalWrite(ledPin, !digitalRead(ledPin));
  }
}

// ------------------------------------------------
// BUZZER PATTERN
// ------------------------------------------------

void handleBuzzerPattern() {
  static unsigned long lastBuzzTime = 0;
  static bool buzzOn = false;

  if (millis() - lastBuzzTime >= 120) {
    lastBuzzTime = millis();
    buzzOn = !buzzOn;

    if (buzzOn) tone(buzzerPin, 2000, 50);
    else noTone(buzzerPin);
  }
}

// ------------------------------------------------
// LCD UPDATE
// ------------------------------------------------

void updateDisplay() {
  if (ledState != lastLedState || buzzerState != lastBuzzerState) {
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("LED:");
    lcd.print(ledState == 0 ? " OFF   " :
              ledState == 1 ? " ON    " : " BLINK");

    lcd.setCursor(0, 1);
    lcd.print("BUZZ:");
    lcd.print(buzzerState == 0 ? " SILENT" :
              buzzerState == 1 ? " TONE  " : " PATTERN");

    lastLedState = ledState;
    lastBuzzerState = buzzerState;
  }
}
