#include <LiquidCrystal.h>
#include <EEPROM.h>

// --- HARDWARE PIN DEFINITIONS (LCD in 4-bit mode) ---
const int rs = 7;
const int en = 6;
const int d4 = 11;
const int d5 = 12;
const int d6 = 13;
const int d7 = 14;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

#define TILT_SWITCH_PIN 2   // Mega Interrupt 0
#define POT_PIN A0          // Analog input for threshold
#define LED_PIN 5           // Logged Event Indicator

// --- INTERRUPT & TIMING VARIABLES ---
volatile bool tiltActive = false;
volatile unsigned long startTime = 0;
volatile unsigned long durationMs = 0;

// --- EEPROM AND LOGGING VARIABLES ---
const int EEPROM_ADDRESS = 0;
int eventCount = 0;
unsigned long loggingThreshold = 1000; // Minimum duration in ms to log (default 1s)

// -----------------------------------------------------------------
// INTERRUPT SERVICE ROUTINE (ISR)
// -----------------------------------------------------------------
void tiltInterruptISR() {
  if (digitalRead(TILT_SWITCH_PIN) == LOW) {
    // TILT STARTED (Pin went LOW)
startTime = millis();
tiltActive = true;
durationMs = 0;
  } else {
    // TILT ENDED (Pin went HIGH)
    if (tiltActive) {
durationMs = millis() - startTime;
tiltActive = false;
    }
  }
}

void setup() {
Serial.begin(9600);

  // Load Event Count from EEPROM
EEPROM.get(EEPROM_ADDRESS, eventCount);
  if (eventCount> 9999) eventCount = 0; // Sanity check

  // Pin Setup
pinMode(TILT_SWITCH_PIN, INPUT_PULLUP); // Use pull-up resistor
pinMode(LED_PIN, OUTPUT);

  // Attach Interrupt: Trigger on CHANGE (both RISING and FALLING edges)
attachInterrupt(digitalPinToInterrupt(TILT_SWITCH_PIN), tiltInterruptISR, CHANGE);

  // LCD Setup
lcd.begin(16, 2);
lcd.print("TILT LOGGER V B");
delay(2000);
lcd.clear();
}

void loop()
{
readThreshold();
checkAndLogEvent();
updateLCD();

  // Update LED based on current tilt state
digitalWrite(LED_PIN, digitalRead(TILT_SWITCH_PIN) == LOW ? HIGH : LOW);

delay(50);
}

// -----------------------------------------------------------------
// CONTROL AND LOGIC
// -----------------------------------------------------------------
void readThreshold() {
  // Read POT (0-1023) and map to threshold (1000ms to 5000ms)
  int potValue = analogRead(POT_PIN);
loggingThreshold = map(potValue, 0, 1023, 1000, 5000);
}

void checkAndLogEvent() {
  // Check if a tilt event has concluded (durationMs> 0)
  if (durationMs> 0) {
    if (durationMs>= loggingThreshold) {
      // ** LOGGING CONDITION MET **
eventCount++;
EEPROM.put(EEPROM_ADDRESS, eventCount); // Write to persistent memory

      // Flash LED twice to confirm logged event
for(int i=0; i<2; i++){
digitalWrite(LED_PIN, HIGH); delay(100);
digitalWrite(LED_PIN, LOW); delay(100);
      }
    }

    // Reset the duration flag after checking
durationMs = 0;
  }
}

void updateLCD() {
  // Row 1: Threshold & Status
lcd.setCursor(0, 0);
lcd.print("Thresh: ");
lcd.print(loggingThreshold / 1000.0, 1);
lcd.print("s ");

  // Print live duration if currently tilted
  if (tiltActive) {
lcd.print("TILTED ");
  } else {
lcd.print("STABLE ");
  }

  // Row 2: Event Count
lcd.setCursor(0, 1);
lcd.print("Logged: ");
lcd.print(eventCount);
lcd.print("  "); // Clear remaining space
}
