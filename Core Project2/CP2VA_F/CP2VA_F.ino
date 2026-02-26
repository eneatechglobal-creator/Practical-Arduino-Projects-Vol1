

// --- PIN DEFINITIONS ---
const int redPin = 9;
const int yellowPin = 7;       // YELLOW LED on Pin 7 (Physical Pin Swap)
const int greenPin = 8;        // GREEN LED on Pin 8 (Physical Pin Swap)
const int buzzerPin = 6;
const int buttonPin = 2;       // Pedestrian Button
const int potPin = A0;         // Timing Potentiometer

// --- STATE VARIABLES ---
// 0:GREEN, 1:YELLOW, 2:RED, 3:PEDESTRIAN, 4:RED_YELLOW
int trafficState = 2;          // Start in RED (2)
unsigned long stateStartTime = 0;
unsigned long potTimingMs = 5000; // Default 5 seconds

// --- TIMING CONSTANTS ---
const long minGreenTime = 3000;
const long maxGreenTime = 10000;
const long yellowTime = 2000;
const long redTime = 5000;
const long pedestrianTime = 4000;
const long redYellowTime = 1000; // 1 second for Red+Yellow phase (NEW)

// --- BUZZER PATTERN VARIABLES (Non-blocking) ---
unsigned long lastBuzzerToggle = 0;
long buzzerInterval = 0;

// -------------------------------------------------------------------
// SETUP & MAIN LOOP
// -------------------------------------------------------------------

void setup() {
  pinMode(redPin, OUTPUT);
  pinMode(yellowPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  trafficState = 2; 
  stateStartTime = millis();
}

void loop() {
  handlePotentiometer();
  handlePedestrianInput();
  checkStateTransition();
  updateOutputs();
}

// -------------------------------------------------------------------
// INPUT HANDLERS
// -------------------------------------------------------------------

void handlePotentiometer() {
  int potValue = analogRead(potPin);
  potTimingMs = map(potValue, 0, 1023, minGreenTime, maxGreenTime);
}

void handlePedestrianInput() {
  if (digitalRead(buttonPin) == LOW) {
    if (trafficState == 0 || trafficState == 1) {
      if (trafficState == 0) {
        changeState(1); // Jump to YELLOW
      }
    }
  }
}

// -------------------------------------------------------------------
// STATE MACHINE CORE (Updated for RED_YELLOW)
// -------------------------------------------------------------------

void checkStateTransition() {
  unsigned long currentTime = millis();
  long elapsedTime = currentTime - stateStartTime;
  
  switch (trafficState) {
    case 0: // GREEN
      if (elapsedTime >= potTimingMs) {
        changeState(1); // GREEN -> YELLOW
      }
      break;
      
    case 1: // YELLOW (Transition to Red)
      if (elapsedTime >= yellowTime) {
        changeState(2); // YELLOW -> RED
      }
      break;
      
    case 2: // RED (Longest State)
      if (elapsedTime >= redTime) {
        changeState(3); // RED -> PEDESTRIAN
      }
      break;
      
    case 3: // PEDESTRIAN (Safe Crossing)
      if (elapsedTime >= pedestrianTime) {
        changeState(4); // PEDESTRIAN -> RED_YELLOW (New State)
      }
      break;
      
    case 4: // NEW: RED_YELLOW (Transition to Green)
      if (elapsedTime >= redYellowTime) {
        changeState(0); // RED_YELLOW -> GREEN
      }
      break;
  }
}

void changeState(int newState) {
  trafficState = newState;
  stateStartTime = millis();
  
  digitalWrite(redPin, LOW);
  digitalWrite(yellowPin, LOW);
  digitalWrite(greenPin, LOW);
  noTone(buzzerPin);
}

// -------------------------------------------------------------------
// OUTPUTS AND BUZZER PATTERN (Updated for RED_YELLOW)
// -------------------------------------------------------------------

void updateOutputs() {
  switch (trafficState) {
    case 0: // GREEN
      digitalWrite(greenPin, HIGH);
      buzzerInterval = 250; 
      handleBuzzerPattern();
      break;
      
    case 1: // YELLOW
      digitalWrite(yellowPin, HIGH);
      buzzerInterval = 10;
      handleBuzzerPattern();
      break;
      
    case 2: // RED
      digitalWrite(redPin, HIGH);
      buzzerInterval = 1000; 
      handleBuzzerPattern();
      break;
      
    case 3: // PEDESTRIAN
      digitalWrite(redPin, HIGH); // Keep traffic stopped!
      buzzerInterval = 100; 
      handleBuzzerPattern();
      break;
      
    case 4: // NEW: RED_YELLOW
      digitalWrite(redPin, HIGH);      // Red and...
      digitalWrite(yellowPin, HIGH);   // ...Yellow ON!
      buzzerInterval = 10; 
      handleBuzzerPattern();
      break;
  }
}

// NON-BLOCKING BUZZER HANDLER
void handleBuzzerPattern() {
  if (millis() - lastBuzzerToggle >= buzzerInterval) {
    lastBuzzerToggle = millis();
    
    if (digitalRead(buzzerPin) == HIGH) {
      digitalWrite(buzzerPin, LOW);
    } else {
      digitalWrite(buzzerPin, HIGH);
    }
  }
}
