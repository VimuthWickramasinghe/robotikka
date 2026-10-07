// Robotikka code v0.1


#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>

// Wi-Fi credentials for OTA uploads
const char *ssid = "Vimuth_hs";
const char *password = "22345678";
// ==============================================================================
// PIN DEFINITIONS (Extracted from Schematic.pdf - EasyEDA Rev 2.0)
// ==============================================================================

// --- TB6612FNG Dual DC Motor Driver (U6) ---
const int PIN_MOTOR_PWMA  = 8;   // GPIO8:  Motor A PWM Speed Control
const int PIN_MOTOR_AIN1  = 4;   // GPIO4:  Motor A Direction 1
const int PIN_MOTOR_AIN2  = 5;   // GPIO5:  Motor A Direction 2
const int PIN_MOTOR_PWMB  = 9;   // GPIO9:  Motor B PWM Speed Control
const int PIN_MOTOR_BIN1  = 6;   // GPIO6:  Motor B Direction 1
const int PIN_MOTOR_BIN2  = 7;   // GPIO7:  Motor B Direction 2
// Note: STBY (Standby) is hardwired to +3.3V in hardware (always enabled)

// --- Arm & Gripper Servos (U7, U8, U9) ---
const int PIN_SERVO_1     = 15;  // GPIO15 (PWM1): Servo U7
const int PIN_SERVO_2     = 16;  // GPIO16 (PWM2): Servo U8
const int PIN_SERVO_3     = 17;  // GPIO17 (PWM3): Servo U9

// --- Shared I2C Bus (Display H1, Color Sensor H6, TOF Panel H5) ---
const int PIN_I2C_SDA     = 11;  // GPIO11: I2C Serial Data
const int PIN_I2C_SCL     = 12;  // GPIO12: I2C Serial Clock

// --- TOF Sensors Panel (H5) - Shutdown / Enable Pins ---
const int PIN_TOF_XSHUT1  = 13;  // GPIO13: TOF Sensor 1 XSHUT
const int PIN_TOF_XSHUT2  = 14;  // GPIO14: TOF Sensor 2 XSHUT
const int PIN_TOF_XSHUT3  = 10;  // GPIO10: TOF Sensor 3 XSHUT
const int PIN_TOF_XSHUT4  = 48;  // GPIO48: TOF Sensor 4 XSHUT

// --- Color Sensor (H6) ---
const int PIN_COLOR_INT   = 18;  // GPIO18: Color Sensor Interrupt
const int PIN_COLOR_LED   = 21;  // GPIO21: Net "LED" (Color Sensor illumination / Status LED)

// --- 8-Sensor IR Line Following Array (J4) ---
const int PIN_IR_1        = 1;   // GPIO1  (ADC1_CH0)
const int PIN_IR_2        = 2;   // GPIO2  (ADC1_CH1)
const int PIN_IR_3        = 38;  // GPIO38 (Digital)
const int PIN_IR_4        = 39;  // GPIO39 / MTCK (Digital)
const int PIN_IR_5        = 40;  // GPIO40 / MTDO (Digital)
const int PIN_IR_6        = 41;  // GPIO41 / MTDI (Digital)
const int PIN_IR_7        = 42;  // GPIO42 / MTMS (Digital)
const int PIN_IR_8        = 47;  // GPIO47 (Digital)

const int IR_PINS[8] = {
  PIN_IR_1, PIN_IR_2, PIN_IR_3, PIN_IR_4,
  PIN_IR_5, PIN_IR_6, PIN_IR_7, PIN_IR_8
};

// --- User Button & Status Indicator ---
const int BUTTON_PIN      = 0;   // GPIO0: Onboard BOOT button (Active LOW)
const int STATUS_LED_PIN  = 21;  // GPIO21: Net "LED" (Onboard indicator / Color Sensor LED)


// Button triple-press detection state
int buttonPressCount = 0;
unsigned long firstPressTime = 0;
const unsigned long MULTI_CLICK_WINDOW = 1500; // 1.5s window to complete 3 clicks
int lastButtonReading = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// OTA state
bool otaEnabled = false;
uint32_t last_ota_time = 0;

// LED Animation state
enum LedMode {
  LED_OFF,
  LED_IDLE_HEARTBEAT,  // Normal offline robot operation (gentle heartbeat double-blink)
  LED_CONNECTING,      // Fast blinking while joining Wi-Fi
  LED_OTA_READY,       // Slow pulse (500ms) when OTA is armed and waiting for code
  LED_OTA_FLASHING     // Rapid strobe (40ms) while actively flashing sketch
};

LedMode currentLedMode = LED_IDLE_HEARTBEAT;
unsigned long lastLedUpdate = 0;

