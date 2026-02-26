
 
// --- PIN DEFINITIONS ---
// Segments A-G connected to D2-D8 respectively
const int segmentPins[] = {2, 3, 4, 5, 6, 7, 8}; 
const int buzzerPin = 9;
const int potPin = A0;      // Speed adjustment
const int modeButtonPin = A1; // Mode selection button
 
// --- LOOKUP TABLE (Common Cathode) ---
// Define the pattern for digits 0 through 9
// The order matches segmentPins: {A, B, C, D, E, F, G}
const byte digitPatterns[][7] = {
  {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, LOW},  // 0
  {LOW, HIGH, HIGH, LOW, LOW, LOW, LOW},      // 1
  {HIGH, HIGH, LOW, HIGH, HIGH, LOW, HIGH},   // 2
  {HIGH, HIGH, HIGH, HIGH, LOW, LOW, HIGH},   // 3
  {LOW, HIGH, HIGH, LOW, LOW, HIGH, HIGH},    // 4
  {HIGH, LOW, HIGH, HIGH, LOW, HIGH, HIGH},   // 5
  {HIGH, LOW, HIGH, HIGH, HIGH, HIGH, HIGH},  // 6
  {HIGH, HIGH, HIGH, LOW, LOW, LOW, LOW},     // 7
  {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH}, // 8
  {HIGH, HIGH, HIGH, HIGH, LOW, HIGH, HIGH}   // 9
};
 
// --- STATE VARIABLES ---
int currentDigit = 0;
unsigned long lastTickTime = 0;
long tickInterval = 1000; // Default 1 second
int countingMode = 0;     // 0:Normal, 1:Reverse, 2:Random
 
// --- BUZZER VARIABLES ---
const int baseFreq = 400; // Base frequency for tick sound
const long tickDuration = 100; // 100ms short sound
 
// --- BUTTON VARIABLES ---
bool lastButtonState = HIGH;
 
// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
 
void setup() {
  // Initialize segment pins as outputs
  for (int i = 0; i < 7; i++) {
    pinMode(segmentPins[i], OUTPUT);
    digitalWrite(segmentPins[i], LOW); // Ensure all segments are OFF initially
  }
  pinMode(buzzerPin, OUTPUT);
  pinMode(modeButtonPin, INPUT_PULLUP);
}
 
// -------------------------------------------------------------------
// MAIN LOOP
// -------------------------------------------------------------------
 
void loop() {
  handlePotTiming();
  handleModeButton();
  
  if (millis() - lastTickTime >= tickInterval) {
    lastTickTime = millis();
    
    // Play the tick sound before changing the digit
    playTickSound(); 
    
    // Update the digit based on the current mode
    updateDigit();
    
    // Display the new digit
    displayDigit(currentDigit);
  }
}
 
// -------------------------------------------------------------------
// HANDLER FUNCTIONS
// -------------------------------------------------------------------
 
void handlePotTiming() {
  int potValue = analogRead(potPin);
  // Map pot (0-1023) to an interval (500ms to 2000ms)
  tickInterval = map(potValue, 0, 1023, 500, 2000);
}
 
void handleModeButton() {
  bool reading = digitalRead(modeButtonPin);
  
  // Check for press (falling edge)
  if (reading == LOW && lastButtonState == HIGH) {
    // Cycle modes: 0 -> 1 -> 2 -> 0
    countingMode = (countingMode + 1) % 3; 
    
    // Reset counter to ensure smooth transition
    if (countingMode == 0 || countingMode == 1) {
      currentDigit = 0; 
    }
  }
  lastButtonState = reading;
}
 
void playTickSound() {
  // Tick frequency changes with the current digit (0-9)
  // Maps 0-9 to a range of frequencies (baseFreq to baseFreq + 200 Hz)
  int frequency = map(currentDigit, 0, 9, baseFreq, baseFreq + 200);
  
  // Play the tone for a short duration
  tone(buzzerPin, frequency, tickDuration);
  
  // Check for the end of the normal sequence (9) for the special sound
  if (currentDigit == 9 && countingMode == 0) {
    // Play a "BOOM" or "Funny Noise" sound effect
    delay(tickDuration + 50); // Wait for the tick to finish
    playSpecialSound();
  }
  // Check for the end of the reverse sequence (0)
  if (currentDigit == 0 && countingMode == 1) {
    delay(tickDuration + 50);
    playSpecialSound();
  }
}
 
void playSpecialSound() {
  // Example: A short, deep "BOOM" sound followed by a quick chirp
  tone(buzzerPin, 100, 300); // Deep tone
  delay(300);
  tone(buzzerPin, 1500, 50); // Chirp
  delay(50);
  noTone(buzzerPin);
}
 
 
void updateDigit() {
  switch (countingMode) {
    case 0: // Normal (0 -> 9)
      currentDigit = (currentDigit + 1);
      if (currentDigit > 9) {
        currentDigit = 0; 
      }
      break;
      
    case 1: // Reverse (9 -> 0)
      currentDigit = (currentDigit - 1);
      if (currentDigit < 0) {
        currentDigit = 9;
      }
      break;
      
    case 2: // Random
      currentDigit = random(0, 10); // 0 up to (but not including) 10
      break;
  }
}
 
 
void displayDigit(int digit) {
  // 1. Get the segment pattern for the current digit
  const byte* pattern = digitPatterns[digit];
  
  // 2. Output the pattern to the segment pins
  for (int i = 0; i < 7; i++) {
    digitalWrite(segmentPins[i], pattern[i]);
  }
}
