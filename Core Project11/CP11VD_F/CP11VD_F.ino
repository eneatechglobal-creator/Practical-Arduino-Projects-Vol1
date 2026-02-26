#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Servo.h>

// --- HARDWARE PIN DEFINITIONS (Mega 2560) ---
#define MPU_INT_PIN 2    // Connected to MPU6050 INT pin (Mega INT0)
#define SERVO_PIN 3
#define BUZZER_PIN 4

// ST7735 TFT Pins (SPI)
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8

// Initialize Libraries
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Adafruit_MPU6050 mpu;
Servo lockServo;

// --- CONTROL CONSTANTS ---
#define TAP_THRESHOLD_G 1.2 // MODIFIED: Lowered g-force threshold for a recognized tap (1.2g)
#define DEBOUNCE_TIME 500   // ms to ignore subsequent taps after one is detected
const long UNLOCK_TIMEOUT = 5000; // 5 seconds

// --- LOCK STATE MANAGEMENT ---
enum TapType { NONE, LEFT, RIGHT, UP, DOWN };
enum LockState { LOCKED, UNLOCKED };

// The Secret Code Sequence (RIGHT, UP, LEFT)
const TapType LOCK_CODE[] = { RIGHT, UP, LEFT };
const int CODE_LENGTH = 3;

volatile int currentStep = 0; 
LockState lockStatus = LOCKED;
long unlockTime = 0;
volatile long lastTapTime = 0; // Cooldown timer for the tap
volatile bool isTapping = false; // Flag to indicate if we need to process a tap

// --- FUNCTION PROTOTYPES ---
void MPU6050_INT_ISR();
void setupMPU6050();
TapType classifyTap();
void processTap();
void unlock();
void resetLock(bool showDenied);
void playTone(bool success);
void playFinalTone(bool success);
void updateTFT(bool full_redraw, const char* override_status = nullptr);

// -----------------------------------------------------------------
// INTERRUPT SERVICE ROUTINE (ISR)
// -----------------------------------------------------------------
void MPU6050_INT_ISR() {
  if (lockStatus == LOCKED && (millis() - lastTapTime> DEBOUNCE_TIME)) {
isTapping = true; 
lastTapTime = millis();
  }
}

void setup() {
Serial.begin(115200);
Serial.println("\n--- Digital Lockbox: MPU6050 Edition ---");

Wire.begin();

  // Setup I/O
pinMode(BUZZER_PIN, OUTPUT);
lockServo.attach(SERVO_PIN);
lockServo.write(0); // Start LOCKED (0 degrees)
Serial.println("Actuator: Locked (0 deg)");

  // TFT Setup
tft.initR(INITR_144GREENTAB); 
tft.setRotation(1); // Landscape mode
tft.fillScreen(ST7735_BLACK);
Serial.println("Display: Initialized ST7735");

  setupMPU6050();

  // Attach the interrupt function to the pin 
attachInterrupt(digitalPinToInterrupt(MPU_INT_PIN), MPU6050_INT_ISR, RISING);
Serial.println("Interrupt: Attached to Digital Pin 2 (RISING)");

updateTFT(true); // Initial HMI draw
}

void loop() {
  // 1. Process tap outside of ISR
  if (isTapping) {
processTap();
isTapping = false; // Reset flag after processing
  }

  // 2. Check for auto-relock timeout
  if (lockStatus == UNLOCKED) {
      if ((millis() - unlockTime) > UNLOCK_TIMEOUT) {
resetLock(false); 
      } else {
          // Update the relock countdown frequently
updateTFT(false);
      }
  }
}

// -----------------------------------------------------------------
// MPU6050 SETUP AND TAP CLASSIFICATION
// -----------------------------------------------------------------
void setupMPU6050() {
  if (!mpu.begin()) {
Serial.println("MPU6050 ERROR: Not found! Check wiring.");
    while (1);
  }
Serial.println("MPU6050 OK. Configuring motion detection...");

  // MPU6050 Motion Interrupt setup
mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);

  // *** CORRECTED INTERRUPT PIN CONFIGURATION ***
mpu.setInterruptPinPolarity(true);        // Active HIGH
mpu.setInterruptPinLatch(false);         // Disable Latch (Pulse mode)

mpu.setMotionDetectionThreshold(10); // 10 * 2mg = 20mg (Low threshold to trigger ISR)
mpu.setMotionDetectionDuration(1); // 1ms

  // *** CORRECTED INTERRUPT ENABLING ***
mpu.setMotionInterrupt(true); // Enable Motion Interrupt
Serial.print("Tap Threshold (Software): ");
Serial.print(TAP_THRESHOLD_G);
Serial.println("g");
}

