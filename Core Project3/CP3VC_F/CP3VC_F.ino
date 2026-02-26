

#include <LiquidCrystal.h>

// -------------------------------------------------
// PIN DEFINITIONS
// -------------------------------------------------
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
const int binaryLedPins[] = {6, 7, 8, 10}; // 1s, 2s, 4s, 8s bit (LSB to MSB)

const int buzzerPin = 9;
const int potPin = A0; // Mode Select Pot

// -------------------------------------------------
// STATE VARIABLES
// -------------------------------------------------
int currentCount = 0;
int countingMode = 0;
const char* modeNames[] = {
  "Normal (0-9)",
  "Reverse (9-0)",
  "Random (0-9)",
  "Binary Flash"
};

unsigned long lastTickTime = 0;
// Fixed tickInterval since Pot is used for mode select
const unsigned long tickInterval = 1000; 

// -------------------------------------------------
// BINARY FLASH
// -------------------------------------------------
unsigned long lastFlashToggle = 0;
const unsigned long flashInterval = 100;
bool flashState = false;

// -------------------------------------------------
// NON-BLOCKING BUZZER ENGINE
// -------------------------------------------------
struct Beep {
  int freq;
  unsigned long duration;
};

// Max size needed is 3 (for double/triple/ascending)
Beep beepSequence[3]; 
int beepCount = 0;
int beepIndex = 0;
unsigned long beepStartTime = 0;
bool buzzerActive = false;

// -------------------------------------------------
// SETUP
// -------------------------------------------------
void setup() {
lcd.begin(16, 2);
lcd.print("CP3 Final (POT)");

  for (int i = 0; i< 4; i++) pinMode(binaryLedPins[i], OUTPUT);
pinMode(buzzerPin, OUTPUT);

randomSeed(analogRead(A5)); // Use unconnected pin for random seed
  delay(1000);   
lcd.clear();
}

// -------------------------------------------------
// MAIN LOOP
// -------------------------------------------------
void loop() {
handlePotModeSelect(); 
handleBuzzer();

  if (countingMode != 3) {
    // Modes 0, 1, 2 (Timed Counting)
    if (millis() - lastTickTime>= tickInterval) {
lastTickTime = millis();
updateCount();
updateOutputs();
    }
  } else {
    // Mode 3: Binary Flashing
    if (millis() - lastTickTime>= tickInterval) {
lastTickTime = millis();
updateCount();
updateOutputs(); // Updates LCD with new count, then falls through to flash handler
flashState = true;
lastFlashToggle = millis();
    }
handleBinaryFlash(); // Continuously handles the rapid ON/OFF toggle
  }
}

// -------------------------------------------------
// INPUT HANDLERS
// -------------------------------------------------
void handlePotModeSelect() {
  int potValue = analogRead(potPin);
  int newMode = 0;

  // Map 0-1023 into four discrete zones (0, 1, 2, 3)
  if (potValue< 256) {
newMode = 0;
  } else if (potValue< 512) {
newMode = 1;
  } else if (potValue< 768) {
newMode = 2;
  } else {
newMode = 3;
  }

  if (newMode != countingMode) {
countingMode = newMode;

    // Mode Change Actions
currentCount = 0;
lastTickTime = millis();
lcd.clear();
startBeepSequence(countingMode);
updateOutputs(); 
  }
}

// -------------------------------------------------
// COUNT LOGIC
// -------------------------------------------------
void updateCount() {
  switch (countingMode) {
    case 0: // Normal
    case 3: // Binary Flashing (Normal count pattern)
currentCount = (currentCount + 1) % 10;
      break;
    case 1: // Reverse
currentCount--;
      if (currentCount< 0) currentCount = 9;
      break;
    case 2: // Random
currentCount = random(0, 10);
      break;
  }
}

// -------------------------------------------------
// OUTPUTS
// -------------------------------------------------
void updateOutputs() {
updateLCD();
  // Only use solid display if NOT in binary flash mode
  if (countingMode != 3) displayBinary(currentCount, true);
}

void updateLCD() {
  // Line 1: Mode Name
lcd.setCursor(0, 0);
lcd.print(modeNames[countingMode]);
lcd.print("        "); 

  // Line 2: Count and Binary Value
lcd.setCursor(0, 1);
lcd.print("Count: ");
lcd.print(currentCount);
lcd.print(" (");

  // Print 4-bit Binary Representation
  for (int i = 3; i>= 0; i--) {
    // Use bitwise shift and mask to print the binary representation
lcd.print((currentCount>>i) & 1);
  }
lcd.print(")");
lcd.print("  "); // Clear remaining space
}

void displayBinary(int num, bool state) {
  // Controls the 4 LEDs based on the number's binary value
  for (int i = 0; i< 4; i++) {
    // The conditional operator handles the logic:
    // If the i-th bit is 1, set the LED to 'state' (HIGH or LOW).
    // If the i-th bit is 0, set the LED to LOW (OFF).
digitalWrite(binaryLedPins[i],
      ((num >>i) & 1) ? (state ? HIGH : LOW) : LOW);
  }
}

void handleBinaryFlash() {
  if (millis() - lastFlashToggle>= flashInterval) {
lastFlashToggle = millis();
flashState = !flashState; // Toggle the flash state

    // Call displayBinary with the flashState to make the 1s flash ON/OFF
displayBinary(currentCount, flashState); 
  }
}

// -------------------------------------------------
// NON-BLOCKING BUZZER ENGINE IMPLEMENTATION
// -------------------------------------------------
void startBeepSequence(int mode) {
buzzerActive = true;
beepIndex = 0;
beepStartTime = millis();

  switch (mode) {
    case 0: // Normal: Single Beep
beepCount = 1;
beepSequence[0] = {800, 120};
      break;

    case 1: // Reverse: Double Beep
beepCount = 2;
beepSequence[0] = {600, 100};
beepSequence[1] = {600, 100};
      break;

    case 2: // Random: Triple Beep
beepCount = 3;
beepSequence[0] = {1000, 70};
beepSequence[1] = {1000, 70};
beepSequence[2] = {1000, 70};
      break;

    case 3: // Binary Flashing: Ascending Tone
beepCount = 3;
beepSequence[0] = {500, 60};
beepSequence[1] = {700, 60};
beepSequence[2] = {900, 60};
      break;
  }
  // Start the first tone immediately
  tone(buzzerPin, beepSequence[0].freq);
}

void handleBuzzer() {
  if (!buzzerActive) return;

  // Check if the current tone duration is over
  if (millis() - beepStartTime>= beepSequence[beepIndex].duration) {
noTone(buzzerPin);
beepIndex++;

    if (beepIndex>= beepCount) {
      // Sequence finished
buzzerActive = false;
      return;
    }

    // Start the next tone in the sequence
beepStartTime = millis();
    // A small pause between tones can be added here if needed, 
    // but the `noTone()` provides a slight natural pause.
    tone(buzzerPin, beepSequence[beepIndex].freq);
  }
}
