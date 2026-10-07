// Robotikka code v0.2 - FreeRTOS Multi-Core Architecture
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// Wi-Fi credentials for OTA uploads
const char *ssid = "Dialog 4G";
const char *password = "BHTBEH22T64";

#include "Config.h"
#include "MotorDriver.h"
#include "LineSensor.h"
#include "LinePID.h"
#include "ServoControl.h"



// ==============================================================================
// FREERTOS OBJECTS & STATE MANAGEMENT
// ==============================================================================

// Task Handles
TaskHandle_t hTaskTelemetryOTA = NULL;  // Core 0: Wi-Fi, OTA, Button, LED
TaskHandle_t hTaskRobotControl = NULL;  // Core 1: Real-time PID Line/Wall following, Motors

// Mutex for safe Serial and I2C shared access
SemaphoreHandle_t xI2CMutex = NULL;

// Robot operating mode
enum RobotMode {
  ROBOT_IDLE_STOPPED,
  ROBOT_LINE_FOLLOWING,
  ROBOT_WALL_FOLLOWING,
  ROBOT_BOX_PICKUP,
  ROBOT_BOX_DROP
};
volatile RobotMode currentRobotMode = ROBOT_IDLE_STOPPED;

// LED Animation state
enum LedMode {
  LED_OFF,
  LED_IDLE_HEARTBEAT,  // Breathing Purple/Magenta in idle
  LED_CONNECTING,      // Fast blinking while joining Wi-Fi
  LED_OTA_READY,       // Slow pulse (500ms) when OTA is armed
  LED_OTA_FLASHING     // Rapid strobe (40ms) while actively flashing
};
volatile LedMode currentLedMode = LED_IDLE_HEARTBEAT;

// OTA state
volatile bool otaEnabled = false;
uint32_t last_ota_time = 0;

// Button debounce state
int buttonPressCount = 0;
unsigned long firstPressTime = 0;
const unsigned long MULTI_CLICK_WINDOW = 1500;
int lastButtonReading = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// ==============================================================================
// LED DRIVER & ANIMATION
// ==============================================================================

void setLedOutput(bool state, uint8_t r = 0, uint8_t g = 255, uint8_t b = 0) {
  digitalWrite(STATUS_LED_PIN, state ? HIGH : LOW);
#ifdef RGB_BUILTIN
  if (state) {
    neopixelWrite(RGB_BUILTIN, r, g, b);
  } else {
    neopixelWrite(RGB_BUILTIN, 0, 0, 0);
  }
#endif
}

void updateLedAnimation() {
  static unsigned long lastLedUpdate = 0;
  unsigned long now = millis();

  switch (currentLedMode) {
    case LED_OFF:
      setLedOutput(false);
      break;

    case LED_IDLE_HEARTBEAT: {
      // Smooth breathing glow in vibrant Purple / Magenta (2000ms cycle)
      unsigned long cycle = now % 2000;
      float brightness;
      if (cycle < 1000) {
        brightness = (float)cycle / 1000.0f;
      } else {
        brightness = (float)(2000 - cycle) / 1000.0f;
      }
      brightness = brightness * brightness; // Quadratic easing
      setLedOutput(brightness > 0.05f, (uint8_t)(180 * brightness), 0, (uint8_t)(255 * brightness));
      break;
    }

    case LED_CONNECTING: {
      static bool connectToggle = false;
      if (now - lastLedUpdate >= 100) {
        lastLedUpdate = now;
        connectToggle = !connectToggle;
        setLedOutput(connectToggle, 150, 80, 0); // Fast Amber/Yellow
      }
      break;
    }

    case LED_OTA_READY: {
      static bool otaReadyToggle = false;
      if (now - lastLedUpdate >= 500) {
        lastLedUpdate = now;
        otaReadyToggle = !otaReadyToggle;
        setLedOutput(otaReadyToggle, 0, 150, 0); // Steady Green pulse
      }
      break;
    }

    case LED_OTA_FLASHING: {
      static bool flashToggle = false;
      if (now - lastLedUpdate >= 40) {
        lastLedUpdate = now;
        flashToggle = !flashToggle;
        setLedOutput(flashToggle, 200, 0, 100); // Fast Magenta strobe
      }
      break;
    }
  }
}

// ==============================================================================
// OTA & WIRELESS ROUTINES
// ==============================================================================

void setupOTA() {
  Serial.println("\n[OTA] Triple-click detected! Activating Wi-Fi & ArduinoOTA...");
  currentLedMode = LED_CONNECTING;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("[OTA] Connecting to Wi-Fi");
  unsigned long startAttemptTime = millis();
  
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    vTaskDelay(pdMS_TO_TICKS(200));
    Serial.print(".");
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
    Serial.println("[OTA] Ready for wireless firmware upload from Arduino IDE.");
  } else {
    Serial.println("\n[OTA] Wi-Fi connection timed out. Returning to offline mode.");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    currentLedMode = LED_IDLE_HEARTBEAT;
  }
}

