
 
#include <LiquidCrystal.h> 
 
// --- PIN DEFINITIONS ---
// LCD Pinout (RS, E, D4, D5, D6, D7) - NOTE: Pins are non-consecutive
// (D12, D11, D5, D4, D3, D2)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2); 
 
const int redPin = 10;
const int yellowPin = 9;
const int greenPin = 8;
const int buzzerPin = 7;     // D7 is the only available pin
const int buttonPin = A0;    // Using Analog Pin for extra Digital Input
 
// --- STATE VARIABLES & TIMING ---
int trafficState = 0;          // 0:RED, 1:GREEN, 2:YELLOW
unsigned long stateStartTime = 0;
long redTimeMs = 10000;          // Initial 10 seconds for RED
const long initialRedTime = 10000;
const long reducedRedTime = 6000;
const long greenTimeMs = 6000;
const long yellowTimeMs = 5000;
 
// --- BUTTON & BUZZER VARIABLES ---
bool timeReduced = false;
bool lastButtonState = HIGH;
unsigned long lastBuzzerToggle = 0;
const long buzzerInterval = 500; // 500ms for "tick-tick-tick"
 
// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
 
void setup() {
  pinMode(redPin, OUTPUT);
  pinMode(yellowPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP); // Button to GND
 
  lcd.begin(16, 2);
  lcd.print("Traffic Light V-C");
  lcd.setCursor(0, 1);
  lcd.print("Starting RED...");
  delay(2000);
  
  // Start the FSM
  changeState(0);
}
 
// -------------------------------------------------------------------
// MAIN LOOP
// -------------------------------------------------------------------
 
void loop() {
  handleButtonInput();
  checkStateTransition();
  updateDisplay();
}
 
// -------------------------------------------------------------------
// INPUT & TRANSITION LOGIC
// -------------------------------------------------------------------
 
void handleButtonInput() {
  bool reading = digitalRead(buttonPin);
  
  // Simple edge detection (LOW to HIGH transition on release)
  if (reading == HIGH && lastButtonState == LOW) {
    // Only allow reduction if it hasn't been reduced AND if we are in RED
    if (trafficState == 0 && !timeReduced) {
      redTimeMs = reducedRedTime;
      timeReduced = true; // Flag reduction permanently
      lcd.setCursor(0, 1);
      lcd.print("TIME REDUCED!");
      delay(1000); // Small pause for user to see message
      
      // OPTIONAL: Restart the RED timer with the new, shorter duration
      stateStartTime = millis(); 
    }
  }
  lastButtonState = reading;
}
 
void checkStateTransition() {
  unsigned long currentTime = millis();
  long elapsedTime = currentTime - stateStartTime;
  long requiredDuration = 0;
  
  // 1. Determine the required duration for the current state
  if (trafficState == 0) requiredDuration = redTimeMs;
  else if (trafficState == 1) requiredDuration = greenTimeMs;
  else if (trafficState == 2) requiredDuration = yellowTimeMs;

  // 2. Check if the time has elapsed
  if (elapsedTime >= requiredDuration) {
    int nextState = 0; // Default next state is RED (0)
    
    // R -> Y -> G -> R sequence: 0 -> 2 -> 1 -> 0
    switch (trafficState) {
      case 0: // Currently RED, next is YELLOW
        nextState = 2; 
        break;
      case 2: // Currently YELLOW, next is GREEN
        nextState = 1; 
        break;
      case 1: // Currently GREEN, next is RED
        nextState = 0; 
        break;
      default:
        nextState = 0;
    }
    
    changeState(nextState);
  } else {
    // Run continuous outputs (like the buzzer pattern)
    updateOutputs(requiredDuration - elapsedTime);
  }
}
 
void changeState(int newState) {
  trafficState = newState;
  stateStartTime = millis(); 
  
  // Reset all light outputs and buzzer
  digitalWrite(redPin, LOW);
  digitalWrite(yellowPin, LOW);
  digitalWrite(greenPin, LOW);
  noTone(buzzerPin);
}
 
// -------------------------------------------------------------------
// OUTPUTS AND DISPLAY
// -------------------------------------------------------------------
 
void updateOutputs(long timeLeft) {
  switch (trafficState) {
    case 0: // RED
      digitalWrite(redPin, HIGH);
      break;
      
    case 1: // GREEN
      digitalWrite(greenPin, HIGH);
      // Buzzer: Tick-tick-tick pattern
      if (millis() - lastBuzzerToggle >= buzzerInterval) {
        lastBuzzerToggle = millis();
        // Toggle the buzzer output pin
        if (digitalRead(buzzerPin) == HIGH) {
          digitalWrite(buzzerPin, LOW);
        } else {
          // Use a tone frequency for the tick sound
          tone(buzzerPin, 800, buzzerInterval / 2); 
        }
      }
      break;
      
    case 2: // YELLOW
      digitalWrite(yellowPin, HIGH);
      break;
  }
}
 
void updateDisplay() {
  unsigned long timeRemaining = 0;
  if (trafficState == 0) timeRemaining = redTimeMs - (millis() - stateStartTime);
  else if (trafficState == 1) timeRemaining = greenTimeMs - (millis() - stateStartTime);
  else if (trafficState == 2) timeRemaining = yellowTimeMs - (millis() - stateStartTime);
 
  // Convert milliseconds to seconds
  int secondsRemaining = (timeRemaining / 1000) + 1; // Round up to the next second
 
  lcd.setCursor(0, 0); 
  
  if (trafficState == 0) {
    lcd.print("STOP            "); // Line 1: Main Status
    lcd.setCursor(0, 1);
    lcd.print("Wait: ");
    lcd.print(secondsRemaining);
    lcd.print("s");
    if (timeReduced) lcd.print(" (Reduced)");
    else lcd.print(" (Full)  ");
    
  } else if (trafficState == 1) {
    lcd.print("GO              "); // Line 1: Main Status
    lcd.setCursor(0, 1);
    lcd.print("Time: ");
    lcd.print(secondsRemaining);
    lcd.print("s");
    lcd.print(" (Sound ON) ");
    
  } else { // State 2: YELLOW
    lcd.print("READY           "); // Line 1: Main Status (16 characters, good)
    lcd.setCursor(0, 1);
    lcd.print("Next: GO");
    // FIX: Use 8 spaces to fully clear the rest of the 16-character line.
    lcd.print("        "); 
  }
}
