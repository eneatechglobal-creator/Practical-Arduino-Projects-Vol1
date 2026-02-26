#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>  // Hardware-specific library for the ST7735

// --- I/O PIN DEFINITIONS (ARDUINO UNO) ---

// ST7735 TFT Pins (Using Hardware SPI)
#define TFT_CS   10  // Chip Select
#define TFT_DC   9   // Data/Command
#define TFT_RST  8   // Reset
// SCK is D13, MOSI is D11 for Uno Hardware SPI
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Sensor Inputs
const int PIR_A_PIN = A0; // Zone A
const int PIR_B_PIN = A1; // Zone B

// LED Outputs
const int LED_A_PIN = 3; // Zone A LED
const int LED_B_PIN = 4; // Zone B LED

// Buzzer Outputs
const int BUZZER_A_PIN = 5;  // Zone A Buzzer
const int BUZZER_B_PIN = 6;  // Zone B Buzzer

// --- ZONE/ALERT DEFINITIONS ---
const int FREQ_A = 1000; 
const int FREQ_B = 500;  
const int BUZZ_DURATION_MS = 150; 
const int BUZZ_PAUSE_MS = 250;

#define ZONE_A_COLOR ST7735_RED
#define ZONE_B_COLOR ST7735_YELLOW
#define NO_ALERT_COLOR ST7735_GREEN
#define BG_COLOR ST7735_BLACK

// --- STATE MANAGEMENT ---
enum AlertState { NO_ALERT, ALERT_A, ALERT_B, DUAL_ALERT };
AlertState currentAlertState = NO_ALERT;
unsigned long lastBuzzTime = 0;
bool isBuzzing = false;
bool isHeaderRed = true;

// --- FUNCTION PROTOTYPES ---
void readSensors();
void updateScreen();
void drawDashboard(AlertState state);
void playAudio(AlertState state);
void stopAudio();

// --- SETUP ---
void setup() {
  Serial.begin(115200);

  tft.initR(INITR_REDTAB); 
  tft.setRotation(1); // Landscape
  tft.fillScreen(BG_COLOR);

  pinMode(PIR_A_PIN, INPUT);
  pinMode(PIR_B_PIN, INPUT);
  pinMode(LED_A_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
  pinMode(BUZZER_A_PIN, OUTPUT);
  pinMode(BUZZER_B_PIN, OUTPUT);

  drawDashboard(NO_ALERT);
}

// --- LOOP ---
void loop() {
  readSensors();
  updateScreen();
  playAudio(currentAlertState); 
}

void readSensors() {
  bool pirA_status = digitalRead(PIR_A_PIN);
  bool pirB_status = digitalRead(PIR_B_PIN);

  AlertState newAlertState = NO_ALERT;
  if (pirA_status && pirB_status) {
    newAlertState = DUAL_ALERT;
  } else if (pirA_status) {
    newAlertState = ALERT_A;
  } else if (pirB_status) {
    newAlertState = ALERT_B;
  } else {
    newAlertState = NO_ALERT;
  }

  if (newAlertState != currentAlertState) {
    currentAlertState = newAlertState;
    stopAudio(); 
    drawDashboard(currentAlertState); 
  }
}

void playAudio(AlertState state) {
  unsigned long currentTime = millis();

  if (state == ALERT_A || state == DUAL_ALERT) {
    if (currentTime - lastBuzzTime > (isBuzzing ? BUZZ_DURATION_MS : BUZZ_PAUSE_MS)) {
      isBuzzing = !isBuzzing;
      lastBuzzTime = currentTime;
      if (isBuzzing) {
        tone(BUZZER_A_PIN, FREQ_A);
      } else {
        noTone(BUZZER_A_PIN);
      }
    }
    if (state == DUAL_ALERT) {
      tone(BUZZER_B_PIN, FREQ_B);
    } else {
      noTone(BUZZER_B_PIN);
    }
  } 
  else if (state == ALERT_B) {
    tone(BUZZER_B_PIN, FREQ_B);
    noTone(BUZZER_A_PIN);
  } 
  else {
    stopAudio();
  }
}

void stopAudio() {
  noTone(BUZZER_A_PIN);
  noTone(BUZZER_B_PIN);
  isBuzzing = false;
  lastBuzzTime = millis();
}

void updateScreen() {
  digitalWrite(LED_A_PIN, (currentAlertState == ALERT_A || currentAlertState == DUAL_ALERT) ? HIGH : LOW);
  digitalWrite(LED_B_PIN, (currentAlertState == ALERT_B || currentAlertState == DUAL_ALERT) ? HIGH : LOW);

  if (currentAlertState == DUAL_ALERT) {
    static unsigned long lastFlashTime = 0;
    if (millis() - lastFlashTime > 500) {
      lastFlashTime = millis();
      tft.fillRect(0, 0, tft.width(), 15, isHeaderRed ? ST7735_RED : BG_COLOR);
      isHeaderRed = !isHeaderRed; 
      tft.setCursor(5, 3);
      tft.setTextColor(ST7735_WHITE);
      tft.setTextSize(1);
      tft.print("!! DUAL BREACH !!");
    }
  }
}

void drawDashboard(AlertState state) {
  tft.fillScreen(BG_COLOR);
  tft.setCursor(5, 3); 
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);

  if (state == DUAL_ALERT) {
    tft.fillRect(0, 0, tft.width(), 15, ST7735_RED);
    tft.print("!! DUAL BREACH !!");
  } else if (state == NO_ALERT) {
    tft.print("  SYSTEM ARMED");
  } else {
    tft.print("  MOTION ALERT");
  }

  int mid_x = tft.width() / 2;
  int panel_y = 18;
  int panel_h = tft.height() - panel_y - 5;
  int zone_w = mid_x - 5;

  // Zone A Panel
  tft.drawRect(3, panel_y, zone_w, panel_h, ST7735_WHITE);
  tft.setCursor(5, panel_y + 3);
  tft.print("ZONE A: ENTRY");
  
  if (state == ALERT_A || state == DUAL_ALERT) {
    tft.fillRect(5, panel_y + 35, zone_w - 4, 20, ZONE_A_COLOR);
  } else {
    tft.fillRect(5, panel_y + 35, zone_w - 4, 20, NO_ALERT_COLOR);
  }

  // Zone B Panel
  tft.drawRect(mid_x + 2, panel_y, zone_w, panel_h, ST7735_WHITE);
  tft.setCursor(mid_x + 5, panel_y + 3);
  tft.print("ZONE B: WINDOWS");

  if (state == ALERT_B || state == DUAL_ALERT) {
    tft.fillRect(mid_x + 4, panel_y + 35, zone_w - 4, 20, ZONE_B_COLOR);
  } else {
    tft.fillRect(mid_x + 4, panel_y + 35, zone_w - 4, 20, NO_ALERT_COLOR);
  }
}