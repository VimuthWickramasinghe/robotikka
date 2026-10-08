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
#include "globals.h"

// ==============================================================================
// FREERTOS OBJECTS & STATE MANAGEMENT
// ==============================================================================

// Mutex for safe Serial and I2C shared access
SemaphoreHandle_t xI2CMutex = NULL;



// OTA state
volatile bool otaEnabled = false;

// ==============================================================================
// OTA & WIRELESS ROUTINES
// ==============================================================================

void setupOTA() {
  Serial.println("\n[OTA] Button combo detected! Activating Wi-Fi & ArduinoOTA...");

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
        Serial.println("[OTA] Start updating sketch");
      })
      .onEnd([]() {
        Serial.println("\n[OTA] Upload Complete. Rebooting...");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[OTA] Progress: %u%%\n", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]: ", error);
      });

    ArduinoOTA.begin();
    otaEnabled = true;

    // Blink green twice when WiFi connects
    for (int i = 0; i < 2; i++) {
      neopixelWrite(RGB_LED_PIN, 0, 255, 0);
      vTaskDelay(pdMS_TO_TICKS(200));
      neopixelWrite(RGB_LED_PIN, 0, 0, 0);
      vTaskDelay(pdMS_TO_TICKS(200));
    }

    Serial.println("[OTA] Ready for wireless firmware upload from Arduino IDE.");
  } else {
    Serial.println("\n[OTA] Wi-Fi connection timed out. Returning to offline mode.");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }
}

// ==============================================================================
// FREERTOS TASKS
// ==============================================================================

/**
 * Task 1 (Core 0): Telemetry, OTA, and LED Animations
 */
void TaskTelemetryOTA(void *pvParameters) {
  Serial.printf("[DEBUG] TaskTelemetryOTA created, priority 1, stack 4096\n");
  Serial.printf("[DEBUG] TaskTelemetryOTA running on Core %d\n", xPortGetCoreID());

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(10);

  for (;;) {
    if (g_runMode == OTA_MODE && !otaEnabled) {
      setupOTA();
    }

    if (otaEnabled) {
      ArduinoOTA.handle();
    }

    handleServoSerialCommands();

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

/**
 * Task 2 (Core 1): High-Speed Real-Time Robot Control Loop
 */
void TaskRobotControl(void *pvParameters) {
  Serial.printf("[DEBUG] TaskRobotControl created, priority 8, stack 4096\n");
  Serial.printf("[DEBUG] TaskRobotControl running on Core %d\n", xPortGetCoreID());

  initLineSensors();
  initLinePID(45.0f, 0.0f, 25.0f, 160);

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xControlPeriod = pdMS_TO_TICKS(2);

    for (;;) {
    switch (g_runMode) {
      case LINE_FOLLOW: {
        printLineSensorValues();
        break;
      }

      case OTA_MODE:
      default: {
        break;
      }
    }

    vTaskDelayUntil(&xLastWakeTime, xControlPeriod);
  }
}

// ==============================================================================
// SETUP & ARDUINO MAIN
// ==============================================================================

void setup() {
  delay(1000); 
  Serial.begin(115200);
  Serial.flush();

  // DISABLE WIFI BEFORE ANY ADC2 OPERATIONS (REQUIRED FOR ADC2 UNLOCK)
  WiFi.mode(WIFI_OFF);
  Serial.println("[DEBUG] Wi-Fi disabled - ADC2 unlocked for IR sensors");

  Serial.println("\n=========================================");
  Serial.println("  --- ROBOTIKKA FREERTOS DUAL-CORE BOOT ---");
  Serial.println("=========================================");
  Serial.printf("Chip Model: %s (Rev %d)\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Total Free Heap: %u bytes\n", ESP.getFreeHeap());
  Serial.println("-----------------------------------------");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);



  xI2CMutex = xSemaphoreCreateMutex();

  // Initialize button task (suspended until pressed)
  buttonTaskInit();

  // Create TaskRobotControl suspended - will be started by button selection
  xTaskCreatePinnedToCore(
    TaskRobotControl,
    "TaskRobotControl",
    4096,
    NULL,
    8,
    &hTaskRobotControl,
    1
  );
  if (hTaskRobotControl != NULL) {
    vTaskSuspend(hTaskRobotControl);
    Serial.println("[DEBUG] TaskRobotControl suspended");
  }

  // Create TaskTelemetryOTA suspended - will be started by button selection
  xTaskCreatePinnedToCore(
    TaskTelemetryOTA,
    "TaskTelemetryOTA",
    4096,
    NULL,
    1,
    &hTaskTelemetryOTA,
    0
  );
  if (hTaskTelemetryOTA != NULL) {
    vTaskSuspend(hTaskTelemetryOTA);
    Serial.println("[DEBUG] TaskTelemetryOTA suspended");
  }

  Serial.println("[DEBUG] All tasks created and suspended");
  Serial.println("=========================================\n");
  Serial.flush();
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}