#include <LiquidCrystal.h>
#include <Keypad.h>
 
// --- I/O PIN DEFINITIONS (MEGA 2560) ---
// LCD Pins (RS, E, D4, D5, D6, D7) - Digital Pins 30-35
const int rs = 30, en = 31, d4 = 32, d5 = 33, d6 = 34, d7 = 35; 
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
 
const int POT_PIN = A0;      // Potentiometer for Sensor Input
const int BUZZER_PIN = 2;    // Buzzer output pin (D2)
 
// --- LED Bar Graph Pins (10 segments: D3 to D12) ---
const int LED_PINS[] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 12}; 
const int NUM_SEGMENTS = 10;
 
const int MAX_SETPOINT = 100;
const char SETPOINT_KEY = '#'; // Key to confirm setpoint entry
 
// --- KEYPAD SETUP (4x4) ---
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
// Rows R1-R4 connected to D25, D24, D23, D22 
byte rowPins[ROWS] = {25, 24, 23, 22}; 
// Columns C1-C4 connected to D26, D27, D28, D29
byte colPins[COLS] = {26, 27, 28, 29};     
 
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
 
// --- STATE VARIABLES ---
int setpoint = 50;         
int currentReading = 0;   
String inputBuffer = "";  
bool entryMode = true;    
 
// Variables for Non-Blocking Alert Timing
unsigned long lastBuzzerTime = 0;
const int BUZZER_PULSE_MS = 100; // Buzzer on/off cycle duration
unsigned long lastFlashTime = 0;
const int LED_FLASH_MS = 150;    // LED flash cycle duration
bool ledsFlashing = false;
 
// --- FUNCTION PROTOTYPES ---
void handleKeypad();
void updateLCD();
void updateBarGraph(int reading);
void updateBuzzer(int reading);
 
// --- SETUP ---
void setup() {
lcd.begin(16, 2);

  // Initialize the Buzzer and LED Pins
pinMode(BUZZER_PIN, OUTPUT);
  for (int i = 0; i< NUM_SEGMENTS; i++) {
pinMode(LED_PINS[i], OUTPUT);
digitalWrite(LED_PINS[i], LOW);
  }

  // Initial LCD display (Entry Mode)
updateLCD(); 
}
 
// --- LOOP ---
void loop() {
  if (entryMode) {
handleKeypad();
    // Small delay helps debounce/slow down the keypad polling
    delay(50);
  } else {
    // 1. Read Inputs
    int potValue = analogRead(POT_PIN);
    // Scale 0-1023 (analog input) to 0-100 (simulated value)
currentReading = map(potValue, 0, 1023, 0, 100);

    // 2. Update Outputs (LCD, Bar Graph, Buzzer)
updateLCD();
updateBarGraph(currentReading);
updateBuzzer(currentReading);
  }
}
 
// --- KEYPAD FUNCTIONS ---
void handleKeypad() {
  char key = keypad.getKey();

  if (key != NO_KEY) {
    if (key >= '0' && key <= '9') {
      // Max 3 digits (allows up to 100)
      if (inputBuffer.length() < 3) {
inputBuffer += key;
updateLCD(); // Refresh display with new digit
      }
    } else if (key == SETPOINT_KEY) { // '#' pressed - Confirm Setpoint
      if (inputBuffer.length() > 0) {
        int newSetpoint = inputBuffer.toInt();

        // Validate input is within 0-100 range
        if (newSetpoint>= 0 &&newSetpoint<= MAX_SETPOINT) {
          setpoint = newSetpoint;
entryMode = false; // Switch to Monitoring Mode
noTone(BUZZER_PIN); // Ensure buzzer is off after confirmation
        } 
inputBuffer = ""; // Clear buffer regardless of validation success
      }
updateLCD(); // Update LCD to show new state/setpoint
    } else if (key == '*') {
      // '*' acts as a clear/backspace function
inputBuffer = "";
updateLCD();
    }
  }
}
 
// --- OUTPUT FUNCTIONS ---
void updateLCD() {
lcd.setCursor(0, 0);

  if (entryMode) {
    // Display for Setpoint Entry Mode
lcd.print("Enter Setpoint");

lcd.setCursor(0, 1);
lcd.print("Input: ");
lcd.print(inputBuffer);
    // Clear remaining characters on the line
lcd.print("        "); 

  } else {
    // Display for Monitoring Mode
    bool alert = currentReading> setpoint;

    // Line 0: Setpoint and Current Reading (Format: Set: [SP] Cur: [CR])
lcd.print("Set: ");
lcd.print(setpoint); 

lcd.print(" Cur: "); 

lcd.print(currentReading); 

    // Clear remaining characters on the line to ensure it's clean
    if (currentReading< 100) {
lcd.print("  "); // Print 2 spaces if reading is 1 or 2 digits
    } else {
lcd.print(" "); // Print 1 space if reading is 3 digits (100)
    }


    // Line 1: Status/Alert
lcd.setCursor(0, 1);
    if (alert) {
lcd.print("ALERT! OVER MAX "); // The space clears the last character
    } else {
lcd.print("Status: OK      "); // Clear the entire line
    }
  }
}
 
void updateBarGraph(int reading) {
  bool alert = reading > setpoint;

  if (alert) {
    // Alert Mode: Flash all 10 LEDs (Non-blocking)
    if (millis() - lastFlashTime>= LED_FLASH_MS) { 
ledsFlashing = !ledsFlashing; // Toggle the flash state
lastFlashTime = millis();
    }
    for (int i = 0; i< NUM_SEGMENTS; i++) {
digitalWrite(LED_PINS[i], ledsFlashing ? HIGH : LOW);
    }
  } else {
    // Monitoring Mode: Display Level (10% steps)
    // Map reading 0-100 to number of segments 0-10
    int segments_on = map(reading, 0, 100, 0, NUM_SEGMENTS);

    for (int i = 0; i< NUM_SEGMENTS; i++) {
digitalWrite(LED_PINS[i], (i<segments_on) ? HIGH : LOW);
    }
ledsFlashing = false; // Reset flash state when alert ends
  }
}
 
void updateBuzzer(int reading) {
  bool alert = reading > setpoint;

  if (alert) {
    // Alert Mode: Pulse Buzzer (Non-blocking)
    if (millis() - lastBuzzerTime>= BUZZER_PULSE_MS) {
lastBuzzerTime = millis();

      // Check if the current time slot is an 'ON' or 'OFF' cycle
      // The tone will automatically turn off after BUZZER_PULSE_MS
      if ((lastBuzzerTime / BUZZER_PULSE_MS) % 2 == 0) { 
        tone(BUZZER_PIN, 3000, BUZZER_PULSE_MS); // Tone: 3kHz for 100ms
      } else {
noTone(BUZZER_PIN); // Ensure silence for the 100ms gap
      }
    }
  } else {
noTone(BUZZER_PIN); // Ensure buzzer is off during normal operation
}
}
