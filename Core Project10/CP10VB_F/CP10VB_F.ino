#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h> // Library for the ST7735 display
#include <SPI.h>
#include <math.h>

// Add these lines at the top of your sketch
#define ST7735_NAVY        0x000F
#define ST7735_DARKGREY    0x7BEF
#define ST7735_LIGHTGREY   0xC618
#define ST7735_ORANGE      0xFD20

// --- I/O PIN DEFINITIONS ---
// ST7735 TFT Display Connections (SPI)
#define TFT_RST 8
#define TFT_DC 9
#define TFT_CS 10

#define JOY_X_PIN A2            
#define JOY_Y_PIN A3            
#define JOY_SW_PIN 4            
#define BUZZER_PIN 5
#define LED_X_PIN 2             // Red LED for X movement
#define LED_Y_PIN 3             // Blue LED for Y movement

// --- CONTROL VARIABLES ---
const int JOY_DEADBAND = 40;
const int JOY_CENTER = 512;

// ST7735 in Rotation(1) (Landscape 160x128)
const int MAX_X = 160;
const int MAX_Y = 128;
const int HEADER_HEIGHT = 20; // Reduced header height for ST7735

int currentX = MAX_X / 2;
int currentY = (MAX_Y + HEADER_HEIGHT) / 2; // Center Y, accounting for header
int lastX = currentX;
int lastY = currentY;

// --- RADAR SWEEP VARIABLES ---
int sweepAngle = 0;
int lastSweepX = 0;
int lastSweepY = 0;
// Max safe radius for the 128px height work area
const int SWEEP_RADIUS = 50;

// --- ZONE TRACKING ---
int currentZone = 0;

// --- TFT SETUP ---
// ST7735 Constructor
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Function Prototypes
void initialDisplaySetup();
void resetCoordinates();
void updateLEDs(int dx, int dy);
void updateTFT_HMI(int x, int y);
void drawTargetMarker(int oldX, int oldY, int newX, int newY);
void drawRadarSweep();
int calculateZone(int x, int y);
void playZoneTone(int newZone, int oldZone);


void setup() {
Serial.begin(9600);

  // *** ST7735 INITIALIZATION ***
tft.initR(INITR_144GREENTAB); // Use correct initialization
tft.setRotation(1); // Landscape mode (160 wide x 128 high)
tft.fillScreen(ST7735_BLACK);

pinMode(BUZZER_PIN, OUTPUT);
pinMode(LED_X_PIN, OUTPUT);
pinMode(LED_Y_PIN, OUTPUT);
pinMode(JOY_SW_PIN, INPUT_PULLUP); 

initialDisplaySetup();
resetCoordinates(); 
currentZone = calculateZone(currentX, currentY); 
}

void loop()
{
handleJoystickMovement();
handleButtonReset();
drawRadarSweep(); 
delay(10); 
}

// --- CORE LOGIC FUNCTIONS (Adapted for ST7735) ---

void handleJoystickMovement() {
  int joyX = analogRead(JOY_X_PIN);
  int joyY = analogRead(JOY_Y_PIN);
  int dx = 0;
  int dy = 0;

  // 1. Check X-Axis Movement (Joystick Left/Right)
  if (joyX> JOY_CENTER + JOY_DEADBAND) {
    dx = -1; // If joyX is high, move screen LEFT (X-)
  } else if (joyX< JOY_CENTER - JOY_DEADBAND) {
    dx = 1; // If joyX is low, move screen RIGHT (X+)
  }

  // 2. Check Y-Axis Movement (Joystick Up/Down)
  if (joyY> JOY_CENTER + JOY_DEADBAND) {
dy = -1; // Moving UP (higher value) moves Y coordinate DOWN (screen UP)
  } else if (joyY< JOY_CENTER - JOY_DEADBAND) {
dy = 1; // Moving DOWN (lower value) moves Y coordinate UP (screen DOWN)
  }

  // 3. Update Coordinates and Check Limits
  if (dx != 0 || dy != 0) {
    int newX = currentX + dx;
    int newY = currentY + dy;

    // Clamp coordinates to the usable screen area (0-159 for X, 21-127 for Y)
currentX = constrain(newX, 0, MAX_X - 1);
currentY = constrain(newY, HEADER_HEIGHT + 1, MAX_Y - 1); 

    if (currentX != lastX || currentY != lastY) {

      // Zone Check
      int newZone = calculateZone(currentX, currentY);
      if (newZone != currentZone) {
playZoneTone(newZone, currentZone); 
currentZone = newZone;
      }

updateLEDs(dx, dy);
updateTFT_HMI(currentX, currentY);
drawTargetMarker(lastX, lastY, currentX, currentY); 

lastX = currentX;
lastY = currentY;
    }
  } else {
digitalWrite(LED_X_PIN, LOW);
digitalWrite(LED_Y_PIN, LOW);
  }
}

void handleButtonReset() {
  if (digitalRead(JOY_SW_PIN) == LOW) { 
resetCoordinates();
delay(500); 
  }
}

