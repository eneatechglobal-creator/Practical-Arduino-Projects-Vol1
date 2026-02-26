

#include <LiquidCrystal.h>

// --- PIN DEFINITIONS ---
LiquidCrystal lcd(12, 11, 5, 4, 3, 2); 
const int trigPin = 7;
const int echoPin = 8;
const int ledPin = 6;
const int buzzerPin = 9;

// --- STATE VARIABLES ---
long durationMicroseconds;
float distanceCm;
long alertInterval = 0; // The time (ms) between alerts (0 = OFF)
unsigned long lastAlertTime = 0;
bool alertState = LOW; // Tracks the ON/OFF state of the LED/Buzzer

// --- TIMING/ALERT CONSTANTS ---
const int SAFE_THRESHOLD = 30; // cm
const int MEDIUM_THRESHOLD = 20; // cm
const int DANGER_THRESHOLD = 10; // cm

const long INTERVAL_SLOW = 500; // 500ms (1Hz)
const long INTERVAL_MEDIUM = 250; // 250ms (2Hz)
const long INTERVAL_RAPID = 100; // 100ms (5Hz)
const int BUZZER_FREQ = 1000;
const long BUZZER_DURATION = 50; // Short 50ms pulse

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  
  lcd.begin(16, 2);
  lcd.print("PARKING SENSOR V1");
  delay(2000);
  lcd.clear();
}

// -------------------------------------------------------------------
// MAIN LOOP
// -------------------------------------------------------------------

void loop() {
  measureDistance();
  setAlertLevel();
  handleAlerts();
  updateLCD();
}

// -------------------------------------------------------------------
// SENSOR LOGIC
// -------------------------------------------------------------------

void measureDistance() {
  // Clear the Trig pin by setting it LOW for a moment
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Set the Trig pin HIGH for 10 microseconds to send the pulse
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Measure the duration of the Echo pulse
  durationMicroseconds = pulseIn(echoPin, HIGH);
  
  // Convert time to distance (cm)
  // Speed of sound = 343 m/s = 0.0343 cm/µs. Time to travel 1 cm is 29.15 µs (round trip).
  // Distance = (Duration / 2) / 29.15 = Duration / 58.3
  distanceCm = durationMicroseconds / 58.0;
  
  // Clamp distance to a reasonable max display value
  if (distanceCm > 400 || distanceCm < 0) {
      distanceCm = 400; 
  }
}

// -------------------------------------------------------------------
// ALERT LOGIC
// -------------------------------------------------------------------

void setAlertLevel() {
  if (distanceCm > SAFE_THRESHOLD) {
    alertInterval = 0; // OFF
  } else if (distanceCm > MEDIUM_THRESHOLD) {
    alertInterval = INTERVAL_SLOW; // 500ms
  } else if (distanceCm > DANGER_THRESHOLD) {
    alertInterval = INTERVAL_MEDIUM; // 250ms
  } else {
    alertInterval = INTERVAL_RAPID; // 100ms
  }
}

void handleAlerts() {
  unsigned long currentTime = millis();
  
  if (alertInterval == 0) {
    // Alert OFF (SAFE)
    digitalWrite(ledPin, LOW);
    noTone(buzzerPin);
    alertState = LOW;
    return;
  }
  
  if (currentTime - lastAlertTime >= alertInterval) {
    lastAlertTime = currentTime;
    
    alertState = !alertState; // Toggle the state
    
    digitalWrite(ledPin, alertState);
    
    // Play a short tone only when turning ON
    if (alertState == HIGH) {
      tone(buzzerPin, BUZZER_FREQ, BUZZER_DURATION);
    } else {
      noTone(buzzerPin);
    }
  }
}

// -------------------------------------------------------------------
// DISPLAY LOGIC
// -------------------------------------------------------------------

void updateLCD() {
  // Line 1: Alert Status
  lcd.setCursor(0, 0);
  if (distanceCm > SAFE_THRESHOLD) {
    lcd.print("STATUS: SAFE    ");
  } else if (distanceCm > MEDIUM_THRESHOLD) {
    lcd.print("STATUS: SLOW ALERT");
  } else if (distanceCm > DANGER_THRESHOLD) {
    lcd.print("STATUS: FASTER ALERT");
  } else {
    lcd.print("STATUS: DANGER (BOOM!)");
  }
  
  // Line 2: Distance Value
  lcd.setCursor(0, 1);
  lcd.print("Dist: ");
  lcd.print(distanceCm, 1); // Print distance with 1 decimal place
  lcd.print(" cm      "); // Clear remaining space
}
