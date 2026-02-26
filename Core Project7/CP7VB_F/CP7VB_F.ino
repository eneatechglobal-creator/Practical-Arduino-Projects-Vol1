#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <EEPROM.h>

// --- PIN DEFINITIONS (ST7735) ---
#define TFT_CS 10 
#define TFT_DC 9
#define TFT_RST 8
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// --- I/O PIN DEFINITIONS ---
const int JOY_Y_PIN = A0; 
const int JOY_X_PIN = A1; 
const int JOY_SW_PIN = 2; 
const int BUZZER_PIN = 7;
const int LED_PINS[] = {6, 5, 3};

// --- STATE MANAGEMENT ---
const int NUM_MODES = 4;
const char* MODE_NAMES[NUM_MODES] = {"OFF", "PULSE", "BLINK", "WAVE"};
int currentModeIndex = 0; 
int selectedModeIndex = 0; 
int parameterValue = 50; 

const int EEPROM_ADDRESS = 0;

// --- TIMING VARIABLES ---
unsigned long lastStateTime = 0;
unsigned long lastBuzzerTime = 0;
const int BUZZER_DEBOUNCE_MS = 150; 
bool ledState = LOW; 

// --- SETUP ---
void setup() {
  Serial.begin(9600);
  Serial.println("=== MODE WEAVER SERIAL HMI ===");

  pinMode(JOY_SW_PIN, INPUT_PULLUP); 
  pinMode(BUZZER_PIN, OUTPUT);
  for (int i = 0; i < 3; i++) pinMode(LED_PINS[i], OUTPUT);

  // TFT (kept, but optional now)
  tft.initR(INITR_144GREENTAB);
  tft.setRotation(1);
  tft.fillScreen(ST7735_BLACK);

  loadModeFromEEPROM();
  currentModeIndex = selectedModeIndex; 
  drawMenu();

  Serial.print("Boot Mode: ");
  Serial.println(MODE_NAMES[currentModeIndex]);
  Serial.println("Use joystick to control system.");
}

// --- LOOP ---
void loop() {
  handleJoystick();
  updateLEDs(currentModeIndex, parameterValue);
}

// --- EEPROM ---
void loadModeFromEEPROM() {
  int savedIndex = EEPROM.read(EEPROM_ADDRESS);
  if (savedIndex >= 0 && savedIndex < NUM_MODES) {
    selectedModeIndex = savedIndex;
    Serial.print("EEPROM Loaded Mode: ");
    Serial.println(MODE_NAMES[selectedModeIndex]);
  } else {
    selectedModeIndex = 0;
    Serial.println("EEPROM invalid → Default OFF");
  }
}

void saveModeToEEPROM() {
  EEPROM.update(EEPROM_ADDRESS, currentModeIndex);
  Serial.print("EEPROM Saved Mode: ");
  Serial.println(MODE_NAMES[currentModeIndex]);
  buzzerCue(1000, 100); 
}

// --- JOYSTICK ---
void handleJoystick() {
  static unsigned long lastMoveTime = 0;
  static unsigned long buttonPressStartTime = 0;
  const int MOVE_DEBOUNCE_MS = 200; 

  int joyY = analogRead(JOY_Y_PIN);
  int joyX = analogRead(JOY_X_PIN);
  int buttonState = digitalRead(JOY_SW_PIN);
  unsigned long currentTime = millis();

  // Menu scroll
  if (currentTime - lastMoveTime > MOVE_DEBOUNCE_MS) {
    int oldSelected = selectedModeIndex;

    if (joyY < 200)
      selectedModeIndex = (selectedModeIndex + 1) % NUM_MODES;
    else if (joyY > 800)
      selectedModeIndex = (selectedModeIndex - 1 + NUM_MODES) % NUM_MODES;

    if (oldSelected != selectedModeIndex) {
      Serial.print("Selected Mode: ");
      Serial.println(MODE_NAMES[selectedModeIndex]);
      lastMoveTime = currentTime;
    }
  }

  // Parameter
  int newParameterValue = map(joyX, 0, 1023, 0, 100);
  if (abs(newParameterValue - parameterValue) > 1) {
    parameterValue = newParameterValue;
    Serial.print("Parameter = ");
    Serial.println(parameterValue);
  }

  // Button
  if (buttonState == LOW) {
    if(buttonPressStartTime == 0)
      buttonPressStartTime = currentTime;
    else if (currentTime - buttonPressStartTime > 1000) {
      currentModeIndex = selectedModeIndex;
      Serial.print("LONG PRESS → SAVE MODE: ");
      Serial.println(MODE_NAMES[currentModeIndex]);
      saveModeToEEPROM();
      buttonPressStartTime = 0;
    }
  } 
  else if (buttonPressStartTime != 0) {
    if (currentTime - buttonPressStartTime < 1000) {
      currentModeIndex = selectedModeIndex;
      Serial.print("SHORT PRESS → ACTIVATE MODE: ");
      Serial.println(MODE_NAMES[currentModeIndex]);
    }
    buttonPressStartTime = 0;
  }
}

// --- LED ENGINE ---
void updateLEDs(int mode, int param) {
  unsigned long currentTime = millis();
  int speed = map(param, 0, 100, 500, 10); 

  switch (mode) {
    case 0:
      for(int i=0;i<3;i++) analogWrite(LED_PINS[i], 0);
      break;

    case 1: {
      int brightness = (sin(currentTime / (float)speed) + 1.0) * 127.5;
      for(int i=0;i<3;i++) analogWrite(LED_PINS[i], brightness);
      break;
    }

    case 2:
      if (currentTime - lastStateTime >= speed) {
        lastStateTime = currentTime;
        ledState = !ledState;
        int v = ledState ? 255 : 0;
        for(int i=0;i<3;i++) analogWrite(LED_PINS[i], v);
      }
      break;

    case 3:
      if (currentTime - lastStateTime >= speed) {
        lastStateTime = currentTime;
        int activeLED = (lastStateTime / speed) % 3;
        for (int i = 0; i < 3; i++)
          analogWrite(LED_PINS[i], (i == activeLED) ? 255 : 0);
      }
      break;
  }
}

// --- TFT (kept, optional) ---
void drawMenu() {
  Serial.println("=== MENU ===");
  for (int i = 0; i < NUM_MODES; i++) {
    if(i == selectedModeIndex) Serial.print("> ");
    else Serial.print("  ");
    Serial.println(MODE_NAMES[i]);
  }
  Serial.print("ACTIVE: ");
  Serial.println(MODE_NAMES[currentModeIndex]);
  Serial.print("PARAM: ");
  Serial.println(parameterValue);
  Serial.println("==============");
}

void buzzerCue(int duration, int freq) {
  tone(BUZZER_PIN, freq, duration);
}
