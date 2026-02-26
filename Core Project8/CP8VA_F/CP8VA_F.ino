#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h> // ST7735 library for the final hardware build
#include <SPI.h>
#include <math.h>
#define ST7735_DARKGREY 0x7BEF


// --- TFT PIN DEFINITIONS (ST7735) ---
#define TFT_CS 10 
#define TFT_DC 9
#define TFT_RST 8

// Initialize ST7735 (TFT_RST, TFT_DC, TFT_CS)
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST); 

// --- I/O PIN DEFINITIONS AND CONSTANTS ---
const int JOY_Y_PIN = A0;      // Channel A Control (Up/Down movement)
const int JOY_X_PIN = A1;      // Channel B Control (Left/Right movement)
const int JOY_SEL_PIN = 2;     // Master Reset Button 
const int BUZZER_PIN = 7;      // Buzzer output pin
const int CONTROL_STEP = 5;    // Percentage step size (5% jump per "click")

// --- LED Bar Graph Pins ---
const int LED_PINS[] = {3, 4, 5, 6, 12, A2, A3, A4, A5, 1}; 
const int NUM_SEGMENTS = 10;
const int CLIP_THRESHOLD = 90; 

// --- STATE VARIABLES ---
int masterLevel = 0; 
int auxLevel = 0;    
bool clipping = false;
int prevMasterLevel = 0; 
int prevAuxLevel = 0;    

// --- DIGITAL DEBOUNCE VARIABLES ---
unsigned long lastInputTime = 0;
const int INPUT_DEBOUNCE_MS = 150; 

// --- BUTTON LONG PRESS VARIABLES ---
unsigned long selPressStartTime = 0;
const int LONG_PRESS_DURATION_MS = 500; // 0.5 second press for Channel B reset
bool selWasPressed = false;

// --- TIMING (Non-Blocking) ---
unsigned long lastBuzzerTime = 0;
const int BUZZER_PULSE_MS = 200; 

// --- FUNCTION PROTOTYPES ---
void drawInterface();
void handleInputs();
void updateBarGraph(int level);
void drawCircularGauge(int level);
void buzzerAlert(bool state);
int mapLogarithmic(int value); 

