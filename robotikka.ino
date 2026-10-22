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
const char *ssid = "HONOR 400 Pro";
const char *password = "lolshiny";

#include "Config.h"
#include "LineSensor.h"
#include "ColorSensor.h"
#include "MotorDriver.h"
#include "TOFSensors.h"
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

  // Deterministically suspend until button selection resumes us (no creation race)
  vTaskSuspend(NULL);

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

  // Deterministically suspend until button selection resumes us (no creation race)
  vTaskSuspend(NULL);

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xControlPeriod = pdMS_TO_TICKS(2);

    for (;;) {
    switch (g_runMode) {
      case LINE_FOLLOW: {
        printLineSensorValues();
        break;
      }

      // Yellow menu selection: live TCS34725 color detector
      case COLOR_DETECT: {
        initColorSensor();

        if (colorSensorFound()) {
          setColorLight(true);   // illumination LED stays ON while reading
          Serial.println("[COLOR] Live color detect active - press BOOT to exit");

          while (true) {
            ColorReading cr;
            bool valid = readColorSensor(cr);

            if (valid) {
              Serial.printf("[COLOR] R:%u G:%u B:%u C:%u -> RGB(%u,%u,%u) | %u K, %u lux\n",
                            cr.r, cr.g, cr.b, cr.c, cr.nR, cr.nG, cr.nB,
                            cr.colorTemp, cr.lux);
              applyColorToLed(cr);
            } else {
              Serial.println("[COLOR] No light (C=0)");
              neopixelWrite(RGB_LED_PIN, 0, 0, 0);
            }

            // One integration period between samples
            vTaskDelay(pdMS_TO_TICKS(COLOR_SAMPLE_PERIOD_MS));

            if (consumeButtonPress()) {
              Serial.println("[COLOR] Live color detect exited");
              setColorLight(false);  // illumination LED off
              break;
            }
          }
        }

        neopixelWrite(RGB_LED_PIN, 0, 0, 0);
        stopMotors();                  // safety: motors coasted on any exit path
        setColorLight(false);          // safety: light off on any exit path

        g_runMode = MODE_IDLE;         // release button-task gating so menu re-opens
        vTaskSuspend(NULL);          // re-suspend until next selection resumes us

        // Resync 2 ms pacing after a long suspension
        xLastWakeTime = xTaskGetTickCount();
        break;
      }

      // Magenta menu selection: motor PWM sweep test
      case MOTOR_TEST: {
        Serial.println("[MOTOR] Pulse-width sweep test active - press BOOT to exit");

        // Varying-PWM sweep (triangle wave): BOTH motors forward,
        // 0 -> 255 -> 0 in steps of 13 every 200 ms
        int pwmVal = 0;
        int pwmDir = 13; // Increments by ~5% of 255 per step

        while (true) {
          pwmVal += pwmDir;
          if (pwmVal >= 255) { pwmVal = 255; pwmDir = -13; }
          if (pwmVal <= 0)   { pwmVal = 0;   pwmDir = 13;  }
          
          setMotorsSpeed(pwmVal);
          // Calculates approximate percentage for the serial log
          Serial.printf("[MOTOR] Sweep BOTH forward @ %d/255 PWM (~%d%%)\n", pwmVal, (pwmVal * 100) / 255);

          vTaskDelay(pdMS_TO_TICKS(350));

          if (consumeButtonPress()) {
            Serial.println("[MOTOR] Sweep test exited");
            break;
          }
        }

        stopMotors();                // motors coast on exit

        g_runMode = MODE_IDLE;       // release button-task gating so menu re-opens
        vTaskSuspend(NULL);          // re-suspend until next selection resumes us

        // Resync 2 ms pacing after a long suspension
        xLastWakeTime = xTaskGetTickCount();
        break;
      }

      // Cyan menu selection: servo 30-90deg swing test (Servos 1 & 2)
      case SERVO_TEST: {
        if (servoSwingInit()) {
          Serial.println("[SERVO] Swing test active - press BOOT to exit");

          while (true) {
            // Both swing servos down to 30deg, hold 1 s
            servoSwingWrite(30);
            Serial.println("[SERVO] S1 & S2 -> 30deg");
            vTaskDelay(pdMS_TO_TICKS(1000));

            if (consumeButtonPress()) {
              Serial.println("[SERVO] Swing test exited");
              break;
            }

            // Both swing servos up to 90deg, hold 1 s
            servoSwingWrite(90);
            Serial.println("[SERVO] S1 & S2 -> 90deg");
            vTaskDelay(pdMS_TO_TICKS(1000));

            if (consumeButtonPress()) {
              Serial.println("[SERVO] Swing test exited");
              break;
            }
          }
        }

        servoSwingDetach();            // park 90deg + release LEDC

        g_runMode = MODE_IDLE;         // release button-task gating so menu re-opens
        vTaskSuspend(NULL);            // re-suspend until next selection resumes us

        // Resync 2 ms pacing after a long suspension
        xLastWakeTime = xTaskGetTickCount();
        break;
      }

      // Orange menu selection: TOF continuous ranging stream
      case TOF_STREAM: {
        Serial.println("[TOF] Continuous stream active - press BOOT to exit");

        while (true) {
          uint16_t mm[TOF_COUNT];
          readTOFContinuous(mm);

          Serial.print("[TOF]");
          for (int i = 0; i < TOF_COUNT; i++) {
            Serial.printf(" %s:", TOF_NAME[i]);
            if (mm[i] == TOF_INVALID_MM) {
              Serial.print("---mm");
            } else {
              Serial.printf("%dmm", (int)mm[i]);
            }
          }
          Serial.println();

          // Continuous period is 50 ms per sensor; poll at 60 ms
          vTaskDelay(pdMS_TO_TICKS(60));

          if (consumeButtonPress()) {
            Serial.println("[TOF] Continuous stream exited");
            break;
          }
        }

        g_runMode = MODE_IDLE;         // release button-task gating so menu re-opens
        vTaskSuspend(NULL);            // re-suspend until next selection resumes us

        // Resync 2 ms pacing after a long suspension
        xLastWakeTime = xTaskGetTickCount();
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

  // TB6612 driver pins + 20 kHz LEDC PWM (coast until commanded)
  initMotors();

  // TOF panel: unique I2C addresses assigned at boot to avoid the 0x29
  // clash (front_right re-addressed to 0x34 first, then XSHUT sensors
  // 0x30-0x33); all 5 enter continuous ranging mode right away
  initTOFSensors();



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
    Serial.println("[DEBUG] TaskTelemetryOTA suspended");
  }

  Serial.println("[DEBUG] All tasks created and suspended");
  Serial.println("=========================================\n");
  Serial.flush();
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}