#include <LiquidCrystal.h>

// --- I/O PIN DEFINITIONS AND CONSTANTS ---
// LCD Pins (RS, E, D4, D5, D6, D7) connected to Arduino Digital Pins 12 down to 7
const int rs = 12, en = 11, d4 = 10, d5 = 9, d6 = 8, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const int POT_LEVEL_PIN = A0;   // Potentiometer for Level Input (Left Pot)
// The Contrast Potentiometer (Right Pot) is physically wired to the LCD V0 pin and GND/VCC.
const int BUZZER_PIN = 1;       // Buzzer output pin (D1)

// --- LED Bar Graph Pins (D2-D6, A2-A5, D13) ---
// D13 replaces the unavailable A6 for the 10th segment.
const int LED_PINS[] = {2, 3, 4, 5, 6, A2, A3, A4, A5, 13}; 
const int NUM_SEGMENTS = 10;

const int CLIP_THRESHOLD = 80;

// --- STATE VARIABLES ---
int level = 0;
int prevLevel = -1; // Force initial draw
bool clipping = false;
unsigned long lastBuzzerTime = 0;
const int BUZZER_PULSE_MS = 100;

// --- FUNCTION PROTOTYPES ---
void updateLCD(int level);
void updateBarGraph(int level);
void updateBuzzer(int level);

// --- SETUP ---
void setup() {
  // Set up LCD 
  lcd.begin(16, 2);
  // Initial message shown for a moment (until loop takes over)
  lcd.print("Level Meter V.B");

  // Set up pins
  pinMode(BUZZER_PIN, OUTPUT);
  for (int i = 0; i < NUM_SEGMENTS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }
}

// --- LOOP ---
void loop() {
  // 1. Read Inputs
  int potValue = analogRead(POT_LEVEL_PIN);

  // 2. Convert to Percentage
  level = map(potValue, 0, 1023, 0, 100);

  // 3. Update Outputs (Only if level has changed)
  if (level != prevLevel) {
    updateLCD(level);
    updateBarGraph(level);
    prevLevel = level;
  }

  // 4. Update Buzzer (Continuous/Pulsed)
  updateBuzzer(level);
}

// --- OUTPUT FUNCTIONS ---

void updateLCD(int level) {
  // 1. Clear line 0 for fresh draw (avoids flicker on line 1)
  lcd.setCursor(0, 0);
  lcd.print("                "); // Prints 16 spaces
  lcd.setCursor(0, 0);

  // 2. Display Percentage
  if (level < 10) lcd.print("0"); 
  lcd.print(level);
  lcd.print("% ");

  // 3. Display Text Bar (10 segments: |)
  int barSegments = map(level, 0, 100, 0, 10);
  for (int i = 0; i < 10; i++) {
    if (i < barSegments) {
      lcd.print("|"); // Solid block
    } else {
      lcd.print(" "); // Empty space
    }
  }

  // 4. Display Clipping/Warning on Line 1
  clipping = (level >= CLIP_THRESHOLD);
  lcd.setCursor(0, 1);
  if (clipping) {
    lcd.print("!! **CLIP** !!  "); 
  } else {
    lcd.print("OK              "); 
  }
}

void updateBarGraph(int level) {
  int segments_on = map(level, 0, 100, 0, NUM_SEGMENTS);
  for (int i = 0; i < NUM_SEGMENTS; i++) {
    // LEDs are connected with current-limiting resistors, active HIGH
    digitalWrite(LED_PINS[i], (i < segments_on) ? HIGH : LOW);
  }
}

void updateBuzzer(int level) {
  unsigned long currentTime = millis();

  if (level == 0) {
    noTone(BUZZER_PIN);
  } else if (level >= CLIP_THRESHOLD) {
    // HIGH ALERT: Pulsing Tone (2000 Hz)
    if (currentTime - lastBuzzerTime >= BUZZER_PULSE_MS) {
      lastBuzzerTime = currentTime;
      if ((currentTime / BUZZER_PULSE_MS) % 2 == 0) { 
        tone(BUZZER_PIN, 2000, BUZZER_PULSE_MS);
      } else {
        noTone(BUZZER_PIN);
      }
    }
  } else if (level >= 40) {
    // MID RANGE: Solid Mid Tone (1000 Hz)
    tone(BUZZER_PIN, 1000);
  } else { // 1 to 39
    // LOW RANGE: Solid Low Tone (500 Hz)
tone(BUZZER_PIN, 500);
  }
}
