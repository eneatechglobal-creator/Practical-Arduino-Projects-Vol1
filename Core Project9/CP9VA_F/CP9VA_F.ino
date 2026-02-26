#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Keypad.h>

// -------- TFT PINS --------
#define TFT_CS   10
#define TFT_DC   9
#define TFT_RST  8

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// -------- KEYPAD --------
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {25, 24, 23, 22};
byte colPins[COLS] = {26, 27, 28, 29};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// -------- I/O --------
#define PIR_PIN 7
#define LED_A_PIN 3
#define LED_B_PIN 4

// -------- STATES --------
enum SystemState { SLEEP, ACTIVE, MENU, LOCKOUT, STATUS_DISPLAY };
SystemState currentState = SLEEP;
SystemState lastState = SLEEP;

// -------- SECURITY --------
const String ACCESS_CODE = "8888";
String inputBuffer = "";
int failedAttempts = 0;
const int MAX_FAILED_ATTEMPTS = 3;

// -------- DEVICE STATES --------
bool ledA_state = LOW;
bool ledB_state = LOW;

// -------- TIMING --------
unsigned long lastMotionTime = 0;
unsigned long lastPirTrigger = 0;
unsigned long lockoutStartTime = 0;
unsigned long statusDisplayStartTime = 0;
unsigned long failDisplayStartTime = 0;

const long SLEEP_TIMEOUT_MS = 15000;
const long LOCKOUT_DURATION_MS = 5000;
const long STATUS_DISPLAY_DURATION_MS = 2000;
const long FAIL_DISPLAY_DURATION_MS = 1500;
const long PIR_COOLDOWN_MS = 5000;

// -------- FLAGS --------
bool showFailMessage = false;

// -------- SETUP --------
void setup() {
  Serial.begin(115200);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST7735_BLACK);

  pinMode(PIR_PIN, INPUT);
  pinMode(LED_A_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);

  setSystemState(SLEEP);
}

// -------- LOOP --------
void loop() {
  handleInput();
  updateTimers();
}

// -------- INPUT --------
void handleInput() {

  // PIR with cooldown
  if (digitalRead(PIR_PIN) == HIGH) {
    if (millis() - lastPirTrigger > PIR_COOLDOWN_MS) {
      lastPirTrigger = millis();
      lastMotionTime = millis();
      if (currentState == SLEEP) {
        setSystemState(ACTIVE);
      }
    }
  }

  char key = keypad.getKey();
  if (key != NO_KEY) {
    lastMotionTime = millis();

    if (currentState == ACTIVE) {
      if (key >= '0' && key <= '9') {
        if (inputBuffer.length() < ACCESS_CODE.length()) {
          inputBuffer += key;
          drawActiveScreen();
        }
      } 
      else if (key == '#') {
        if (inputBuffer == ACCESS_CODE) {
          setSystemState(MENU);
          failedAttempts = 0;
        } else {
          failedAttempts++;
          showFailMessage = true;
          failDisplayStartTime = millis();
          inputBuffer = "";
          drawActiveScreen();
          if (failedAttempts >= MAX_FAILED_ATTEMPTS) {
            setSystemState(LOCKOUT);
          }
        }
      }
      else if (key == '*') {
        inputBuffer = "";
        drawActiveScreen();
      }
    }

    else if (currentState == MENU) {
      if (key == '1') toggleLED(LED_A_PIN, ledA_state);
      if (key == '2') toggleLED(LED_B_PIN, ledB_state);
      if (key == '*') setSystemState(SLEEP);
    }
  }
}

// -------- TIMERS --------
void updateTimers() {

  if (currentState != SLEEP && currentState != LOCKOUT) {
    if (millis() - lastMotionTime > SLEEP_TIMEOUT_MS) {
      setSystemState(SLEEP);
    }
  }

  if (currentState == LOCKOUT) {
    if (millis() - lockoutStartTime > LOCKOUT_DURATION_MS) {
      setSystemState(SLEEP);
    }
  }

  if (currentState == STATUS_DISPLAY) {
    if (millis() - statusDisplayStartTime > STATUS_DISPLAY_DURATION_MS) {
      setSystemState(MENU);
    }
  }

  if (showFailMessage) {
    if (millis() - failDisplayStartTime > FAIL_DISPLAY_DURATION_MS) {
      showFailMessage = false;
      drawActiveScreen();
    }
  }
}

// -------- STATE CONTROL --------
void setSystemState(SystemState newState) {
  currentState = newState;
  tft.fillScreen(ST7735_BLACK);

  if (newState == SLEEP) {
    inputBuffer = "";
    digitalWrite(LED_A_PIN, LOW);
    digitalWrite(LED_B_PIN, LOW);
    ledA_state = LOW;
    ledB_state = LOW;
    drawSleepScreen();
  }

  if (newState == ACTIVE) drawActiveScreen();
  if (newState == MENU) drawMenuScreen();

  if (newState == LOCKOUT) {
    lockoutStartTime = millis();
    drawLockoutScreen();
  }

  if (newState == STATUS_DISPLAY) {
    statusDisplayStartTime = millis();
    drawStatusScreen();
  }
}

// -------- UI DRAW FUNCTIONS --------
void drawSleepScreen() {
  tft.setTextSize(2);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(10, 20);
  tft.print("SYSTEM ARMED");

  tft.setTextSize(1);
  tft.setCursor(10, 60);
  tft.print("Waiting for motion...");
}

void drawActiveScreen() {
  tft.fillScreen(ST7735_BLACK);

  if (showFailMessage) {
    tft.setTextSize(2);
    tft.setTextColor(ST7735_RED);
    tft.setCursor(10, 20);
    tft.print("ACCESS FAILED");
    return;
  }

  tft.setTextSize(2);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(10, 10);
  tft.print("MOTION DETECTED");

  tft.setTextSize(1);
  tft.setCursor(10, 40);
  tft.print("Enter code:");

  tft.setCursor(10, 60);
  for (int i = 0; i < inputBuffer.length(); i++) {
    tft.print('*');
  }
  tft.print("_");
}

void drawMenuScreen() {
  tft.setTextSize(2);
  tft.setTextColor(ST7735_CYAN);
  tft.setCursor(10, 10);
  tft.print("CONTROL MENU");

  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(10, 40);
  tft.print("1. LED A: ");
  tft.print(ledA_state ? "ON" : "OFF");

  tft.setCursor(10, 60);
  tft.print("2. LED B: ");
  tft.print(ledB_state ? "ON" : "OFF");

  tft.setCursor(10, 90);
  tft.setTextColor(ST7735_RED);
  tft.print("* Logout");
}

void drawStatusScreen() {
  tft.setTextSize(2);
  tft.setTextColor(ST7735_GREEN);
  tft.setCursor(10, 10);
  tft.print("STATUS");

  tft.setTextSize(1);
  tft.setCursor(10, 40);
  tft.print("LED A: ");
  tft.print(ledA_state ? "ON" : "OFF");

  tft.setCursor(10, 60);
  tft.print("LED B: ");
  tft.print(ledB_state ? "ON" : "OFF");
}

void drawLockoutScreen() {
  tft.setTextSize(2);
  tft.setTextColor(ST7735_RED);
  tft.setCursor(10, 20);
  tft.print("LOCKED OUT");

  tft.setTextSize(1);
  tft.setCursor(10, 60);
  tft.print("Wait 5 seconds");
}

// -------- LED --------
void toggleLED(int pin, bool &state) {
  state = !state;
  digitalWrite(pin, state);
  setSystemState(STATUS_DISPLAY);
}
