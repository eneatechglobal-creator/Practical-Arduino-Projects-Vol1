#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <DHT.h>
#include <AccelStepper.h> // Required for ULN2003-type control

// --- HARDWARE PIN DEFINITIONS (Mega 2560) ---
#define DHTPIN 2
#define DHTTYPE DHT11 // *** UPDATED TO DHT11 ***
DHT dht(DHTPIN, DHTTYPE);

// ULN2003 Stepper Driver Pins (4-wire control)
// NOTE: AccelStepper expects these in order IN1, IN3, IN2, IN4 for proper operation (or similar sequence)
#define IN1_PIN 3
#define IN2_PIN 5
#define IN3_PIN 4
#define IN4_PIN 6

// Initialize AccelStepper for 4-pin unipolar motor
// The pin order here is a common working sequence for 28BYJ-48 motors.
AccelStepper stepper(AccelStepper::FULL4WIRE, IN1_PIN, IN3_PIN, IN2_PIN, IN4_PIN);

// Joystick Pins
#define JOYSTICK_SW 7 // Changed to 7 to free up pins 3-6 for ULN2003
#define JOYSTICK_Y A1
#define JOYSTICK_X A0

// ST7735 TFT Pins (Using Mega's dedicated SPI pins 50-52)
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8

// Initialize ST7735 (ST7735_GREENTAB is standard 1.8" type)
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// --- CONTROL VARIABLES ---
float T_Set = 25.0;
const float T_RANGE_MIN = 20.0;
const float T_RANGE_MAX = 30.0;

bool autoMode = true;
long targetSpeed = 0; // Motor speed in steps/second (AccelStepper)

// Stepper Motor Constants (28BYJ-48 is approx 2048 steps/revolution)
const int STEPS_PER_REV = 2048; // Updated for 28BYJ-48
const int MAX_SPEED = 3000; // Max speed in steps/sec (approx 1.5 RPM for 2048 steps/rev)
const int MANUAL_SPEED = 1000; // Manual run speed (approx 0.5 RPM)

// Debouncing and Timed Update Variables
int lastSwitchState = HIGH;
long lastTFTUpdate = 0;
const int TFT_UPDATE_INTERVAL = 500;

int rawX = 512;
int rawY = 512;

void setup() {
Serial.begin(115200);
dht.begin();

  // Setup I/O Pins
pinMode(JOYSTICK_SW, INPUT_PULLUP); // Joystick button

  // Stepper Motor Setup (AccelStepper)
stepper.setMaxSpeed(MAX_SPEED);
stepper.setAcceleration(200); // Set a moderate acceleration

  // TFT Initialization
tft.initR(INITR_GREENTAB); // ST7735 1.8" type
tft.setRotation(1); // Landscape mode (160x128)
tft.fillScreen(ST7735_BLACK);

drawBaseHMI();
}

void loop() {
readInput();

  if (autoMode) {
calculateProportionalControl();
  } else {
handleManualOverride();
  }

runStepperMotor(); // Calls AccelStepper's run method

  if (millis() - lastTFTUpdate>= TFT_UPDATE_INTERVAL) {
updateTFT();
lastTFTUpdate = millis();
  }

  delay(10);
}

// -----------------------------------------------------------------
// INPUT & CONTROL LOGIC
// -----------------------------------------------------------------
void readInput() {
rawY = analogRead(JOYSTICK_Y);
rawX = analogRead(JOYSTICK_X);

  // Map Y-axis to Temperature Setpoint
T_Set = map(rawY, 0, 1023, T_RANGE_MIN * 10, T_RANGE_MAX * 10) / 10.0;
T_Set = constrain(T_Set, T_RANGE_MIN, T_RANGE_MAX);

  // Read Joystick Button for Mode Toggle
  int switchState = digitalRead(JOYSTICK_SW);
  if (switchState == LOW &&lastSwitchState == HIGH) {
autoMode = !autoMode;
    // Reset stepper speed/target when changing mode
targetSpeed = 0;
stepper.stop();
stepper.setCurrentPosition(0);
  }
lastSwitchState = switchState;
}

void calculateProportionalControl() {
  float T_Current = dht.readTemperature(); // DHT11 read

  if (isnan(T_Current)) {
targetSpeed = 0;
    return;
  }

  float Error = T_Current - T_Set;

  if (Error > 0.1) { // Only run if too hot (Positive Error > small threshold)
    // Scale speed from 0 to MAX_SPEED based on Error
    // Error range (0.1 to 5.0) -> Speed range (100 to MAX_SPEED)
targetSpeed = constrain(map(Error * 100, 10, 500, 100, MAX_SPEED), 100, MAX_SPEED);
stepper.setSpeed(targetSpeed); // Set Clockwise direction (Cooling)
  } else {
targetSpeed = 0; // Stop motor
stepper.setSpeed(0);
  }
}

