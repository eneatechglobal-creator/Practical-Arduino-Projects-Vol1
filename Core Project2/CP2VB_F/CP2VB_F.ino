
#include <LiquidCrystal.h> 

// --- LCD PIN DEFINITIONS ---
LiquidCrystal lcd(12, 11, 5, 4, 3, 2); 

// --- CUSTOM CHARACTER DEFINITIONS (8 bytes = 1 character) ---
// Define a Walking Man and a Standing Man for basic animation
// Figures are wider/bolder to appear 'bigger'
byte standingMan[] = {
  B01110, // Wider Head
  B11111, // Wider Shoulders
  B01110,
  B01110,
  B11111, // Wider Hips/Arms
  B01110,
  B11011, // Wider Stance
  B10001
};

byte walkingMan[] = {
  B01110,
  B11111,
  B01110,
  B01110,
  B01111, // Leading arm/hip
  B11100, // Bold Leg Position 1
  B01010,
  B10001
};

// --- LIGHT & BUZZER PIN DEFINITIONS ---
const int redPin = 10;
const int yellowPin = 9;
const int greenPin = 8;
const int buzzerPin = 7;

// --- STATE & TIMING VARIABLES ---
int trafficState = 0;              // 0:GREEN, 1:YELLOW, 2:RED, 3:PEDESTRIAN
unsigned long stateStartTime = 0;
// bool buttonPressed = false; // REMOVED: Button is no longer used.

// Fixed Durations
const long greenTimeMs = 6000;
const long yellowTimeMs = 5000;
const long redTimeMs = 6000;
const long pedestrianTimeMs = 4000;

// Animation and Buzzer Timing
unsigned long lastAnimationToggle = 0;
const long animationInterval = 200; // 200ms per frame for walking man
unsigned long lastBuzzerToggle = 0;
const long buzzerInterval = 500; 

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void setup() {
  pinMode(redPin, OUTPUT);
  pinMode(yellowPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  // pinMode(buttonPin, INPUT_PULLUP); // REMOVED: Button setup removed.

  lcd.begin(16, 2);
  
  // Create the custom characters in CGRAM
  lcd.createChar(0, standingMan);
  lcd.createChar(1, walkingMan);
  
  // Start the FSM
  changeState(2); // Start at RED to immediately proceed to PEDESTRIAN, then GREEN
}

// -------------------------------------------------------------------
// MAIN LOOP
// -------------------------------------------------------------------
void loop() {
  // handleButtonInput(); // REMOVED: Input handler removed.
  checkStateTransition();
  updateDisplay();
}

// -------------------------------------------------------------------
// INPUT & TRANSITION LOGIC
// -------------------------------------------------------------------
// void handleButtonInput() { /* REMOVED: This function is deleted */ } 

void checkStateTransition()
{
  unsigned long currentTime = millis();
  long elapsedTime = currentTime - stateStartTime;
  long requiredDuration = 0;
  
  // Determine duration based on current state
  if (trafficState == 0) requiredDuration = greenTimeMs;
  else if (trafficState == 1) requiredDuration = yellowTimeMs;
  else if (trafficState == 2) requiredDuration = redTimeMs;
  else if (trafficState == 3) requiredDuration = pedestrianTimeMs;
  
  // --- Standard Time-based Transition ---
  if (elapsedTime >= requiredDuration) {
    int nextState;
    if (trafficState == 0) nextState = 1; // GREEN -> YELLOW
    else if (trafficState == 1) nextState = 2; // YELLOW -> RED
    else if (trafficState == 2) nextState = 3; // RED -> PEDESTRIAN
    else nextState = 0; // PEDESTRIAN -> GREEN
    
    changeState(nextState);
  } else {
    // Run continuous outputs (buzzer, animation)
    updateOutputs(requiredDuration - elapsedTime);
  }
}

void changeState(int newState) {
  trafficState = newState;
  stateStartTime = millis(); 
  
  digitalWrite(redPin, LOW);
  digitalWrite(yellowPin, LOW);
  digitalWrite(greenPin, LOW);
  noTone(buzzerPin);
  lcd.clear(); // Clear display for new status
}

// -------------------------------------------------------------------
// OUTPUTS AND ANIMATION
// -------------------------------------------------------------------
void updateOutputs(long timeLeft) {
  switch (trafficState) {
    case 0: // GREEN
      digitalWrite(greenPin, HIGH);
      // Buzzer: Tick-tick-tick pattern
      if (millis() - lastBuzzerToggle >= buzzerInterval) {
        lastBuzzerToggle = millis();
        // Use a tone frequency for the tick sound
        tone(buzzerPin, 800, buzzerInterval / 2); 
      }
      break;
      
    case 1: // YELLOW
      digitalWrite(yellowPin, HIGH);
      break;
      
    case 2: // RED
      digitalWrite(redPin, HIGH);
      break;
      
    case 3: // PEDESTRIAN - Animation runs here
      digitalWrite(redPin, HIGH); // Traffic MUST remain RED
      
      if (millis() - lastAnimationToggle >= animationInterval) {
        lastAnimationToggle = millis();
        
        // Use a static bool to reliably toggle between the two custom characters
        static bool isWalking = false; 
        
        // Determine which character to use
        byte manChar = isWalking ? 1 : 0; // 1=Walking, 0=Standing
        
        // Write the character three times side-by-side
        lcd.setCursor(13, 1);
        lcd.write(manChar); 
        lcd.setCursor(14, 1);
        lcd.write(manChar); 
        lcd.setCursor(15, 1);
        lcd.write(manChar); 
        
        isWalking = !isWalking; // Toggle for the next frame
      }
      break;
  }
}

// -------------------------------------------------------------------
// DISPLAY UPDATE
// -------------------------------------------------------------------
void updateDisplay() {
  // Convert time remaining to seconds
  unsigned long timeRemaining = 0;
  if (trafficState == 0) timeRemaining = greenTimeMs - (millis() - stateStartTime);
  else if (trafficState == 1) timeRemaining = yellowTimeMs - (millis() - stateStartTime);
  else if (trafficState == 2) timeRemaining = redTimeMs - (millis() - stateStartTime);
  else if (trafficState == 3) timeRemaining = pedestrianTimeMs - (millis() - stateStartTime);
  
  int secondsRemaining = (timeRemaining / 1000) + 1;

  lcd.setCursor(0, 0); 
  
  if (trafficState == 0) {
    // State 0: GREEN
    lcd.print("GO (6s)       ");
    lcd.setCursor(0, 1);
    lcd.print("Time Left: ");
    lcd.print(secondsRemaining);
    lcd.print("s ");
    
  } else if (trafficState == 1) {
    // State 1: YELLOW
    lcd.print("READY (5s)    ");
    lcd.setCursor(0, 1);
    lcd.print("Time Left: ");
    lcd.print(secondsRemaining);
    lcd.print("s ");
    
  } else if (trafficState == 2) {
    // State 2: RED 
    lcd.print("RED (6s)      ");
    lcd.setCursor(0, 1);
    // Updated wait message for fixed cycle
    lcd.print("PEDESTR Next"); 
    
  } else { // State 3: PEDESTRIAN
    
    // State 3: RED/PEDESTRIAN - Fixed cycle always includes this phase
    lcd.setCursor(0, 0);
    lcd.print("RED! CROSS NOW ");
    
    lcd.setCursor(0, 1);
    lcd.print("Time: ");
    lcd.print(secondsRemaining);
    lcd.print("s   "); 
    // The animation is handled by updateOutputs()
  }
}