// --- SETUP ---
void setup() {
  Serial.begin(9600); 

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(JOY_SEL_PIN, INPUT_PULLUP);

  for (int i = 0; i < NUM_SEGMENTS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  masterLevel = 0; 
  auxLevel = 0;

  // TFT Init (ST7735)
  tft.initR(INITR_144GREENTAB); 
  tft.setRotation(1); // Landscape (160x128)
  tft.fillScreen(ST7735_BLACK); 
  drawInterface();
}

// --- LOOP ---
void loop() {
  handleInputs();

  if (abs(masterLevel - prevMasterLevel) >= 1) {
    updateBarGraph(masterLevel);
    prevMasterLevel = masterLevel;
  }

  if (abs(auxLevel - prevAuxLevel) >= 1) {
    drawCircularGauge(auxLevel);
    prevAuxLevel = auxLevel;
  }

  clipping = (masterLevel >= CLIP_THRESHOLD) || (auxLevel >= CLIP_THRESHOLD);
  buzzerAlert(clipping);
}

// --- CORE LOGIC FUNCTIONS ---
void handleInputs() {
  int joyY = analogRead(JOY_Y_PIN);
  int joyX = analogRead(JOY_X_PIN);
  int selState = digitalRead(JOY_SEL_PIN); 

  unsigned long currentTime = millis();

  bool levelChanged = false;

  // --- Button State Tracking for Resets (Short Press = A Reset, Long Press = B Reset) ---

  if (selState == LOW) {
    if (!selWasPressed) {
      selPressStartTime = currentTime;
      selWasPressed = true;
    }
  } 
  // Check on button release (HIGH)
  else if (selWasPressed) {
    selWasPressed = false;
    unsigned long pressDuration = currentTime - selPressStartTime;

    if (pressDuration >= LONG_PRESS_DURATION_MS) {
      auxLevel = 0; // LONG PRESS -> RESET CHANNEL B
    } else {
      masterLevel = 0; // SHORT PRESS -> RESET CHANNEL A
    }
    lastInputTime = currentTime;
    levelChanged = true;
    return;
  }

  // --- Stepping Control Logic ---
  if (currentTime - lastInputTime < INPUT_DEBOUNCE_MS) {
    return;
  }

  // Channel A: Stepping Control (Y-Axis)
  if (joyY >= 1000) { // DOWN -> INCREASE
    masterLevel = constrain(masterLevel + CONTROL_STEP, 0, 100);
    levelChanged = true;
  } else if (joyY <= 23) { // UP -> DECREASE
    masterLevel = constrain(masterLevel - CONTROL_STEP, 0, 100);
    levelChanged = true;
  }

  // Channel B: Stepping Control (X-Axis)
  if (joyX <= 23) { // LEFT -> INCREASE
    auxLevel = constrain(auxLevel + CONTROL_STEP, 0, 100);
    levelChanged = true;
  } else if (joyX >= 1000) { // RIGHT -> DECREASE
    auxLevel = constrain(auxLevel - CONTROL_STEP, 0, 100);
    levelChanged = true;
  }

  if (levelChanged) {
    lastInputTime = currentTime;
  }

  // Update TFT numeric values (Coordinates for ST7735)
  tft.fillRect(40, 25, 40, 15, ST7735_BLACK); 
  tft.setCursor(40, 25);
  tft.setTextColor(ST7735_CYAN);
  tft.print(masterLevel);
  tft.print("%");

  tft.fillRect(125, 25, 40, 15, ST7735_BLACK); 
  tft.setCursor(125, 25);
  tft.setTextColor(ST7735_MAGENTA);
  tft.print(auxLevel);
  tft.print("%");
}

int mapLogarithmic(int value) {
    return value;
}

// --- OUTPUT FUNCTIONS ---

void updateBarGraph(int level) {
  // 1. PHYSICAL LED CONTROL
  int segments_on = map(level, 0, 100, 0, NUM_SEGMENTS);
  for (int i = 0; i < NUM_SEGMENTS; i++) {
    digitalWrite(LED_PINS[i], (i < segments_on) ? HIGH : LOW);
  }

  // 2. VIRTUAL TFT BAR GRAPH DRAWING (Coordinates for ST7735)
  const int barX = 10, barY = 50, barW = 80, barH = 20;  
  int filledWidth = map(level, 0, 100, 0, barW);

  tft.fillRect(barX, barY, barW, barH, ST7735_BLACK);

  uint16_t barColor;
  if (level > CLIP_THRESHOLD) {
    barColor = ST7735_RED;
  } else if (level > 70) {
    barColor = ST7735_YELLOW;
  } else {
    barColor = ST7735_GREEN;
  }
  tft.fillRect(barX, barY, filledWidth, barH, barColor);
  tft.drawRect(8, 48, 84, 24, ST7735_WHITE); 
}

void drawCircularGauge(int level) {
  // 360-degree fill (Coordinates for ST7735)
  const int cx = 130; 
  const int cy = 90; 
  const int r = 30; 

  float start_angle = 0.0;     
  float max_sweep = 360.0;     

  float sweep_angle = map(level, 0, 100, 0, max_sweep);
  float end_angle = start_angle + sweep_angle;

  tft.fillCircle(cx, cy, r + 5, ST7735_BLACK); 
  tft.drawCircle(cx, cy, r, ST7735_DARKGREY);

  uint16_t color;
  if (level > 90) {
    color = ST7735_RED;
  } else if (level > 70) {
    color = ST7735_YELLOW;
  } else {
    color = ST7735_GREEN;
  }

  for (int angle = start_angle; angle <= end_angle; angle += 1) {
      float rad = angle * M_PI / 180.0;
      int x_arc = cx + round(r * cos(rad));
      int y_arc = cy + round(r * sin(rad));
      tft.drawLine(cx, cy, x_arc, y_arc, color);
  }
  tft.fillCircle(cx, cy, 5, ST7735_MAGENTA); 
}

void buzzerAlert(bool state) {
  unsigned long currentTime = millis();
  if (state) {
    if (currentTime - lastBuzzerTime >= BUZZER_PULSE_MS) {
      lastBuzzerTime = currentTime;
      if ((currentTime / BUZZER_PULSE_MS) % 2 == 0) { 
        tone(BUZZER_PIN, 1500, BUZZER_PULSE_MS);
      } else {
        noTone(BUZZER_PIN);
      }
    }
  } else {
    noTone(BUZZER_PIN);
  }
}

void drawInterface() {
  tft.fillScreen(ST7735_BLACK);
  tft.setTextSize(2); tft.setTextColor(ST7735_WHITE); tft.setCursor(10, 5);
  tft.print("MIX METER 8A");

  // Channel A (Master)
  tft.setTextSize(1); tft.setTextColor(ST7735_CYAN); tft.setCursor(10, 27); tft.print("A:");

  // Channel B (Aux)
  tft.setTextColor(ST7735_MAGENTA); tft.setCursor(105, 27); tft.print("B:");

  // Bar Graph Box (Visual Aid) - Channel A
  tft.drawRect(8, 48, 84, 24, ST7735_WHITE); 
  tft.setTextColor(ST7735_WHITE); tft.setTextSize(1); tft.setCursor(10, 75);
  tft.print("Master (Short Reset)");

  // Circular Gauge Label - Channel B
  tft.setCursor(100, 120);
  tft.print("Aux (Long Reset)");
}
