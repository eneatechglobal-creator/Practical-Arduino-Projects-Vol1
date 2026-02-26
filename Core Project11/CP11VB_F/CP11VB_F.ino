#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Servo.h>
 
// --- CUSTOM COLOR DEFINITION (Fixes the 'ST7735_DARKGREY' compilation error) ---
#define ST7735_DARKGREY 0x39E7 
// ----------------------------------------------------------------------------
 
// --- PIN DEFINITIONS (Arduino Mega 2560) ---
// ST7735 SPI Control Pins
#define TFT_CS 9  
#define TFT_DC 8  
#define TFT_RST 10
 
// Potentiometer and Servo Pins
#define POT_PIN A0
#define SERVO_PIN 3 // PWM pin
 
// --- CONTROL CONSTANTS ---
#define UNLOCKED_ANGLE 90   // Servo angle for safe/unlocked state
#define LOCKED_ANGLE 0      // Servo angle for hazardous/locked state
#define MAX_G_THRESHOLD 2.0 // Max limit user can set (2.0g)
 
// --- LIBRARY INITIALIZATION ---
Adafruit_MPU6050 mpu;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Servo safetyServo;
 
// Global variables for state management
float safeLimit = 0.5; // Initial safety limit
float currentAccX = 0.0;
 
void setup() {
  Serial.begin(115200); // CRITICAL: Baud rate set to 115200
  
  // 1. Servo Setup
  safetyServo.attach(SERVO_PIN);
  safetyServo.write(UNLOCKED_ANGLE);
 
  // 2. TFT Setup
  tft.initR(INITR_144GREENTAB); // Use INITR_REDTAB if the screen is still corrupt
  tft.fillScreen(ST7735_BLACK);
  tft.setRotation(3); // Landscape mode
 
  // 3. MPU6050 Setup
  if (!mpu.begin()) {
    tft.setCursor(0, 0);
    tft.setTextColor(ST7735_RED);
    tft.print("MPU6050 Error!");
    Serial.println("FATAL ERROR: MPU6050 not found!");
    while (1);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G); 
}
 
void loop() {
  // 1. READ SENSORS
  readInputs();
 
  // 2. LOGIC EVALUATION
  checkSafetyThreshold();
 
  // 3. DISPLAY UPDATE
  updateTFTGauge();
 
  delay(50); // Small delay for loop control
}
 
// --- HELPER FUNCTIONS ---
 
void readInputs() {
  // Read MPU6050
sensors_event_t aevent, gevent, tevent;
  mpu.getEvent(&aevent, &gevent, &tevent);
  currentAccX = aevent.acceleration.x;
 
  // Read Potentiometer and map to a g-force threshold
  int potValue = analogRead(POT_PIN);
  safeLimit = map(potValue, 0, 1023, 10, 200) / 100.0;
 
  // --- SERIAL MONITOR OUTPUT ---
  Serial.print("Tilt (AccX): ");
  Serial.print(currentAccX, 2);
  Serial.print(" g | Limit: ");
  Serial.print(safeLimit, 2);
  Serial.print(" g | Status: ");
 
  // Print the status directly to the monitor
  if (abs(currentAccX) >safeLimit) {
    Serial.println("HAZARD - LOCKED");
  } else {
    Serial.println("SAFE - UNLOCKED");
  }
}
 
void checkSafetyThreshold() {
  if (abs(currentAccX) >safeLimit) {
    // TILT HAZARDOUS -> LOCK
    safetyServo.write(LOCKED_ANGLE);
  } else {
    // TILT SAFE -> UNLOCK
    safetyServo.write(UNLOCKED_ANGLE);
  }
}
 
void updateTFTGauge() {
  // TFT Gauge drawing implements Suggestion A (Analog Safety Gauge)
  
  // Constants for gauge drawing (128x128 screen)
  const int gaugeX = 10, gaugeY = 35;
  const int gaugeW = 108, gaugeH = 20;
  
  // 1. Draw Static Background (Full Range -2.0g to +2.0g)
  tft.drawRect(gaugeX, gaugeY, gaugeW, gaugeH, ST7735_WHITE);
  tft.fillRect(gaugeX + 1, gaugeY + 1, gaugeW - 2, gaugeH - 2, ST7735_DARKGREY);
  
  // 2. Draw Live Tilt Bar (AccX)
  // Map currentAccX (-2.0 to 2.0) to pixel width (0 to gaugeW)
  int tiltWidth = map(currentAccX * 100, -200, 200, 0, gaugeW);
  int color = (abs(currentAccX) >safeLimit) ? ST7735_RED : ST7735_GREEN;
  
  // Clear the full inner area first (ensures the bar redraws cleanly)
  tft.fillRect(gaugeX + 1, gaugeY + 1, gaugeW - 2, gaugeH - 2, ST7735_DARKGREY);
  
  // Draw from center (gaugeW/2 = 54) outward
  if (tiltWidth>gaugeW / 2) {
      // Draw from center right to the current tilt position
      tft.fillRect(gaugeX + gaugeW / 2, gaugeY + 1, tiltWidth - gaugeW / 2, gaugeH - 2, color); 
  } else if (tiltWidth<gaugeW / 2) {
      // Draw from the current tilt position up to the center
      tft.fillRect(gaugeX + tiltWidth, gaugeY + 1, gaugeW / 2 - tiltWidth, gaugeH - 2, color); 
  }
  
  // 3. Draw Limit Marker (User-Set Threshold - on the left and right)
  int limitPixels = map(safeLimit * 100, 0, 200, 0, gaugeW / 2); // Map 0-2g to 0-54px
  
  // Clear old markers (by drawing black over them)
  tft.drawFastVLine(gaugeX + limitPixels, gaugeY - 2, gaugeH + 4, ST7735_BLACK);
  tft.drawFastVLine(gaugeX + gaugeW - limitPixels, gaugeY - 2, gaugeH + 4, ST7735_BLACK);
  
  // Draw new markers
  tft.drawFastVLine(gaugeX + limitPixels, gaugeY - 2, gaugeH + 4, ST7735_YELLOW); // Left marker
  tft.drawFastVLine(gaugeX + gaugeW - limitPixels - 1, gaugeY - 2, gaugeH + 4, ST7735_YELLOW); // Right marker
  
  
  // 4. Digital Readouts and Status
  // Line 1: Header
  tft.setCursor(0, 5);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setTextSize(1);
  tft.print("TILT GAUGE: ");
  
  // Line 2: Limit
  tft.setCursor(0, 65);
  tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  tft.print("LIMIT: ");
  tft.setTextSize(2);
  tft.print(safeLimit, 1);
  tft.print("g");
  
  // Line 3: Live AccX
  tft.setCursor(0, 95);
  tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
  tft.print("TILT: ");
  tft.setTextSize(2);
  tft.print(currentAccX, 2);
  tft.print("g");
  
  // Line 4: Servo Status
  tft.setCursor(0, 120);
  tft.setTextColor(color, ST7735_BLACK);
  tft.setTextSize(1);
  if (abs(currentAccX) >safeLimit) {
    tft.print("!! LOCKED - HAZARD !!");
  } else {
    tft.print("SAFE - UNLOCKED");
  }
}