/** Custom software logic to classify the tap direction after the ISR fires. */
TapType classifyTap() {
sensors_event_t aevent, gevent, tevent;
mpu.getEvent(&aevent, &gevent, &tevent);

  float accX = aevent.acceleration.x;
  float accY = aevent.acceleration.y;

Serial.print("Raw Acc (X, Y): ");
Serial.print(accX);
Serial.print(", ");
Serial.println(accY);

  // Get absolute max axis acceleration
  float absX = abs(accX);
  float absY = abs(accY);

TapType result = NONE;

  // Check against threshold and determine dominant axis
  if (absX> TAP_THRESHOLD_G || absY> TAP_THRESHOLD_G) {

    if (absX>absY) {
      // X-axis dominant (LEFT or RIGHT)
      if (accX< -TAP_THRESHOLD_G) result = LEFT;
      if (accX> TAP_THRESHOLD_G) result = RIGHT;
    } else {
      // Y-axis dominant (UP or DOWN)
      if (accY< -TAP_THRESHOLD_G) result = UP; // Tap UP 
      if (accY> TAP_THRESHOLD_G) result = DOWN; // Tap DOWN
    }
  }
  return result;
}

// -----------------------------------------------------------------
// FSM AND TAP LOGIC
// -----------------------------------------------------------------

void processTap() {
  // Read and clear motion interrupt source register
mpu.getMotionInterruptStatus(); 

TapType detectedTap = classifyTap();
  const char* tapNames[] = {"NONE", "LEFT", "RIGHT", "UP", "DOWN"};

Serial.print("Detected Tap: ");
Serial.println(tapNames[detectedTap]);
Serial.print("Expected Tap: ");
Serial.println(tapNames[LOCK_CODE[currentStep]]);

  if (detectedTap == LOCK_CODE[currentStep]) {
    // CORRECT TAP: Advance state
currentStep++;
playTone(true); // Success tone
Serial.print("FSM: CORRECT. Step advanced to ");
Serial.println(currentStep);
    if (currentStep == CODE_LENGTH) {
      unlock();
    }
  } else if (detectedTap != NONE) {
    // INCORRECT TAP: Reset FSM
Serial.println("FSM: INCORRECT. Resetting FSM.");
resetLock(true);
  } else {
    // Motion detected, but did not pass the software threshold
Serial.print("FSM: Motion below software threshold (");
Serial.print(TAP_THRESHOLD_G);
Serial.println("g) - Ignored.");
  }

  // Redraw TFT after state change
updateTFT(false);
}

void unlock() {
lockStatus = UNLOCKED;
lockServo.write(90); // UNLOCK
unlockTime = millis();
playFinalTone(true);
currentStep = CODE_LENGTH; // Hold at the unlocked state
Serial.println("LOCK STATUS: UNLOCKED! (90 deg)");
updateTFT(false);
}

void resetLock(bool showDenied) {
lockStatus = LOCKED;
lockServo.write(0); // LOCK
currentStep = 0;

Serial.println("LOCK STATUS: LOCKED (0 deg)");

  if (showDenied) {
playTone(false); // Failure tone
Serial.println("ACCESS DENIED / FAILURE TONE");
    // Brief display of ACCESS DENIED
updateTFT(true, "ACCESS DENIED");
    delay(500); 
  }
updateTFT(true); // Full redraw to reset icons
}

// -----------------------------------------------------------------
// OUTPUT & FEEDBACK
// -----------------------------------------------------------------
void playTone(bool success) {
  if (success) {
    tone(BUZZER_PIN, 1500, 100); 
  } else {
    tone(BUZZER_PIN, 300, 500); 
  }
}

void playFinalTone(bool success) {
  // Extended success jingle
  tone(BUZZER_PIN, 1000, 150); delay(150);
  tone(BUZZER_PIN, 1500, 200);
}

void updateTFT(bool full_redraw, const char* override_status = nullptr) {
  // Draw Icons: RIGHT (R), UP (U), LEFT (L)
  int icon_x[] = {10, 50, 90}; 
  const char* icon_label[] = {"R", "U", "L"};

  if (full_redraw) {
tft.fillScreen(ST7735_BLACK);
tft.setTextSize(2);
tft.setCursor(5, 5);
tft.setTextColor(ST7735_WHITE);
tft.print("DIGITAL LOCKBOX");
  }

  // Draw Sequence Icons and Progress
  for (int i = 0; i< CODE_LENGTH; i++) {
    uint16_t color = (i<currentStep) ? ST7735_GREEN : ST7735_BLUE;
tft.drawRect(icon_x[i], 30, 30, 30, color);
tft.setCursor(icon_x[i] + 8, 38);
tft.setTextColor(ST7735_WHITE);
tft.setTextSize(2);
tft.print(icon_label[i]);
  }

  // Status Message Area (Clears the area for clean update)
tft.fillRect(0, 80, 160, 50, ST7735_BLACK); 
tft.setCursor(5, 90);
tft.setTextSize(2);

  if (override_status) {
tft.setTextColor(ST7735_RED);
tft.print(override_status);
  } else if (lockStatus == UNLOCKED) {
tft.setTextColor(ST7735_YELLOW);
tft.print("ACCESS GRANTED!");
tft.setCursor(5, 110);
tft.setTextSize(1);
tft.print("RELOCKING IN ");
tft.print((UNLOCK_TIMEOUT - (millis() - unlockTime)) / 1000);
tft.print("s");
  } else {
tft.setTextColor(ST7735_CYAN);
tft.print("ENTER CODE");
tft.setCursor(5, 110);
tft.setTextSize(1);
tft.print("STEP: ");
tft.print(currentStep + 1);
tft.print(" OF 3");
}
}
