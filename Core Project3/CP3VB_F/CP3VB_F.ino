

// ---------------- PIN DEFINITIONS ----------------
// Segments A-G connected to D10, D11, D12, D2, D3, D4, D5 respectively
const int segmentPins[] = {10, 11, 12, 2, 3, 4, 5}; 
const int buzzerPin = 9;
const int buttonPin = A0;  // Wired to GND, using INPUT_PULLUP

// ---------------- 7-SEG LOOKUP (COMMON CATHODE) ----------------
// The segment order is always {A, B, C, D, E, F, G}
// 1 = HIGH (ON), 0 = LOW (OFF)
const byte digitPatterns[10][7] = {
  {1,1,1,1,1,1,0}, // 0
  {0,1,1,0,0,0,0}, // 1
  {1,1,0,1,1,0,1}, // 2
  {1,1,1,1,0,0,1}, // 3
  {0,1,1,0,0,1,1}, // 4
  {1,0,1,1,0,1,1}, // 5
  {1,0,1,1,1,1,1}, // 6
  {1,1,1,0,0,0,0}, // 7
  {1,1,1,1,1,1,1}, // 8
  {1,1,1,1,0,1,1}  // 9
};

// ---------------- STATE ----------------
int currentDigit = 0;

// ---------------- BUTTON TIMING (Optimized for Stability) ----------------
const unsigned long debounceDelay    = 20;  // Faster Debounce
const unsigned long longPressTime    = 600; // Easier to trigger Long Press (Decrement)
const unsigned long doubleClickTime  = 500; // CRUCIAL: Gives enough time to register Double Click

bool lastReading = HIGH;
bool buttonState = HIGH;

unsigned long lastDebounceTime = 0;
unsigned long pressStartTime   = 0;
unsigned long lastReleaseTime  = 0;

int clickCount = 0;

// ---------------- BUZZER ----------------
const int baseFreq = 500;
const int soundDuration = 80;

// ------------------------------------------------
// SETUP
// ------------------------------------------------
void setup() {
  for (int i = 0; i< 7; i++) {
pinMode(segmentPins[i], OUTPUT);
  }

pinMode(buzzerPin, OUTPUT);
  // Use INPUT_PULLUP for button wired to GND
pinMode(buttonPin, INPUT_PULLUP);

  // Set initial random seed for the random mode
randomSeed(analogRead(A5));

displayDigit(currentDigit); // Show 0 initially
}

// ------------------------------------------------
// LOOP
// ------------------------------------------------
void loop() {
handleButton();
}

// ------------------------------------------------
// BUTTON HANDLER (STABLE)
// ------------------------------------------------
void handleButton() {
  bool reading = digitalRead(buttonPin);
  unsigned long now = millis();

  // 1. Debounce
  if (reading != lastReading) {
lastDebounceTime = now;
  }

  // Skip the rest of the logic if currently debouncing
  if ((now - lastDebounceTime) <debounceDelay) {
lastReading = reading;
return;
  }

  // 2. Button Pressed (Falling Edge: HIGH -> LOW)
  if (reading == LOW &&buttonState == HIGH) {
pressStartTime = now;
buttonState = LOW;
  }

  // 3. Button Released (Rising Edge: LOW -> HIGH)
  if (reading == HIGH &&buttonState == LOW) {
    unsigned long pressDuration = now - pressStartTime;
buttonState = HIGH;

    if (pressDuration>= longPressTime) {
      // ACTION: LONG PRESS (>= 600ms) -> DECREMENT
currentDigit = (currentDigit == 0) ?9 :currentDigit - 1;
playSound();
displayDigit(currentDigit);
clickCount = 0; // Clear clicks if it was a Long Press
    } else {
      // SHORT PRESS (< 600ms) -> Potential Single/Double Click
clickCount++;
lastReleaseTime = now;
    }
  }

  // 4. Resolve single / double click
  // Checks if a click has occurred AND the doubleClickTime window has elapsed
  if (clickCount> 0 && (now - lastReleaseTime) >doubleClickTime) {
    if (clickCount == 1) {
      // ACTION: SINGLE CLICK -> INCREMENT
currentDigit = (currentDigit + 1) % 10;
playSound();
displayDigit(currentDigit);
    } else if (clickCount>= 2) {
      // ACTION: DOUBLE CLICK -> RANDOM
currentDigit = random(0, 10);
playSpecialSound();
displayDigit(currentDigit);
    }
    // Reset click counter after resolving the action
clickCount = 0;
  }

lastReading = reading;
}

// ------------------------------------------------
// OUTPUT FUNCTIONS
// ------------------------------------------------
void displayDigit(int digit) {
  for (int i = 0; i< 7; i++) {
digitalWrite(segmentPins[i], digitPatterns[digit][i]);
  }
}

void playSound() {
  // Map digit 0-9 to a range of frequencies for varied pitch
  int freq = map(currentDigit, 0, 9, baseFreq, baseFreq + 500);
tone(buzzerPin, freq, soundDuration);
}

void playSpecialSound() {
  // Rapid ascending tone for random change
tone(buzzerPin, 1000, 50);
delay(50);
tone(buzzerPin, 1500, 50);
delay(50);
tone(buzzerPin, 2000, 50);
delay(50);
noTone(buzzerPin);
}
