#include <Keypad.h>
#include <LiquidCrystal.h>
#include <DHT.h>
#include <Servo.h>

// --- HARDWARE DEFINITIONS (MEGA 2560) ---

// DHT Sensor
#define DHTPIN 2    // DHT Data Pin (D2)
#define DHTTYPE DHT11 // !!! CHANGED to DHT11 !!!
DHT dht(DHTPIN, DHTTYPE);

// Buzzer & Servo
const int BUZZER_PIN = 3;  // Buzzer Signal (D3)
const int SERVO_PIN = 5;   // Servo Signal (D5 - PWM)

Servo servoMotor;

// --- LCD DEFINITIONS (Parallel 4-bit Mode) ---
// RS=D52, E=D53, D4=D6, D5=D7, D6=D8, D7=D9 (Corrected Mapping)
const int rs = 52, en = 53, d4 = 6, d5 = 7, d6 = 8, d7 = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// --- KEYPAD DEFINITIONS (4x4) ---
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
// Rows: D44-D47 | Columns: D48-D51
byte rowPins[ROWS] = {44, 45, 46, 47}; 
byte colPins[COLS] = {48, 49, 50, 51}; 

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// --- STATE VARIABLES ---
float currentTempC = 0.0;
float currentHumid = 0.0;
int maxHumiditySet = 65; // Default max humidity threshold
int servoPosition = 0;   // Servo starts closed (0 degrees)

// Alert & Buzzer Timing
const long sensorInterval = 2000; // Read sensor every 2 seconds
unsigned long lastSensorRead = 0;
const long buzzerInterval = 1500; // 1.5 seconds between beeps
unsigned long lastBuzzerTime = 0;
bool alertActive = false;

// Keypad Input State
String inputBuffer = "";
bool settingThreshold = false;


// --- FUNCTION PROTOTYPES ---
void readSensor();
void readKeypad();
void updateServo();
void checkAlert();
void updateLCD();
void pulseBuzzer();


// --- SETUP ---
void setup() {
Serial.begin(9600);
lcd.begin(16, 2);
lcd.print("Init System...");
dht.begin();
servoMotor.attach(SERVO_PIN);
servoMotor.write(servoPosition); // Initialize to 0 degrees

delay(1000);
lcd.clear();
lcd.setCursor(0, 0);
lcd.print("H:--.-% T:--.-C");
lcd.setCursor(0, 1);
lcd.print("MAX H: 65% SER:0");
}


// --- LOOP ---
void loop() {
readKeypad();

  // Time-based actions
  if (millis() - lastSensorRead>sensorInterval) {
lastSensorRead = millis();
readSensor();
checkAlert();
updateServo(); // Execute control logic
updateLCD(); 
  }

  // Handle buzzer pulsing non-blocking
  if (alertActive) {
pulseBuzzer();
  } else {
noTone(BUZZER_PIN); 
  }
}


// --- LOGIC FUNCTIONS ---
void readSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  // Check if sensor reading failed
  if (isnan(h) || isnan(t)) return;
currentHumid = h;
currentTempC = t;
}

void readKeypad() {
  char key = keypad.getKey();
  if (key) {
    if (key >= '0' && key <= '9') {
      if (!settingThreshold) {
settingThreshold = true;
inputBuffer = "";
lcd.setCursor(0, 0);
lcd.print("Set Max H (%):   "); 
      }
      if (inputBuffer.length() < 2) {
inputBuffer += key;
lcd.setCursor(14, 0);
lcd.print(inputBuffer);
      }
      if (inputBuffer.length() == 2) {
maxHumiditySet = inputBuffer.toInt();
settingThreshold = false;
lcd.clear();
      }
    } else if (key == '#' &&settingThreshold) {
      if (inputBuffer.length() > 0) {
maxHumiditySet = inputBuffer.toInt();
      }
settingThreshold = false;
lcd.clear();
    }
  }
}

void checkAlert() {
  // Set alert flag if humidity exceeds the threshold
  if (currentHumid>maxHumiditySet) {
alertActive = true;
  } else {
alertActive = false;
  }
}

void updateServo() {
  // Threshold Control Logic: Servo is coupled to the alert state
  if (alertActive) {
servoPosition = 180; // Open the vent/valve
  } else {
servoPosition = 0;   // Close the vent/valve
  }

  // Command the servo
servoMotor.write(servoPosition);
}

void pulseBuzzer() {
  // Plays a short tone every 1.5 seconds
  if (millis() - lastBuzzerTime>= buzzerInterval) {
lastBuzzerTime = millis();
    const int toneDuration = 150;
tone(BUZZER_PIN, 1200, toneDuration); 
  }
}


// --- DISPLAY FUNCTIONS ---
void updateLCD() {
  if (settingThreshold) return;

  // Line 1: Current Readings
lcd.setCursor(0, 0);
lcd.print("H:");
lcd.print(currentHumid, 1);
lcd.print("% T:");
lcd.print(currentTempC, 1);
lcd.print((char)223); // Degree symbol
lcd.print("C  ");

  // Line 2: Setpoint and Control Status
lcd.setCursor(0, 1);
lcd.print("MAX H:");
lcd.print(maxHumiditySet);
lcd.print("% SER:");
lcd.print(servoPosition);

  // Show Alert Status
  if (alertActive) {
lcd.print(" !"); 
  } else {
lcd.print("  "); 
  }
}
