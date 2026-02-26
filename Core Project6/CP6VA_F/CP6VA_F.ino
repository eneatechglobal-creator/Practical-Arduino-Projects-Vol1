#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// --- HARDWARE PIN DEFINITIONS (Arduino Mega 2560) ---
#define TILT_SWITCH_PIN 2      
#define JOYSTICK_Y A0          
#define BUZZER_PIN 4           
#define LED_PIN 5              

// ST7735 TFT Pins (SPI - Mega Defaults)
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// --- CONTROL VARIABLES ---
float currentPitchAngle = 0.0; 
const float tiltErrorRate = 0.4;    
const float correctionGain = 0.4;  // Effective control authority
const float CRITICAL_ANGLE = 20.0;  
const int HALF_HEIGHT = 64;        
const int AIRCRAFT_SIZE = 12;      

// Global variables for HMI optimization
int prevHorizonY = 0; 
int drawCounter = 0;
const int DRAW_INTERVAL = 5; // Draw static elements every 5 loops (250ms)

// --- HMI GRAPHICS COLORS ---
const int AIRCRAFT_COLOR = ST7735_YELLOW;
const int SKY_COLOR = 0x011f;     
const int GROUND_COLOR = 0x9800;  

// Function prototypes 
void readInputsAndCalculatePitch();
void updateAlertStatus();
void drawHorizon();
void drawAircraftAndText();

void setup() {
Serial.begin(115200);

pinMode(TILT_SWITCH_PIN, INPUT_PULLUP);
pinMode(BUZZER_PIN, OUTPUT);
pinMode(LED_PIN, OUTPUT);

  // TFT Setup
tft.initR(INITR_144GREENTAB);
tft.setRotation(1); // Landscape mode (160x128)

  // SCREEN OFFSET FIX: Added for better centering/boundary on ST7735 displays.
tft.setAddrWindow(0, 0, 160, 128); 

  // Initial draw
tft.fillScreen(GROUND_COLOR);
tft.fillRect(0, 0, tft.width(), HALF_HEIGHT, SKY_COLOR);
prevHorizonY = HALF_HEIGHT;
}

void loop() {
readInputsAndCalculatePitch();
updateAlertStatus();
drawHorizon(); // Always draw the dynamic horizon line

  // Draw static elements less frequently to reduce tearing/flicker
  if (drawCounter % DRAW_INTERVAL == 0) {
drawAircraftAndText();
  }
drawCounter++;

  delay(50); // Loop runs at 20 FPS
}

// -----------------------------------------------------------------
// CONTROL LOGIC
// -----------------------------------------------------------------
void readInputsAndCalculatePitch() {

  // Logic for stability when board is FLAT (HIGH means TILTED)
  int isTilted = (digitalRead(TILT_SWITCH_PIN) == HIGH); 

  int rawCorrection = analogRead(JOYSTICK_Y) - 512; 

  // A. Apply Disturbance (Error Injection)
  if (isTilted) {
currentPitchAngle += tiltErrorRate; 
  } else {
    // Self-Stabilization/Damping
    if (abs(currentPitchAngle) > 0.1) {
currentPitchAngle *= 0.99; 
    } else {
currentPitchAngle = 0.0;
    }
  }

  // B. Apply User Correction
currentPitchAngle -= (float)rawCorrection * correctionGain / 512.0;

  // C. Constrain Pitch
currentPitchAngle = constrain(currentPitchAngle, -45.0, 45.0);
}

void updateAlertStatus() {
  if (abs(currentPitchAngle) > CRITICAL_ANGLE) {
    // Critical Alert (STALL)
digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 500); 
  } else {
    // Stable
digitalWrite(LED_PIN, LOW);
noTone(BUZZER_PIN);
  }
}

// -----------------------------------------------------------------
// HMI DRAWING FUNCTIONS
// -----------------------------------------------------------------

// Draws only the dynamic horizon line with flicker optimization
void drawHorizon() {
  int horizonOffset = (int)(currentPitchAngle *(HALF_HEIGHT / 45.0));
  int horizonY = HALF_HEIGHT + horizonOffset;

  // Flicker Reduction Logic: only redraw the old line being uncovered
  if (horizonY != prevHorizonY) {
    if (horizonY<prevHorizonY) {
tft.drawFastHLine(0, prevHorizonY, tft.width(), GROUND_COLOR);
    } else {
tft.drawFastHLine(0, prevHorizonY, tft.width(), SKY_COLOR);
    }
  }

tft.drawFastHLine(0, horizonY, tft.width(), ST7735_WHITE);
prevHorizonY = horizonY;
}

// Draws static (or slowly changing) elements
void drawAircraftAndText() {
  // --- Aircraft Symbol (Fixed Center) ---
  int centerX = tft.width() / 2;
  int centerY = HALF_HEIGHT;

  // Clear and redraw Crosshair
tft.fillRect(centerX - AIRCRAFT_SIZE, centerY - AIRCRAFT_SIZE/2, 2 * AIRCRAFT_SIZE, AIRCRAFT_SIZE, ST7735_BLACK);
tft.drawFastHLine(centerX - AIRCRAFT_SIZE, centerY, 2 * AIRCRAFT_SIZE, AIRCRAFT_COLOR); 
tft.drawFastVLine(centerX, centerY - AIRCRAFT_SIZE/2, AIRCRAFT_SIZE, AIRCRAFT_COLOR);   

  // --- Text Status ---
  // Pitch Angle Display
tft.fillRect(5, 5, 60, 20, ST7735_BLACK);
tft.setCursor(5, 5);
tft.setTextSize(2);
tft.setTextColor(ST7735_WHITE);
tft.print(currentPitchAngle, 0); 
tft.print((char)247); 

  // Critical Status Display
tft.fillRect(5, tft.height() - 15, tft.width() - 10, 10, ST7735_BLACK);
tft.setCursor(5, tft.height() - 15);
tft.setTextSize(1);
  if (abs(currentPitchAngle) > CRITICAL_ANGLE) {
tft.setTextColor(ST7735_RED);
tft.print("CRITICAL PITCH");
  } else {
tft.setTextColor(ST7735_GREEN);
tft.print("STABLE CONTROL");
  }
}
