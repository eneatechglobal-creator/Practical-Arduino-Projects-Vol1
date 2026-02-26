
#include <LiquidCrystal.h> 

// --- Pin Definitions ---
const int buttonPin = 2; 
const int ledPin = 11; 
const int buzzerPin = 12; 

// --- LCD Initialization ---
// RS pin moved to D7. Init structure: LiquidCrystal(RS, E, d4, d5, d6, d7)
LiquidCrystal lcd(7, 10, 5, 4, 3, 6); 

// --- State Variables ---
int systemState = 0; 
int lastSystemState = 0; 
// ✅ NEW: Stable state variable for debouncing
int stableButtonState = HIGH; // Tracks the clean state of the button

// --- Button Timing Variables ---
long buttonPressStartTime = 0;
const long longPressThreshold = 1000; 
int lastDebouncePinState = HIGH; // Tracks the raw pin state for the timer
unsigned long lastDebounceTime = 0;
const long debounceDelay = 50;

// --- Timing Variables for Non-Blocking Functions ---
unsigned long lastBlinkToggleTime = 0;
const long blinkInterval = 250; 

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------

void setup() {
  pinMode(buttonPin, INPUT_PULLUP); 
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  
  lcd.begin(16, 2);
  lcd.print("System Initialized"); 
  delay(1500);
}

// -------------------------------------------------------------------
// MAIN LOOP
// -------------------------------------------------------------------

void loop() {
  handleButtonInput();
  updateOutputs();
  updateDisplay();
}

// -------------------------------------------------------------------
// INPUT HANDLING FUNCTION (Robust Debounce & Edge Detection)
// -------------------------------------------------------------------

void handleButtonInput() {
  int reading = digitalRead(buttonPin);

  // 1. Reset Debounce Timer if RAW input changes
  if (reading != lastDebouncePinState) {
    lastDebounceTime = millis();
  }

  // 2. Check if the Debounce Period has passed
  if ((millis() - lastDebounceTime) > debounceDelay) {
    
    // 3. Check for a Stable Change (the actual press/release event)
    if (reading != stableButtonState) {
      stableButtonState = reading; // Update the clean state

      // --- PRESS EVENT (HIGH to LOW) ---
      if (stableButtonState == LOW) {
        buttonPressStartTime = millis(); 
      } 
      // --- RELEASE EVENT (LOW to HIGH) ---
      else { 
        long pressDuration = millis() - buttonPressStartTime;

        if (pressDuration < longPressThreshold) {
          // --- SHORT PRESS LOGIC (State 0->1, State 2->0) ---
          if (systemState == 0 || systemState == 2) {
            systemState = 1; 
          } else if (systemState == 1) {
            systemState = 0; 
          }
        } else {
          // --- LONG PRESS LOGIC (State 1->2) ---
          if (systemState == 1) {
            systemState = 2; 
          }
        }
      }
    }
  }
  
  // 4. Update the raw tracking state (must be done outside the debounce block)
  lastDebouncePinState = reading; 
}

// -------------------------------------------------------------------
// OUTPUT CONTROL FUNCTION (Based on State - No Change)
// -------------------------------------------------------------------

void updateOutputs() {
  switch (systemState) {
    case 0: 
      digitalWrite(ledPin, LOW);
      digitalWrite(buzzerPin, LOW); 
      break;

    case 1: 
      digitalWrite(ledPin, HIGH);
      digitalWrite(buzzerPin, HIGH); 
      break;

    case 2: 
      handleBlinkAndPattern();
      break;
  }
}

// -------------------------------------------------------------------
// NON-BLOCKING BLINK AND PATTERN HANDLER (Used in State 2 - No Change)
// -------------------------------------------------------------------

void handleBlinkAndPattern() {
  if (millis() - lastBlinkToggleTime >= blinkInterval) {
    lastBlinkToggleTime = millis(); 
    
    if (digitalRead(ledPin) == HIGH) {
      digitalWrite(ledPin, LOW);
    } else {
      digitalWrite(ledPin, HIGH);
    }

    if (digitalRead(ledPin) == HIGH) {
      digitalWrite(buzzerPin, HIGH); 
    } else {
      digitalWrite(buzzerPin, LOW);  
    }
  }
}

// -------------------------------------------------------------------
// DISPLAY UPDATE FUNCTION (No Change)
// -------------------------------------------------------------------

void updateDisplay() {
  if (systemState != lastSystemState) {
    lcd.clear();
    lcd.setCursor(0, 0); 

    switch (systemState) {
      case 0:
        lcd.print("SYSTEM OFF");
        lcd.setCursor(0, 1);
        lcd.print("Ready to Start");
        break;
      case 1:
        lcd.print("LED & BUZZER ON");
        lcd.setCursor(0, 1);
        lcd.print("Long Press for Blink");
        break;
      case 2:
        lcd.print("LED BLINKING");
        lcd.setCursor(0, 1);
        lcd.print("Buzzer Pattern Active");
        break;
    }
    lastSystemState = systemState; 
  }
}