void setupOTA() {
  Serial.println("\n[OTA] Triple-click detected! Activating Wi-Fi & ArduinoOTA...");
  currentLedMode = LED_CONNECTING;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("[OTA] Connecting to Wi-Fi");
  unsigned long startAttemptTime = millis();
  
  // Timeout after 10 seconds if network is unreachable
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(200);
    Serial.print(".");
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN)); // Fast blink while connecting
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[OTA] Wi-Fi Connected!");
    Serial.print("[OTA] IP Address: ");
    Serial.println(WiFi.localIP());

    ArduinoOTA.setHostname("robotikka-s3");
    ArduinoOTA.setPort(3232);

    ArduinoOTA
      .onStart([]() {
        currentLedMode = LED_OTA_FLASHING;
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.println("[OTA] Start updating " + type);
      })
      .onEnd([]() {
        Serial.println("\n[OTA] Upload Complete. Rebooting...");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        if (millis() - last_ota_time > 500) {
          Serial.printf("[OTA] Progress: %u%%\n", (progress / (total / 100)));
          last_ota_time = millis();
        }
      })
      .onError([](ota_error_t error) {
        currentLedMode = LED_OTA_READY;
        Serial.printf("[OTA] Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
      });

    ArduinoOTA.begin();
    otaEnabled = true;
    currentLedMode = LED_OTA_READY;
    Serial.println("[OTA] Ready for wireless firmware upload from Arduino IDE / PlatformIO.");
  } else {
    Serial.println("\n[OTA] Wi-Fi connection timed out. Returning to offline mode.");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    currentLedMode = LED_IDLE_HEARTBEAT;
  }
}

void checkTripleClick() {
  int reading = digitalRead(BUTTON_PIN);

  // Debounce check
  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    static int stableState = HIGH;
    if (reading != stableState) {
      stableState = reading;
      // Active LOW (pressed)
      if (stableState == LOW) {
        unsigned long now = millis();
        if (buttonPressCount == 0) {
          firstPressTime = now;
          buttonPressCount = 1;
        } else {
          buttonPressCount++;
        }
        Serial.printf("[BUTTON] Click detected (%d/3)\n", buttonPressCount);

        if (buttonPressCount >= 3) {
          buttonPressCount = 0;
          if (!otaEnabled) {
            setupOTA();
          } else {
            Serial.println("[OTA] OTA is already running.");
          }
        }
      }
    }
  }

  lastButtonReading = reading;

  // Reset count if time window expires before reaching 3 presses
  if (buttonPressCount > 0 && (millis() - firstPressTime > MULTI_CLICK_WINDOW)) {
    Serial.println("[BUTTON] Click timeout. Resetting count.");
    buttonPressCount = 0;
  }
}

void setup() {
  Serial.begin(115200);
  
  // Wait up to 3 seconds for Serial Monitor to connect (if USB is plugged in)
  unsigned long serialWaitStart = millis();
  while (!Serial && millis() - serialWaitStart < 3000) {
    delay(10);
  }

  Serial.println("\n=========================================");
  Serial.println("       --- ROBOTIKKA SYSTEM BOOT ---     ");
  Serial.println("=========================================");
  Serial.printf("Chip Model: %s (Rev %d)\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Free Heap: %u bytes\n", ESP.getFreeHeap());
  Serial.println("-----------------------------------------");
  Serial.printf("Button Pin: GPIO %d\n", BUTTON_PIN);
  Serial.printf("Status LED: GPIO %d\n", STATUS_LED_PIN);
  Serial.println("-----------------------------------------");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  // In accordance with competition rules (Section 4.d / 8.a.vi):
  // All wireless communication MUST be terminated before placing on arena.
  // Wi-Fi starts OFF. Only activates when user presses the button 3 times.
  WiFi.mode(WIFI_OFF);
  Serial.println("[System] Wi-Fi is currently OFF (Competition mode).");
  Serial.println("[System] To enable OTA -> Click the button (GPIO 0) 3 times within 1.5s.");
  Serial.println("=========================================\n");
}

void updateLedAnimation() {
  unsigned long now = millis();

  switch (currentLedMode) {
    case LED_OFF:
      digitalWrite(STATUS_LED_PIN, LOW);
      break;

    case LED_IDLE_HEARTBEAT: {
      // Periodic heartbeat double-blink every 1200ms
      unsigned long cycle = now % 1200;
      if (cycle < 80 || (cycle >= 180 && cycle < 260)) {
        digitalWrite(STATUS_LED_PIN, HIGH);
      } else {
        digitalWrite(STATUS_LED_PIN, LOW);
      }
      break;
    }

    case LED_CONNECTING: {
      // Fast toggle every 100ms
      if (now - lastLedUpdate >= 100) {
        lastLedUpdate = now;
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
      }
      break;
    }

    case LED_OTA_READY: {
      // Smooth 500ms ON / 500ms OFF blink to indicate OTA armed
      if (now - lastLedUpdate >= 500) {
        lastLedUpdate = now;
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
      }
      break;
    }

    case LED_OTA_FLASHING: {
      // Hyper-fast strobe (40ms) during active OTA write
      if (now - lastLedUpdate >= 40) {
        lastLedUpdate = now;
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
      }
      break;
    }
  }
}

void loop() {
  checkTripleClick();

  if (otaEnabled) {
    ArduinoOTA.handle();
  }

  updateLedAnimation();

  // Periodic heartbeat debug log (every 3 seconds)
  static unsigned long lastDebugPrint = 0;
  if (millis() - lastDebugPrint >= 3000) {
    lastDebugPrint = millis();
    if (otaEnabled) {
      Serial.printf("[DEBUG] Running | OTA Active on %s | Heap: %u bytes\n", WiFi.localIP().toString().c_str(), ESP.getFreeHeap());
    } else {
      Serial.printf("[DEBUG] Running | Offline Mode (Wi-Fi OFF) | Heap: %u bytes | Press button 3x for OTA\n", ESP.getFreeHeap());
    }
  }

  // --- Normal robot operation (Line following, TOF wall following, arm servos) runs here ---
}

