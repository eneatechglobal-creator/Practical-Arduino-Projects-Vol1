#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// --- PIN DEFINITIONS ---
#define TFT_CS 9
#define TFT_DC 8
#define TFT_RST 10
#define BUZZER_PIN 6
#define LED_PIN 7

// --- CONTROL CONSTANTS ---
#define TILT_THRESHOLD 0.8     // Stronger tilt required
#define DEAD_ZONE 0.3         // Neutral zone
#define COMMAND_COOLDOWN 400  // ms
#define AVERAGE_SAMPLES 10

// --- MENU STATE ENUMS ---
enum MenuState { MAIN, SETTINGS, COLORS, INFO };

// --- LIBRARY INITIALIZATION ---
Adafruit_MPU6050 mpu;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// --- GLOBAL STATE VARIABLES ---
MenuState currentMenuState = MAIN;
int cursorPosition = 0;
unsigned long lastCommandTime = 0;
unsigned long lastSerialTime = 0;
const int SERIAL_INTERVAL = 300;

// Smoothing buffer
float x_buffer[AVERAGE_SAMPLES];
float y_buffer[AVERAGE_SAMPLES];
int buffer_index = 0;
float averagedAccX = 0.0;
float averagedAccY = 0.0;

// Redraw control
bool needsRedraw = true;

// Menu definitions
const char* mainMenu[] = {"1. Settings", "2. Colors", "3. Info"};
const char* settingsMenu[] = {"-> Sensitivity", "-> Contrast", "-> Back"};
const char* colorsMenu[] = {"-> Red", "-> Blue", "-> Green", "-> Back"};
const int menuSize[] = {3, 3, 4, 1};

// --- FUNCTION PROTOTYPES ---
void readAndSmoothInputs();
void checkGestures();
void executeSelection(int selection);
void feedbackConfirmation();
void drawMenu();
const char* getMenuStateName(MenuState state);

// -------------------------------------------------

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // TFT
  tft.initR(INITR_144GREENTAB);
  tft.setRotation(3);
  tft.fillScreen(ST7735_BLACK);

  // MPU6050
  if (!mpu.begin()) {
    tft.setTextColor(ST7735_RED);
    tft.println("MPU6050 Error!");
    while (1);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ); // hardware low-pass

  // Initialize smoothing buffers
  for (int i = 0; i < AVERAGE_SAMPLES; i++) {
    x_buffer[i] = 0;
    y_buffer[i] = 0;
  }

  Serial.println("--- Gesture Menu Initialized ---");
  needsRedraw = true;
}

// -------------------------------------------------

void loop() {
  readAndSmoothInputs();

  if (millis() - lastCommandTime > COMMAND_COOLDOWN) {
    checkGestures();
  }

  // Only redraw when needed
  if (needsRedraw) {
    drawMenu();
    needsRedraw = false;
  }

  // Serial debug
  if (millis() - lastSerialTime > SERIAL_INTERVAL) {
    Serial.print("ACC: X=");
    Serial.print(averagedAccX, 2);
    Serial.print(" Y=");
    Serial.print(averagedAccY, 2);
    Serial.print(" Cursor=");
    Serial.println(cursorPosition);
    lastSerialTime = millis();
  }
}

// -------------------------------------------------

void readAndSmoothInputs() {
  sensors_event_t aevent, gevent, tevent;
  mpu.getEvent(&aevent, &gevent, &tevent);

  x_buffer[buffer_index] = aevent.acceleration.x;
  y_buffer[buffer_index] = aevent.acceleration.y;
  buffer_index = (buffer_index + 1) % AVERAGE_SAMPLES;

  float sumX = 0, sumY = 0;
  for (int i = 0; i < AVERAGE_SAMPLES; i++) {
    sumX += x_buffer[i];
    sumY += y_buffer[i];
  }

  averagedAccX = sumX / AVERAGE_SAMPLES;
  averagedAccY = sumY / AVERAGE_SAMPLES;
}

// -------------------------------------------------

void checkGestures() {

  // LEFT
  if (averagedAccX < -TILT_THRESHOLD) {
    cursorPosition = (cursorPosition == 0) ?
      menuSize[currentMenuState] - 1 :
      cursorPosition - 1;

    lastCommandTime = millis();
    needsRedraw = true;
    return;
  }

  // RIGHT
  if (averagedAccX > TILT_THRESHOLD) {
    cursorPosition = (cursorPosition + 1) %
      menuSize[currentMenuState];

    lastCommandTime = millis();
    needsRedraw = true;
    return;
  }

  // SELECT
  if (averagedAccY < -TILT_THRESHOLD) {
    executeSelection(cursorPosition);
    feedbackConfirmation();
    lastCommandTime = millis();
    needsRedraw = true;
    return;
  }

  // BACK
  if (averagedAccY > TILT_THRESHOLD) {
    if (currentMenuState != MAIN) {
      currentMenuState = MAIN;
      cursorPosition = 0;
      feedbackConfirmation();
      lastCommandTime = millis();
      needsRedraw = true;
    }
    return;
  }

  // Dead zone (do nothing)
  if (abs(averagedAccX) < DEAD_ZONE &&
      abs(averagedAccY) < DEAD_ZONE) {
    return;
  }
}

// -------------------------------------------------

void executeSelection(int selection) {
  MenuState oldState = currentMenuState;

  switch (currentMenuState) {
    case MAIN:
      if (selection == 0) currentMenuState = SETTINGS;
      else if (selection == 1) currentMenuState = COLORS;
      else if (selection == 2) currentMenuState = INFO;
      break;

    case SETTINGS:
      if (selection == 2) currentMenuState = MAIN;
      break;

    case COLORS:
      if (selection == 3) currentMenuState = MAIN;
      break;

    case INFO:
      currentMenuState = MAIN;
      break;
  }

  if (currentMenuState != oldState) {
    cursorPosition = 0;
    needsRedraw = true;
  }
}

// -------------------------------------------------

void feedbackConfirmation() {
  digitalWrite(LED_PIN, HIGH);
  tone(BUZZER_PIN, 500, 100);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);
}

// -------------------------------------------------

void drawMenu() {
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(2);
  tft.setCursor(0, 0);

  const char** menuItems;
  int size;
  const char* title;

  switch (currentMenuState) {
    case MAIN:
      menuItems = mainMenu;
      size = menuSize[MAIN];
      title = "MAIN MENU";
      break;

    case SETTINGS:
      menuItems = settingsMenu;
      size = menuSize[SETTINGS];
      title = "SETTINGS";
      break;

    case COLORS:
      menuItems = colorsMenu;
      size = menuSize[COLORS];
      title = "COLORS";
      break;

    case INFO:
      menuItems = mainMenu;
      size = 1;
      title = "INFO";
      break;
  }

  tft.println(title);
  tft.println("----------------");
  tft.setTextSize(1);

  for (int i = 0; i < size; i++) {
    tft.setCursor(0, 40 + i * 15);
    if (i == cursorPosition) {
      tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
      tft.print("> ");
    } else {
      tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
      tft.print("  ");
    }
    tft.println(menuItems[i]);
  }
}

// -------------------------------------------------

const char* getMenuStateName(MenuState state) {
  switch (state) {
    case MAIN: return "MAIN";
    case SETTINGS: return "SETTINGS";
    case COLORS: return "COLORS";
    case INFO: return "INFO";
    default: return "UNKNOWN";
  }
}
