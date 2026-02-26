#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <DHT.h>

// --- CUSTOM COLORS (RGB565) ---
#define ST7735_DARKGREY 0x7BEF
#define ST7735_ORANGE   0xFD20

// Predefined colors from Adafruit_ST7735 library:
// ST7735_BLACK, ST7735_WHITE, ST7735_RED, ST7735_GREEN, ST7735_BLUE, ST7735_CYAN, ST7735_MAGENTA, ST7735_YELLOW

// --- DISPLAY DEFINITIONS (ST7735) ---
// Note: DIN/MOSI (D51) and CLK/SCK (D52) are fixed for Mega SPI
#define TFT_CS    10 // Chip Select
#define TFT_DC    53 // Data/Command (D53)
#define TFT_RST   9  // Reset 
// Initialize ST7735 display 
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// --- SENSOR DEFINITIONS (DHT11) ---
#define DHTPIN    2    // DHT data pin
#define DHTTYPE   DHT11 // Defined for DHT11 sensor
DHT dht(DHTPIN, DHTTYPE);

// --- CONTROL DEFINITIONS ---
const int POT_PIN = A0;    // Potentiometer for threshold setting
const int LED_PIN = 3;     // Visual Alert LED
const int BUZZER_PIN = 4;  // Audio Alert Buzzer

// --- ULN2003 STEPPER DEFINITIONS ---
const int motorPins[] = {5, 6, 7, 8}; 
const int STEP_DELAY_US = 2000; // Delay between step sequences (controls speed)
const float ALERT_RESET_MARGIN = 1.0; // Hysteresis: Fan stops 1.0 degree C below threshold

// --- STATE VARIABLES ---
float currentTempC = 0.0;
float thresholdTempC = 25.0; 
unsigned long lastSensorRead = 0;
const long sensorInterval = 2000; // Read sensor every 2 seconds

// Fan control state
unsigned long lastStepTime = 0;
int stepIndex = 0; // Current position in the sequence
bool fanActive = false; 

// Buzzer timing & state
const long buzzerInterval = 500; // 500ms ON / 500ms OFF
unsigned long lastBuzzerTime = 0;
bool buzzerOn = false;
bool previousFanState = false; // State flag to prevent flickering

// Stepper Full Step Sequence (Forward)
const byte stepSequence[][4] = {
  {HIGH, LOW, LOW, LOW},
  {LOW, HIGH, LOW, LOW},
  {LOW, LOW, HIGH, LOW},
  {LOW, LOW, LOW, HIGH}
};

// --- FUNCTION PROTOTYPES ---
void readSensor();
void readThreshold();
void checkControl();
void runFanStep();
void drawDashboard();
void updateStatusPanel(bool isAlert);
void printTemp(float temp, int x, int y, uint16_t color);

// --- SETUP ---
void setup() {
  Serial.begin(115200);

  // Initialize ST7735 TFT
  tft.initR(INITR_BLACKTAB); 
  tft.setRotation(1); 

  // Initial Draw of static elements
  tft.fillScreen(ST7735_BLACK);
  tft.setTextSize(2); 
  tft.setTextColor(ST7735_CYAN);
  tft.setCursor(5, 5);
  tft.print("HCS CONTROL");
  tft.drawFastHLine(0, 25, tft.width(), ST7735_DARKGREY);
  tft.setTextSize(1);
  tft.setCursor(5, 35);
  tft.print("Temp C: ");
  tft.setCursor(5, 55);
  tft.print("Set Max: ");

  // Initialize DHT
  dht.begin();

  // Configure Pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  for (int i = 0; i < 4; i++) {
    pinMode(motorPins[i], OUTPUT);
    digitalWrite(motorPins[i], LOW); // Ensure motor is off initially
  }
}

// --- LOOP ---
void loop() {
  readThreshold();

  if (millis() - lastSensorRead > sensorInterval) {
    lastSensorRead = millis();
    readSensor();
    drawDashboard(); // Update dynamic numbers
  }

  checkControl();

  // Run stepper motor only when fan is active
  if (fanActive) {
    runFanStep();
  }
}

// --- LOGIC FUNCTIONS ---
void readSensor() {
  float t = dht.readTemperature();
  if (!isnan(t)) currentTempC = t;
}

void readThreshold() {
  int potValue = analogRead(POT_PIN);
  thresholdTempC = map(potValue, 0, 1023, 15, 35);
}

void runFanStep() {
  unsigned long currentTime = micros();
  if (currentTime - lastStepTime >= STEP_DELAY_US) {
    lastStepTime = currentTime;
    for (int i = 0; i < 4; i++) {
      digitalWrite(motorPins[i], stepSequence[stepIndex][i]);
    }
    stepIndex = (stepIndex + 1) % 4; 
  }
}

void stopMotor() {
  for (int i = 0; i < 4; i++) digitalWrite(motorPins[i], LOW);
  stepIndex = 0;
}

void checkControl() {
  if (!fanActive && (currentTempC > thresholdTempC)) fanActive = true; 
  if (fanActive && (currentTempC < (thresholdTempC - ALERT_RESET_MARGIN))) { fanActive = false; stopMotor(); }

  // LED/Buzzer
  if (fanActive) {
    digitalWrite(LED_PIN, HIGH);
    if (millis() - lastBuzzerTime >= buzzerInterval) {
      lastBuzzerTime = millis();
      buzzerOn = !buzzerOn; 
      if (buzzerOn) tone(BUZZER_PIN, 1000, 250); 
      else noTone(BUZZER_PIN);
    }
  } else {
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
  }

  // Display update
  if (fanActive != previousFanState) {
    updateStatusPanel(fanActive);
    previousFanState = fanActive;
  }
}

// --- DISPLAY FUNCTIONS ---
void printTemp(float temp, int x, int y, uint16_t color) {
  tft.setTextColor(color, ST7735_BLACK);
  tft.setCursor(x, y);
  tft.print(temp, 1); 
  tft.print(" deg C"); 
}

void drawDashboard() {
  tft.setTextSize(1);
  tft.fillRect(60, 35, 70, 10, ST7735_BLACK); 
  printTemp(currentTempC, 60, 35, ST7735_YELLOW);

  tft.fillRect(60, 55, 70, 10, ST7735_BLACK); 
  tft.setCursor(60, 55);
  tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
  tft.print(thresholdTempC, 0); 
  tft.print(" deg C"); 
}

void updateStatusPanel(bool isFanRunning) {
  int panel_y = 70;
  tft.fillRect(0, panel_y, tft.width(), tft.height() - panel_y, ST7735_BLACK);
  tft.drawRect(5, panel_y, tft.width() - 10, tft.height() - panel_y - 5, ST7735_DARKGREY);

  tft.setTextSize(1); 
  if (isFanRunning) {
    tft.setTextColor(ST7735_RED);
    tft.setCursor(10, panel_y + 10); 
    tft.print("!! ALERT: OVERHEAT !!");

    tft.setCursor(10, panel_y + 30); 
    tft.setTextColor(ST7735_ORANGE);
    tft.print("Fan Running (Active)");

  } else {
    tft.setTextColor(ST7735_GREEN);
    tft.setCursor(10, panel_y + 10);
    tft.print("SYSTEM: STABLE");

    tft.setCursor(10, panel_y + 30);
    tft.setTextColor(ST7735_WHITE);
    tft.print("Fan OFF | Silent");
  }
}