void handleManualOverride() {
  if (rawX< 300) {
targetSpeed = -MANUAL_SPEED; // CCW (Negative speed for reverse direction)
  } else if (rawX> 700) {
targetSpeed = MANUAL_SPEED; // CW
  } else {
targetSpeed = 0;
  }
stepper.setSpeed(targetSpeed);
}

void runStepperMotor() {
stepper.runSpeed(); // AccelStepper non-blocking run
}

// -----------------------------------------------------------------
// TFT VISUALIZATION (ST7735) - ADAPTED FOR 160x128
// -----------------------------------------------------------------

long calculateRPM(long steps_per_sec) {
    if (steps_per_sec == 0) return 0;
    // RPM = (steps/sec * 60) / steps_per_rev
    return (abs(steps_per_sec) * 60) / STEPS_PER_REV;
}

void drawBaseHMI() {
tft.fillScreen(ST7735_BLACK);
tft.setRotation(1); // 160x128 (width x height)

tft.setTextSize(2);
tft.setTextColor(ST7735_WHITE);
tft.setCursor(5, 5);
tft.print("CLIMATE V C");

  // Draw static labels
tft.drawFastHLine(0, 25, 160, ST7735_BLUE);

tft.setCursor(5, 35);
tft.setTextColor(ST7735_MAGENTA, ST7735_BLACK);
tft.print("SET: ");

tft.setCursor(5, 60);
tft.setTextColor(ST7735_RED, ST7735_BLACK);
tft.print("CURR:");

tft.setCursor(5, 85);
tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
tft.print("HUM:");

tft.setCursor(5, 110);
tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
tft.print("RPM:");
}

void updateTFT() {
  float T_Current = dht.readTemperature();
  float H_Current = dht.readHumidity();

  // DHT11 readTemperature/Humidity returns 0 or non-numeric if error.
  if (isnan(T_Current) || isnan(H_Current)) return;

  // 1. SETPOINT/CURRENT TEMP UPDATE
  // SET T (Y-axis pos 35)
tft.setCursor(55, 35);
tft.setTextColor(ST7735_MAGENTA, ST7735_BLACK);
tft.print(T_Set, 1);
tft.print((char)247); tft.print("C  "); // Prints degree symbol

  // CURRENT T (Y-axis pos 60)
tft.setCursor(55, 60);
tft.setTextColor(ST7735_RED, ST7735_BLACK);
tft.print(T_Current, 1);
tft.print((char)247); tft.print("C  "); // Prints degree symbol

  // 2. HUMIDITY UPDATE
tft.setCursor(55, 85);
tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
tft.print(H_Current, 0);
tft.print("%  ");

  // 3. MODE STATUS (Displayed at the top right)
tft.setCursor(95, 5);
tft.setTextSize(1); // Smaller font for status
tft.setTextColor(ST7735_BLACK, ST7735_BLACK);// Clear previous text
tft.print("       ");
tft.setCursor(95, 5);

  if (autoMode) {
tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
tft.print("AUTO");
  } else {
tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
tft.print("MANUAL");
  }
tft.setTextSize(2); // Reset size

  // 4. MOTOR STATUS & RPM UPDATE
  long currentSpeed = stepper.speed();
  long rpm = calculateRPM(currentSpeed);

  // RPM Display (Y-axis pos 110)
tft.setCursor(55, 110);
tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
tft.print(rpm);
tft.print("   ");

  // Motor Status Text (Below RPM)
tft.setCursor(5, 135);
tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  // Clear previous line
tft.print("MOTOR:                       ");
tft.setCursor(5, 135);

  if (currentSpeed == 0) {
tft.print("MOTOR: STOPPED");
  } else if (autoMode) {
    if (currentSpeed> (MAX_SPEED / 2)) {
tft.print("MOTOR: HIGH COOL");
    } else {
tft.print("MOTOR: LOW COOL");
    }
  } else { // Manual mode
    if (currentSpeed> 0) {
tft.print("MOTOR: MANUAL CW");
    } else {
tft.print("MOTOR: MANUAL CCW");
    }
  }

  // DEBUGGING OUTPUT (Smaller font, bottom right)
tft.setTextSize(1);
tft.setCursor(120, 120);
tft.setTextColor(ST7735_ORANGE, ST7735_BLACK);
tft.print(rawX);
tft.print("/");
tft.print(rawY);
}
