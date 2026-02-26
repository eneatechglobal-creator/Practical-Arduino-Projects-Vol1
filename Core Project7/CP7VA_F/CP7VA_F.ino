#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// --- CUSTOM COLORS (RGB565) ---
#define ST7735_DARKGREY  0x7BEF
#define ST7735_NAVY     0x000F

// --- I/O PIN DEFINITIONS ---
const int POT_PINS[] = {A0, A1, A2};
const int LED_PINS[] = {9, 10, 6}; 
const int NUM_VARS = 3;
#define BUZZER_PIN 12 

// TFT Connections
#define TFT_CS 4
#define TFT_DC 7
#define TFT_RST 8 

// Create TFT object (even if white, we keep it)
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// --- CONTROL & ALARM VARIABLES ---
int varValues[NUM_VARS] = {50, 50, 50}; 
const int MIN_VAL = 0;
const int MAX_VAL = 100;
const int SAFE_RANGE_MIN = 20;
const int SAFE_RANGE_MAX = 80;

bool systemAlarmActive = false; 

unsigned long lastBlinkTime = 0;
const int ALARM_BLINK_RATE = 100; 
bool ledState = LOW; 

// Function Prototypes
void initializePins();
void handlePots();
void checkAlarms();
void updateLEDs(int varIndex); 
void drawBar(int varIndex, int value);
void drawHMI(); 
void printStatusSerial();   // 👈 NEW

// --- SETUP ---
void setup() {
  Serial.begin(9600);

  // Try any one init (doesn't matter for serial test)
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST7735_BLACK);

  initializePins();
  drawHMI();

  Serial.println("=== SYSTEM STARTED ===");
}

// --- LOOP ---
void loop()
{
  handlePots();      
  checkAlarms();     
  printStatusSerial();   // 👈 SERIAL DASHBOARD
  delay(300);           // slow so readable
}

// --- CORE LOGIC ---

void initializePins() {
  pinMode(BUZZER_PIN, OUTPUT); 
  for (int i = 0; i < NUM_VARS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
  }
}

void handlePots() {
  for (int i = 0; i < NUM_VARS; i++) {
    int rawValue = analogRead(POT_PINS[i]);
    int newValue = map(rawValue, 0, 1023, MIN_VAL, MAX_VAL); 

    if (newValue != varValues[i]) {
      varValues[i] = newValue;
      drawBar(i, varValues[i]);   // TFT (ignored if white)
    }
  }
}

void checkAlarms() {
  bool newAlarmState = false;

  for (int i = 0; i < NUM_VARS; i++) {
    if (varValues[i] < SAFE_RANGE_MIN || varValues[i] > SAFE_RANGE_MAX) {
      newAlarmState = true;
      break; 
    }
  }

  if (newAlarmState != systemAlarmActive) {
    systemAlarmActive = newAlarmState;
    if (systemAlarmActive) {
      tone(BUZZER_PIN, 3000);
    } else {
      noTone(BUZZER_PIN);
    }
  }

  unsigned long currentTime = millis();
  if (currentTime - lastBlinkTime >= ALARM_BLINK_RATE) {
    lastBlinkTime = currentTime;
    ledState = !ledState;
    for (int i = 0; i < NUM_VARS; i++) {
      updateLEDs(i);
    }
  }
}

void updateLEDs(int varIndex) {
  int value = varValues[varIndex];
  bool inAlarmZone = (value < SAFE_RANGE_MIN || value > SAFE_RANGE_MAX);
  int pwmValue = map(value, MIN_VAL, MAX_VAL, 0, 255);

  if (systemAlarmActive && inAlarmZone) {
    analogWrite(LED_PINS[varIndex], ledState ? pwmValue : 0);
  } else {
    analogWrite(LED_PINS[varIndex], pwmValue);
  }
}

// --- SERIAL MONITOR DASHBOARD ---

void printStatusSerial() {
  Serial.print("V1: ");
  Serial.print(varValues[0]);
  Serial.print("% | ");

  Serial.print("V2: ");
  Serial.print(varValues[1]);
  Serial.print("% | ");

  Serial.print("V3: ");
  Serial.print(varValues[2]);
  Serial.print("% | ");

  Serial.print("ALARM: ");
  if (systemAlarmActive) {
    Serial.println("ACTIVE");
  } else {
    Serial.println("SAFE");
  }
}

// --- TFT (ignored if white) ---

void drawHMI() {
  tft.fillScreen(ST7735_DARKGREY);
  tft.fillRect(0, 0, 128, 18, ST7735_NAVY);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(15, 6);
  tft.print("TRIPLE ALARM MONITOR");
}

void drawBar(int varIndex, int value) {
  // empty for serial test, not needed
}
