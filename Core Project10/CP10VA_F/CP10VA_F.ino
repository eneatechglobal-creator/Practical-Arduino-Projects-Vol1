#include <Adafruit_GFX.h>       // Core graphics library
#include <Adafruit_ST7735.h>    // Library for the ST7735 display

// --- I/O PIN DEFINITIONS ---
#define POT_PIN A2              // Analog pin for Potentiometer
#define BUZZER_PIN 5            

// TFT Display Connections (SPI - Uno Default)
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8

// LED PIN Array (PINS D2, D3, D4, D7, A0, A1)
const int LED_PINS[] = {2, 3, 4, 7, A0, A1};
const int NUM_POSITIONS = 6;

// --- CONTROL VARIABLES ---
int currentPosition = 0; // 0 = System OFF/Standby
int lastPotValue = 0;
const int POT_DEADBAND = 50; // Required change in Potentiometer value to register a 'step'
const int POT_OFF_THRESHOLD = 50; // Below this value, system is OFF

// --- TFT SETUP (ST7735 is typically 128x160) ---
// Note: Using ST7735 constructor
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);


void setup() {
Serial.begin(9600);

  // *** ST7735 INITIALIZATION ***
tft.initR(INITR_144GREENTAB); // Use correct initialization for your specific ST7735 variant
tft.setRotation(1); // Set rotation to horizontal view (Landscape)
tft.fillScreen(ST7735_BLACK);

pinMode(BUZZER_PIN, OUTPUT);
  for (int i = 0; i< NUM_POSITIONS; i++) {
pinMode(LED_PINS[i], OUTPUT);
digitalWrite(LED_PINS[i], LOW);
  }

lastPotValue = analogRead(POT_PIN);
updateTFT(0, 0); 
}

void loop() {
handlePotentiometer();
delay(10); 
}

void handlePotentiometer() {
  int currentPotValue = analogRead(POT_PIN);
  int delta = currentPotValue - lastPotValue;

  // 1. CHECK FOR SYSTEM OFF STATE
  if (currentPotValue< POT_OFF_THRESHOLD) {
    if (currentPosition != 0) {
setSystemOff();
    }
lastPotValue = currentPotValue;
return;
  }

  // 2. CHECK FOR FORWARD MOVEMENT (CW Turn)
  if (delta > POT_DEADBAND) {
movePosition(1); 
lastPotValue = currentPotValue;
  }

  // 3. CHECK FOR REVERSE MOVEMENT (CCW Turn)
  else if (delta < -POT_DEADBAND) {
movePosition(-1); 
lastPotValue = currentPotValue;
  }
}

void movePosition(int direction) {
  int newPosition = currentPosition + direction;
  int oldPosition = currentPosition;

  if (currentPosition == 0 && direction == 1) {
newPosition = 1;
  } else if (newPosition> NUM_POSITIONS) {
newPosition = 1; // Wrap around to P1
  } else if (newPosition< 1) {
newPosition = NUM_POSITIONS; // Wrap around to P6
  }

  if (newPosition == oldPosition&&oldPosition != 0) return;

currentPosition = newPosition;
updateLEDs(oldPosition, currentPosition);
updateTFT(currentPosition, direction);
playTone(direction);
}

void setSystemOff() {
  int oldPosition = currentPosition;
currentPosition = 0;
updateLEDs(oldPosition, currentPosition);
updateTFT(0, 0);

tone(BUZZER_PIN, 500, 100); delay(150);
tone(BUZZER_PIN, 500, 100); delay(150);
tone(BUZZER_PIN, 500, 100); delay(150);
noTone(BUZZER_PIN);
}

void updateLEDs(int oldPos, int newPos) {
  if (oldPos>= 1 &&oldPos<= NUM_POSITIONS) {
digitalWrite(LED_PINS[oldPos - 1], LOW);
  }
  if (newPos>= 1 &&newPos<= NUM_POSITIONS) {
digitalWrite(LED_PINS[newPos - 1], HIGH);
  }
}

void playTone(int direction) {
  if (direction == 1) { // Forward/Success tone
tone(BUZZER_PIN, 1000, 150); 
  } else if (direction == -1) { // Reverse/Warning tone
tone(BUZZER_PIN, 300, 300);  
  }
}

// --- TFT HMI UPDATE FUNCTION (ADAPTED FOR ST7735 SIZE) ---
void updateTFT(int pos, int direction) {
  // ST7735 is 160 pixels wide in rotation(1)
tft.fillRect(0, 30, 160, 130, ST7735_BLACK); 

  if (pos == 0) {
tft.setCursor(10, 50);
tft.setTextColor(ST7735_YELLOW);
tft.setTextSize(2);
tft.print("STANDBY MODE");
tft.setCursor(5, 80);
tft.setTextSize(1);
tft.print("TURN POT CW TO START P1");
return;
  }

  // Position/Stage Name Display
tft.setCursor(5, 40);
tft.setTextColor(ST7735_WHITE);
tft.setTextSize(2);
tft.print("POS: P");
tft.print(pos);

  const char* descriptions[] = {
    "OFFLINE", "GRAB PART", "MOVE TO WELD", "WELDING A", "WELDING B", "QUALITY CHECK", "PACKAGE"
  };
tft.setCursor(5, 70);
tft.setTextColor(ST7735_CYAN);
tft.setTextSize(1);
tft.print(descriptions[pos]);

  // Flow Direction & Status
tft.setCursor(5, 100);
tft.setTextColor((direction == 1) ? ST7735_GREEN : ST7735_RED);
tft.setTextSize(2);

  if (direction == 1) {
tft.print("-> FORWARD STEP");
  } else if (direction == -1) {
tft.print("<- REVERSE STEP");
  } else {
tft.print("CMD REGISTERED");
  }

  // Active Stage Highlight
  int box_width = 25;
  int box_x = 5 + (pos - 1) * 27;
tft.drawRect(box_x, 130, box_width, 20, ST7735_YELLOW);
tft.setCursor(box_x + 3, 134);
tft.setTextSize(1);
tft.print("P");
tft.print(pos);
}
