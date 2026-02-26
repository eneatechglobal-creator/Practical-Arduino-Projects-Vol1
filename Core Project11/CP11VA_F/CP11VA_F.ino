#include <Wire.h>
#include <Adafruit_MPU6050.h> // Success library
#include <Adafruit_Sensor.h>   // Required by MPU6050 library
#include <LiquidCrystal.h>
#include <Stepper.h>           // For ULN2003 Driver

// --- HARDWARE PIN DEFINITIONS (SAFE MEGA PLAN) ---

// ================= LCD 1602 A Parallel Pins (Safe Mega Plan) =================
// Pins: RS, E, D4, D5, D6, D7 (D7 is on pin 7, D6 is on pin 8)
LiquidCrystal lcd(12, 11, 10, 9, 8, 7); 

// ================= ULN2003 Stepper Driver Pins (Sequential D3-D6) =================
#define IN1 3
#define IN2 4
#define IN3 5
#define IN4 6
const int stepsPerRevolution = 2048; 
// Use the common, corrected wiring order (IN1, IN3, IN2, IN4)
Stepper myStepper(stepsPerRevolution, IN1, IN3, IN2, IN4); 

// ================= LED Pins (Using high Mega pins) =================
#define RED_LED 22    // Tilt Right / CCW Indicator
#define GREEN_LED 23  // Tilt Left / CW Indicator

// ================= MPU6050 IMU Initialization =================
Adafruit_MPU6050 mpu; // Success chip

// --- CONTROL CONSTANTS ---
const float TILT_THRESHOLD = 1.5; // g-force threshold to trigger motion
const int FIXED_STEPS = 500;      // Fixed number of steps for each correction pulse
const int MOTOR_SPEED_RPM = 10;   // Set speed in RPM for the ULN2003 motor

// ================= HELPER FUNCTION (LCD Update) =================
void show(const char* msg) {
lcd.setCursor(0, 1);
lcd.print("               "); // Clear line 2 completely
lcd.setCursor(0, 1);
lcd.print(msg);
}

void setup() {
Serial.begin(115200);
Serial.println("--- CP11V-A: Directional Stepper Controller (MPU6050) ---");
Serial.print("Threshold set: "); Serial.print(TILT_THRESHOLD); Serial.println("g\n");

  // 1. PIN SETUP (LEDs)
pinMode(RED_LED, OUTPUT);
pinMode(GREEN_LED, OUTPUT);

  // 2. STEPPER SETUP
myStepper.setSpeed(MOTOR_SPEED_RPM);

  // 3. LCD SETUP
lcd.begin(16, 2);
lcd.print("TILT CONTROLLER");
lcd.setCursor(0, 1);
lcd.print("STANDBY");

  // 4. SENSOR SETUP
  if (!mpu.begin()) {
Serial.println("FATAL ERROR: MPU6050 not found at 0x68!");
lcd.clear();
lcd.print("MPU6050 ERROR!");
    while (1); // Lock the program here
  }

  // Optional: Configure ranges (Default ±2g, suitable for this project)
mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
mpu.setGyroRange(MPU6050_RANGE_250_DEG);
mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void loop() {
  // Read sensor events (aevent = accelerometer)
sensors_event_t aevent, gevent, tevent; // Note the space after the _t
mpu.getEvent(&aevent, &gevent, &tevent);
  float accX = aevent.acceleration.x; // We only need the X-axis acceleration

  // Serial debug output
Serial.print("AccX: "); Serial.print(accX, 2); Serial.print("g | State: ");

  // --- MOTOR LOGIC ---
  if (accX> TILT_THRESHOLD) {
    // TILT RIGHT -> Motor runs Anti-Clockwise (LEFT)
myStepper.step(FIXED_STEPS);

    // Feedback
digitalWrite(RED_LED, HIGH);
digitalWrite(GREEN_LED, LOW);
    show("STEPPER: LEFT   ");
Serial.println("TILT RIGHT -> LEFT (CCW)");

  } else if (accX< -TILT_THRESHOLD) {
    // TILT LEFT -> Motor runs Clockwise (RIGHT)
myStepper.step(-FIXED_STEPS);

    // Feedback
digitalWrite(RED_LED, LOW);
digitalWrite(GREEN_LED, HIGH);
    show("STEPPER: RIGHT  ");
Serial.println("TILT LEFT -> RIGHT (CW)");

  } else {
    // STANDBY
digitalWrite(RED_LED, LOW);
digitalWrite(GREEN_LED, LOW);
    show("STEPPER: STANDBY");
Serial.println("STANDBY (Level)");
  }

  delay(150); // Small delay before next reading
}