void resetCoordinates() {
  int CENTER_Y_WORK_AREA = (MAX_Y + HEADER_HEIGHT) / 2;

drawTargetMarker(currentX, currentY, 0, 0); 

currentX = MAX_X / 2;
currentY = CENTER_Y_WORK_AREA;
lastX = currentX;
lastY = currentY;

currentZone = 0;

digitalWrite(LED_X_PIN, HIGH);
digitalWrite(LED_Y_PIN, HIGH);
tone(BUZZER_PIN, 500, 200); delay(250);
noTone(BUZZER_PIN);
digitalWrite(LED_X_PIN, LOW);
digitalWrite(LED_Y_PIN, LOW);

tft.fillRect(0, HEADER_HEIGHT + 1, MAX_X, MAX_Y - HEADER_HEIGHT - 1, ST7735_NAVY);
initialDisplaySetup(); 

tft.fillCircle(currentX, currentY, 3, ST7735_YELLOW);
updateTFT_HMI(currentX, currentY);
}


// --- ZONE & TONE FUNCTIONS ---

int calculateZone(int x, int y) {
  const int CENTER_X = MAX_X / 2;
  const int CENTER_Y = (MAX_Y + HEADER_HEIGHT) / 2;

  long dx = x - CENTER_X;
  long dy = y - CENTER_Y;

  long distanceSq = (dx * dx) + (dy * dy);
  int distance = (int)sqrt(distanceSq);

  // Adapted Zone sizes for the ST7735's smaller screen
  if (distance < 25) return 0; // Inner Zone
  if (distance < 50) return 1; // Middle Zone
  return 2; // Outer Zone (covers everything > 50)
}

void playZoneTone(int newZone, int oldZone) {
  int baseTone;
  int numBeeps;

  if (newZone>oldZone) {
baseTone = 1000;
numBeeps = newZone + 1;
  } else {
baseTone = 500;
numBeeps = oldZone + 1;
  }

  for (int i = 0; i<numBeeps; i++) {
tone(BUZZER_PIN, baseTone, 50); // Shorter beep for ST7735 version
delay(75);
noTone(BUZZER_PIN);
delay(50); 
  }
}


// --- TFT HMI FUNCTIONS (Adapted for ST7735) ---

void updateLEDs(int dx, int dy) {
digitalWrite(LED_X_PIN, (dx != 0) ? HIGH : LOW);
digitalWrite(LED_Y_PIN, (dy != 0) ? HIGH : LOW);
}

void initialDisplaySetup() {
tft.fillRect(0, HEADER_HEIGHT, MAX_X, MAX_Y - HEADER_HEIGHT, ST7735_NAVY); 

  // Title Bar
tft.fillRect(0, 0, MAX_X, HEADER_HEIGHT, ST7735_DARKGREY);
tft.setTextColor(ST7735_WHITE);
tft.setTextSize(1);
tft.setCursor(2, 5);
tft.print("CNC RADAR JOG");

  const int CENTER_X = MAX_X / 2;
  const int CENTER_Y = (MAX_Y + HEADER_HEIGHT) / 2;

  // Draw the Reticle
tft.drawLine(CENTER_X, HEADER_HEIGHT, CENTER_X, MAX_Y, ST7735_DARKGREY); 
tft.drawLine(0, CENTER_Y, MAX_X, CENTER_Y, ST7735_DARKGREY); 

  // Draw Concentric Circles (25 and 50 pixel radius)
tft.drawCircle(CENTER_X, CENTER_Y, 25, ST7735_DARKGREY);
tft.drawCircle(CENTER_X, CENTER_Y, 50, ST7735_DARKGREY);
}

void updateTFT_HMI(int x, int y) {
  // Clear only the coordinate display area in the header (Top Right)
tft.fillRect(85, 0, 75, HEADER_HEIGHT, ST7735_DARKGREY);

tft.setCursor(87, 5);
tft.setTextColor(ST7735_YELLOW);
tft.setTextSize(1);
tft.print("X:");
tft.print(x);
tft.print(" Y:");
  // Remap Y for display: 128 - current Y
tft.print(MAX_Y - y); 
}

void drawTargetMarker(int oldX, int oldY, int newX, int newY) {
  // Clear the old position
  if (oldY> HEADER_HEIGHT) { 
tft.fillRect(oldX - 4, oldY - 4, 8, 8, ST7735_NAVY);
  }

  // Draw the new Target Marker (Flashing)
  if (newY> HEADER_HEIGHT) { 
    if (millis() % 200 < 100) { 
tft.drawCircle(newX, newY, 3, ST7735_RED);
    } else {
tft.drawCircle(newX, newY, 3, ST7735_GREEN);
    }
tft.drawPixel(newX, newY, ST7735_WHITE); 
  }
}

void drawRadarSweep() {
  const int CENTER_X = MAX_X / 2;
  const int CENTER_Y = (MAX_Y + HEADER_HEIGHT) / 2;

  // Erase the last line
  if (lastSweepX != 0 &&lastSweepY> HEADER_HEIGHT) { 
tft.drawLine(CENTER_X, CENTER_Y, lastSweepX, lastSweepY, ST7735_NAVY); 
  }

  float rad = sweepAngle * 0.0174532925;

  int endX = CENTER_X + (int)(SWEEP_RADIUS * cos(rad));
  int endY = CENTER_Y + (int)(SWEEP_RADIUS * sin(rad));

tft.drawLine(CENTER_X, CENTER_Y, endX, endY, ST7735_CYAN);

sweepAngle = (sweepAngle + 5) % 360;

lastSweepX = endX;
lastSweepY = endY;
}