void checkTripleClick() {
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    static int stableState = HIGH;
    if (reading != stableState) {
      stableState = reading;
      if (stableState == LOW) { // Active LOW
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

  if (buttonPressCount > 0 && (millis() - firstPressTime > MULTI_CLICK_WINDOW)) {
    buttonPressCount = 0;
  }
}

// ==============================================================================
// FREERTOS TASKS
// ==============================================================================

/**
 * Task 1 (Core 0): Telemetry, OTA, Button, and LED Animations
 * Priority: 1 (Lower than real-time motor/sensor control)
 */
void TaskTelemetryOTA(void *pvParameters) {
  Serial.printf("[RTOS] TaskTelemetryOTA running on Core %d\n", xPortGetCoreID());

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(10); // Run at 100 Hz

  for (;;) {
    checkTripleClick();

    // Check for interactive Servo commands from Serial Monitor
    handleServoSerialCommands();

    if (otaEnabled) {
      ArduinoOTA.handle();
    }

    updateLedAnimation();

    // Periodic heartbeat debug log (every 3 seconds)
    static unsigned long lastDebugPrint = 0;
    if (millis() - lastDebugPrint >= 3000) {
      lastDebugPrint = millis();
      if (otaEnabled) {
        Serial.printf("[DEBUG] Core0 OTA Active on %s | Free Heap: %u bytes\n", 
                      WiFi.localIP().toString().c_str(), ESP.getFreeHeap());
      } else {
        Serial.printf("[DEBUG] Core0 Offline (Competition Mode) | Free Heap: %u bytes\n", 
                      ESP.getFreeHeap());
      }
    }

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

/**
 * Task 2 (Core 1): High-Speed Real-Time Robot Control Loop
 * Priority: 3 (Higher priority, strictly deterministic PID cycle)
 */
void TaskRobotControl(void *pvParameters) {
  Serial.printf("[RTOS] TaskRobotControl running on Core %d\n", xPortGetCoreID());

  // Initialize motor, servo, and sensor subsystems on Core 1
  initMotors();
  initServos();
  initLineSensors();
  initLinePID(45.0f, 0.0f, 25.0f, 160); // Tune Kp, Ki, Kd, baseSpeed

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xControlPeriod = pdMS_TO_TICKS(10); // Strict 100 Hz (10ms) control loop

  for (;;) {
    switch (currentRobotMode) {
      case ROBOT_LINE_FOLLOWING: {
        // Read 8-sensor array & compute position centroid (-3.5 to +3.5)
        LineSensorState lineState = readLineSensors();

        // Update PID steering & drive TB6612 motors
        updateLineFollower(lineState.position, lineState.isDottedGap);
        break;
      }

      case ROBOT_WALL_FOLLOWING: {
        // TOF wall following logic will plug in here
        break;
      }

      case ROBOT_IDLE_STOPPED:
      default: {
        // Safe idle state (motors stopped)
        stopMotors();
        break;
      }
    }

    // Deterministic delay for consistent PID dt
    vTaskDelayUntil(&xLastWakeTime, xControlPeriod);
  }
}

// ==============================================================================
// SETUP & ARDUINO MAIN
// ==============================================================================

void setup() {
  Serial.begin(115200);

  unsigned long serialWaitStart = millis();
  while (!Serial && millis() - serialWaitStart < 3000) {
    delay(10);
  }

  Serial.println("\n=========================================");
  Serial.println("  --- ROBOTIKKA FREERTOS DUAL-CORE BOOT ---");
  Serial.println("=========================================");
  Serial.printf("Chip Model: %s (Rev %d)\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Total Free Heap: %u bytes\n", ESP.getFreeHeap());
  Serial.println("-----------------------------------------");

  // Initialize UI Pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  // Set initial robot mode to Line Following
  currentRobotMode = ROBOT_LINE_FOLLOWING;

  // Competition rule: Wi-Fi starts OFF
  WiFi.mode(WIFI_OFF);

  // Create I2C Mutex
  xI2CMutex = xSemaphoreCreateMutex();

  // Create FreeRTOS Tasks pinned to specific cores:
  // Core 0: Wireless / OTA / LED / User button
  xTaskCreatePinnedToCore(
    TaskTelemetryOTA,     // Task function
    "TaskTelemetryOTA",   // Name
    4096,                 // Stack size (bytes)
    NULL,                 // Parameters
    1,                    // Priority
    &hTaskTelemetryOTA,   // Task handle
    0                     // Core 0
  );

  // Core 1: Robot Real-time Sensor & Motor Control (PID Loop)
  xTaskCreatePinnedToCore(
    TaskRobotControl,     // Task function
    "TaskRobotControl",   // Name
    4096,                 // Stack size (bytes)
    NULL,                 // Parameters
    3,                    // Priority (Higher priority)
    &hTaskRobotControl,   // Task handle
    1                     // Core 1
  );

  Serial.println("[RTOS] FreeRTOS tasks successfully launched!");
  Serial.println("=========================================\n");
}

void loop() {
  // Arduino loop task is left idle or with a sleep yield,
  // as all operations are cleanly governed by FreeRTOS tasks.
  vTaskDelay(pdMS_TO_TICKS(1000));
}


