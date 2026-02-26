
#include <LiquidCrystal.h> 
#include <stdio.h> 
#include <math.h> // Required for round()

// --- LCD Pin Initialization ---
LiquidCrystal lcd(12, 11, 5, 4, 7, 6); 

// --- Pin Definitions ---
const int controlPotPin = A0;  // Analog pin for the pattern control Potentiometer
const int ledPin = 9;          // PWM pin for LED
const int buzzerPin = 8;       // Digital pin for the Active Buzzer

// --- State Variables ---
int systemState = 0; // Start in OFF state
int lastSystemState = -1; 

// --- Potentiometer Threshold ---
const int POT_OFF_THRESHOLD = 10; // Analog value < 10 forces system OFF

// --- Timing and Analog Control Variables ---
unsigned long lastPatternToggleTime = 0;
int currentIntervalMs = 500;    
float currentFrequencyHz = 1.0; 
int lastAnalogValue = 0;        
const int analogUpdateThreshold = 10; 

// --- LCD Buffers ---
char line1Buffer[17]; 

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
 
void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  Serial.begin(9600); 

  // Initialize LCD
  lcd.begin(16, 2);
  lcd.print("System Initialized");
  lcd.setCursor(0, 1);
  lcd.print("P2 = ON/Speed");
  delay(1500);
}
 
// -------------------------------------------------------------------
// MAIN LOOP
// -------------------------------------------------------------------
 
void loop() {
  // 1. Read Analog Input (P2/A0) and Map to Control Variables
  readAndMapAnalogInput();
  
  // 2. NEW LOGIC: Handle state transitions based on Potentiometer reading
  handlePotentiometerState();
  
  // 3. Output Logic: Update Outputs based on Current State
  updateOutputs(); 
  
  // 4. Feedback Logic: Update Display
  updateDisplay();
}

// -------------------------------------------------------------------
// ANALOG INPUT HANDLING FUNCTION
// -------------------------------------------------------------------
 
void readAndMapAnalogInput() {
  int analogValue = analogRead(controlPotPin);
 
  // Map analog 0-1023 to LED/Buzzer interval (1000ms is slow, 50ms is fast)
  // We use the greater of the analog value or the threshold to prevent 
  // division by zero/huge numbers if the reading is zero.
  int effectiveAnalogValue = max(analogValue, POT_OFF_THRESHOLD); 
  
  currentIntervalMs = map(effectiveAnalogValue, POT_OFF_THRESHOLD, 1023, 1000, 50); 
  
  // Frequency (Hz) = 1000 / (2 * Period)
  currentFrequencyHz = 1000.0 / (2.0 * currentIntervalMs); 

  lastAnalogValue = analogValue;
}

// -------------------------------------------------------------------
// NEW STATE HANDLING FUNCTION (Zero-Crossing)
// -------------------------------------------------------------------
void handlePotentiometerState() {
  int analogValue = analogRead(controlPotPin);

  if (analogValue < POT_OFF_THRESHOLD) {
    systemState = 0; // OFF State
  } else {
    systemState = 2; // PATTERN State
  }
}
 
// -------------------------------------------------------------------
// OUTPUT CONTROL FUNCTION
// -------------------------------------------------------------------
 
void updateOutputs() {
  switch (systemState) {
    case 0: // OFF/IDLE State
      digitalWrite(ledPin, LOW);
      digitalWrite(buzzerPin, LOW); 
      // Note: Missing lastPatternToggleTime = millis() for clean start, but using tested code.
      break;
    
    // State 1 (ON/STEADY) is no longer used for simplicity
    
    case 2: // PATTERN/ANALOG_CONTROL State
      handleAnalogControlledPattern();
      break;
  }
}

// -------------------------------------------------------------------
// NON-BLOCKING PATTERN HANDLER
// -------------------------------------------------------------------
 
void handleAnalogControlledPattern()
{
  if (millis() - lastPatternToggleTime >= currentIntervalMs) {
    lastPatternToggleTime = millis(); 
    
    digitalWrite(ledPin, !digitalRead(ledPin));
    digitalWrite(buzzerPin, !digitalRead(buzzerPin));
    Serial.print("State 2: Interval="); 
    Serial.print(currentIntervalMs);
    Serial.println("ms"); 
  }
}
 
// -------------------------------------------------------------------
// DISPLAY UPDATE FUNCTION (FLOAT FIX INCLUDED)
// -------------------------------------------------------------------
 
void updateDisplay() {
  bool stateChanged = (systemState != lastSystemState);
  int currentAnalogValue = analogRead(controlPotPin);
  
  // Update check: State change OR (State 2 AND significant knob change)
  bool analogValueChanged = (abs(currentAnalogValue - lastAnalogValue) >= analogUpdateThreshold);

  if (stateChanged) {
    lcd.clear(); 
  }

  if (stateChanged || (systemState == 2 && analogValueChanged)) {
    
    lcd.setCursor(0, 0); 
    
    switch (systemState) {
      case 0:
        lcd.print("SYSTEM OFF      ");
        lcd.setCursor(0, 1);
        lcd.print("Turn P2 Knob ON ");
        break;
      
      case 2:
        // Line 1: LED Blink Speed (Interval)
        sprintf(line1Buffer, "LED Speed: %4dms", currentIntervalMs);
        lcd.print(line1Buffer);
 
        // --- Line 2: Tone Freq (Float Fix) ---
        int freqTimesTen = round(currentFrequencyHz * 10.0);
        int freqWhole = freqTimesTen / 10;
        int freqDec = freqTimesTen % 10;

        lcd.setCursor(0, 1);
        lcd.print("Tone Freq:  "); // Print base text
        
        lcd.setCursor(11, 1); // Start number at column 11
        lcd.print(freqWhole);
        lcd.print(".");
        lcd.print(freqDec);
        lcd.print("Hz");
        
        // Optional: Print a space to clear any old trailing characters
        if (freqWhole < 10) { 
            lcd.print(" ");
        }
        break;
    }
  }

  if (stateChanged || (systemState == 2 && analogValueChanged)) {
     lastSystemState = systemState; 
     lastAnalogValue = currentAnalogValue;
  }
}
