
#include <LiquidCrystal.h> 

// --- LCD Initialization ---
// RS, E, D4, D5, D6, D7 (Using common, non-consecutive pins for this example)
LiquidCrystal lcd(12, 11, 5, 4, 7, 6); 

// --- Pin Definitions ---
const int ledPin = 9;              // PWM pin for LED Brightness
const int buzzerPin = 8;           // Digital pin for Passive Buzzer (tone)
const int brightnessPotPin = A0;   // P2 for LED Brightness
const int frequencyPotPin = A1;    // P3 for Buzzer Frequency

// --- Mapping Constants ---
const int minFreq = 100;
const int maxFreq = 2000;

// --- Tracking Variables ---
int ledPercent = 0;
int soundPercent = 0;

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  
  // Note: No pinMode needed for Analog Pins (A0, A1)
  
  lcd.begin(16, 2);
  lcd.print("Analog Control V-D");
  lcd.setCursor(0, 1);
  lcd.print("Pots Set Outputs");
  delay(1500);
}

// -------------------------------------------------------------------
// MAIN LOOP
// -------------------------------------------------------------------

void loop() {
  readAndMapAnalogInputs();
  updateOutputs();
  updateDisplay();
}

// -------------------------------------------------------------------
// ANALOG INPUT HANDLING FUNCTION
// -------------------------------------------------------------------

void readAndMapAnalogInputs() {
  // P2: LED Brightness Control
  int brightVal = analogRead(brightnessPotPin);
  
  // Map P2 (0-1023) to PWM (0-255) for brightness
  int pwmValue = map(brightVal, 0, 1023, 0, 255);
  analogWrite(ledPin, pwmValue);
  
  // Calculate percentage for display (0-100)
  ledPercent = map(brightVal, 0, 1023, 0, 100);

  // P3: Buzzer Frequency Control
  int freqVal = analogRead(frequencyPotPin);
  
  // Map P3 (0-1023) to Frequency (100-2000 Hz)
  int frequencyHz = map(freqVal, 0, 1023, minFreq, maxFreq);
  
  // Output sound via tone()
  if (frequencyHz > minFreq + 5) { // Add small threshold to silence when near zero
    tone(buzzerPin, frequencyHz);
  } else {
    noTone(buzzerPin);
  }

  // Calculate percentage for display (0-100)
  soundPercent = map(freqVal, 0, 1023, 0, 100);
}

// -------------------------------------------------------------------
// OUTPUTS (Simplified, handled within readAndMapAnalogInputs)
// -------------------------------------------------------------------

void updateOutputs() {
  // All output control is handled directly inside readAndMapAnalogInputs
}

// -------------------------------------------------------------------
// DISPLAY UPDATE FUNCTION
// -------------------------------------------------------------------

void updateDisplay() {
  // Line 1: LED Brightness Percentage
  lcd.setCursor(0, 0);
  lcd.print("LED Bright: ");
  lcd.print(ledPercent);
  lcd.print("%  "); // Spaces to clear old digits

  // Line 2: Sound Frequency Percentage
  lcd.setCursor(0, 1);
  lcd.print("Tone Freq: ");
  lcd.print(soundPercent);
  lcd.print("%  "); // Spaces to clear old digits
}
