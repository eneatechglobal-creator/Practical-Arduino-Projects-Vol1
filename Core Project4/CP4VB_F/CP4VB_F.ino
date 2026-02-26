
#include <LiquidCrystal.h>

// --- PIN DEFINITIONS ---
LiquidCrystal lcd(12, 11, 5, 4, 3, 2); 
const int trigPin = 7;
const int echoPin = 8;
const int ledPins[] = {A0, A1, A2, A3}; // L1 (Closest) to L4 (Farthest)
const int NUM_LEDS = 4;

// --- STATE VARIABLES ---
float distanceCm;
// The time (ms) to keep an LED ON after the object leaves its zone
const unsigned long PERSISTENCE_DURATION = 2000; // 2.0 seconds
// Tracks the last time (millis) an object was detected in this zone
unsigned long lastDetectedTime[NUM_LEDS] = {0}; 
// Thresholds for L1, L2, L3, L4 (in cm)
const int thresholds[] = {10, 20, 30, 40}; 

// Function prototypes
void measureDistance();
void updateLedStates();
void updateLCD();

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW); // Ensure all are off initially
  }
  
  lcd.begin(16, 2);
  lcd.print("DISTANCE ZONING V1");
  delay(2000);
  lcd.clear();
}

void loop() {
  measureDistance();
  updateLedStates(); 
  updateLCD();
}

// --- SENSOR LOGIC (Same as C4VA) ---
void measureDistance() {
  // ... (HC-SR04 logic to calculate distanceCm) ...
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long durationMicroseconds = pulseIn(echoPin, HIGH);
  distanceCm = durationMicroseconds / 58.0;
  
  if (distanceCm > 400 || distanceCm < 0) {
      distanceCm = 400; 
  }
}

// --- CORE LED/STATE LOGIC ---
void updateLedStates() {
  unsigned long currentTime = millis();
  
  for (int i = 0; i < NUM_LEDS; i++) {
    // 1. Check if the object is currently in the zone (Detection)
    // The check is '<' because L1 is the closest (smallest threshold)
    if (distanceCm < thresholds[i]) {
      lastDetectedTime[i] = currentTime; // Object is detected, refresh timer
    }
    
    // 2. Latching/Persistence Check
    // If the current time is within the persistence window of the last detection
    if (currentTime < lastDetectedTime[i] + PERSISTENCE_DURATION) {
      digitalWrite(ledPins[i], HIGH); // Keep LED ON
    } else {
      digitalWrite(ledPins[i], LOW); // Timeout, turn LED OFF
    }
  }
}

// --- DISPLAY LOGIC ---
void updateLCD() {
  // Build the list of active LEDs for Line 1
  String ledStatus = "ON: ";
  bool firstLed = true;
  for (int i = 0; i < NUM_LEDS; i++) {
    // Check if the LED is currently HIGH
    if (digitalRead(ledPins[i]) == HIGH) {
      if (!firstLed) {
        ledStatus += ", ";
      }
      // 'i + 1' gives the LED number (L1, L2, etc.)
      ledStatus += "L" + String(i + 1);
      firstLed = false;
    }
  }

  if (firstLed) { // If no LEDs are ON
    ledStatus = "ON: NONE        "; // Pad to clear line
  }

  lcd.setCursor(0, 0);
  // Ensure the string fits the 16x2 display
  if (ledStatus.length() > 16) {
    lcd.print(ledStatus.substring(0, 16));
  } else {
    lcd.print(ledStatus);
  }

  // Line 2: Distance Value
  lcd.setCursor(0, 1);
  lcd.print("Dist: ");
  lcd.print(distanceCm, 1);
  lcd.print(" cm       "); // Clear remaining space
}
