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

// Wi-Fi credentials for OTA uploads (Default fallback + NVM storage)
#include <Preferences.h>
Preferences preferences;

char wifi_ssid[64] = "Dialog 4G";
char wifi_password[64] = "BHTBEH22T64";

#include "Config.h"
#include "MotorDriver.h"
#include "LineSensor.h"
#include "LinePID.h"
#include "ServoControl.h"
#include "globals.h"

// Forward declarations
void loadWiFiCredentials();
void saveWiFiCredentials(const char *new_ssid, const char *new_pass);
void handleSerialCommands();

// ==============================================================================
// FREERTOS OBJECTS & STATE MANAGEMENT
// ==============================================================================

// Mutex for safe Serial and I2C shared access
SemaphoreHandle_t xI2CMutex = NULL;

// OTA state
volatile bool otaEnabled = false;

// ==============================================================================
// NVM WI-FI CREDENTIAL STORAGE
// ==============================================================================

void loadWiFiCredentials() {
  preferences.begin("robotikka_cfg", true); // read-only
  String s = preferences.getString("ssid", "Dialog 4G");
  String p = preferences.getString("pass", "BHTBEH22T64");
  preferences.end();

  strncpy(wifi_ssid, s.c_str(), sizeof(wifi_ssid) - 1);
  wifi_ssid[sizeof(wifi_ssid) - 1] = '\0';
  strncpy(wifi_password, p.c_str(), sizeof(wifi_password) - 1);
  wifi_password[sizeof(wifi_password) - 1] = '\0';

  Serial.printf("[NVM] Loaded Wi-Fi SSID: '%s'\n", wifi_ssid);
}

void saveWiFiCredentials(const char *new_ssid, const char *new_pass) {
  preferences.begin("robotikka_cfg", false); // read-write
  preferences.putString("ssid", new_ssid);
  preferences.putString("pass", new_pass);
  preferences.end();

  strncpy(wifi_ssid, new_ssid, sizeof(wifi_ssid) - 1);
  wifi_ssid[sizeof(wifi_ssid) - 1] = '\0';
  strncpy(wifi_password, new_pass, sizeof(wifi_password) - 1);
  wifi_password[sizeof(wifi_password) - 1] = '\0';

  Serial.println("\n[NVM] >>> Wi-Fi credentials saved to Non-Volatile Memory (NVM) <<<");
  Serial.printf("[NVM] Stored SSID: '%s'\n", wifi_ssid);
}

// ==============================================================================
// OTA & WIRELESS ROUTINES
// ==============================================================================

void setupOTA() {
  Serial.println("\n[OTA] Activating Wi-Fi & ArduinoOTA service...");
  Serial.printf("[OTA] Connecting to SSID: '%s' ...\n", wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 12000) {
    vTaskDelay(pdMS_TO_TICKS(200));
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[OTA] Wi-Fi Connected!");
    Serial.print("[OTA] ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    ArduinoOTA.setHostname("robotikka-s3");
    ArduinoOTA.setPort(3232);

    ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
          type = "sketch";
        } else { // U_SPIFFS
          type = "filesystem";
        }
        Serial.println("\n[OTA] >>> Start updating " + type + " <<<");
      })
      .onEnd([]() {
        Serial.println("\n[OTA] >>> Upload Complete! Rebooting ESP32... <<<");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[OTA Progress]: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("\n[OTA ERROR %u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
      });

    ArduinoOTA.begin();
    otaEnabled = true;

    // Flash RGB LED Green twice to notify user OTA is listening
    for (int i = 0; i < 2; i++) {
      neopixelWrite(RGB_LED_PIN, 0, 255, 0);
      vTaskDelay(pdMS_TO_TICKS(150));
      neopixelWrite(RGB_LED_PIN, 0, 0, 0);
      vTaskDelay(pdMS_TO_TICKS(150));
    }

    Serial.println("[OTA] ArduinoOTA listening on Port 3232. Ready for IDE upload!");
  } else {
    Serial.println("\n[OTA] Wi-Fi connection timed out. Check SSID/Password or router.");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }
}

// ==============================================================================
// SERIAL COMMAND PROCESSOR (Wi-Fi Config, Servos, IR Sensors)
// ==============================================================================

void handleSerialCommands() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  // Check for: Wifi cred "SSID" "Password" or Wifi cred SSID Password
  // Case-insensitive check for prefix
  String lowerLine = line;
  lowerLine.toLowerCase();

  if (lowerLine.startsWith("wifi cred") || lowerLine.startsWith("wificred")) {
    int firstQuote = line.indexOf('"');
    if (firstQuote != -1) {
      int secondQuote = line.indexOf('"', firstQuote + 1);
      int thirdQuote = line.indexOf('"', secondQuote + 1);
      int fourthQuote = line.indexOf('"', thirdQuote + 1);

      if (firstQuote != -1 && secondQuote != -1 && thirdQuote != -1 && fourthQuote != -1) {
        String newSsid = line.substring(firstQuote + 1, secondQuote);
        String newPass = line.substring(thirdQuote + 1, fourthQuote);
        saveWiFiCredentials(newSsid.c_str(), newPass.c_str());
        Serial.println("[WIFI] New credentials saved! Restarting Wi-Fi...");
        WiFi.disconnect(true);
        otaEnabled = false;
        setupOTA();
        return;
      }
    }

    // Space-separated fallback: Wifi cred <ssid> <password>
    int space1 = line.indexOf(' ', 9);
    if (space1 != -1) {
      String newSsid = line.substring(9, space1);
      newSsid.trim();
      String newPass = line.substring(space1 + 1);
      newPass.trim();
      saveWiFiCredentials(newSsid.c_str(), newPass.c_str());
      Serial.println("[WIFI] New credentials saved! Restarting Wi-Fi...");
      WiFi.disconnect(true);
      otaEnabled = false;
      setupOTA();
      return;
    }

    Serial.println("[WIFI] Format error! Use: Wifi cred \"YourSSID\" \"YourPassword\"");
    return;
  }
  else if (lowerLine == "ota") {
    g_runMode = OTA_MODE;
    Serial.println("[MODE] Switched to OTA_MODE via Serial command.");
    if (!otaEnabled) {
      setupOTA();
    }
    return;
  }
  else if (lowerLine == "wifi status") {
    Serial.printf("[WIFI STATUS] Connected: %s | SSID: '%s' | IP: %s\n",
                  WiFi.status() == WL_CONNECTED ? "YES" : "NO",
                  wifi_ssid,
                  WiFi.localIP().toString().c_str());
    return;
  }

  // Pass remaining commands to Servo & Line sensor command handler
  handleServoSerialCommandsWithLine(line);
}

// ==============================================================================
// FREERTOS TASKS
// ==============================================================================

/**
 * Task 1 (Core 0): Telemetry, OTA, and Serial Monitoring
 */
void TaskTelemetryOTA(void *pvParameters) {
  Serial.printf("[DEBUG] TaskTelemetryOTA running on Core %d\n", xPortGetCoreID());

  for (;;) {
    handleSerialCommands();

    if (g_runMode == OTA_MODE && !otaEnabled) {
      setupOTA();
    }

    if (otaEnabled) {
      ArduinoOTA.handle();
    }

    // Short delay to avoid starving other tasks while keeping OTA responsive
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

/**
 * Task 2 (Core 1): High-Speed Real-Time Robot Control Loop
 */
// Helper function: Read front obstacle/box distance via TOF sensor 1 (or mock/analog fallback)
uint16_t getFrontDistanceMM() {
  // Can be plugged into VL53L0X / VL53L1X I2C reading
  // Placeholder returning 999 if no box, or measured mm
  return 999;
}

// Helper function: Detect color of box via Color Sensor (TCS34725 / APDS9960 / analog)
BoxColor readBoxColor() {
  // Color sensor is directed at the box in front
  // Reads R, G, B channels and returns detected dominant color
  // Default to Red if test run
  return BOX_COLOR_RED;
}

/**
 * Task 2 (Core 1): High-Speed Real-Time Robot Control & Competition Mission Loop
 */
void TaskRobotControl(void *pvParameters) {
  Serial.printf("[DEBUG] TaskRobotControl running on Core %d\n", xPortGetCoreID());

  initMotors();
  initServos();
  initLineSensors();
  initLinePID(45.0f, 0.0f, 25.0f, 160);

  // Set initial arm pose: Arm raised, gripper open
  setServoAngle(1, SERVO_ARM_UP);
  setServoAngle(2, SERVO_GRIP_OPEN);

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xControlPeriod = pdMS_TO_TICKS(2);

  unsigned long stageStartTime = millis();

  for (;;) {
    switch (g_runMode) {
      case LINE_FOLLOW: {
        LineSensorState lineState = readLineSensors();

        switch (g_missionStage) {
          // ------------------------------------------------------------------
          // 1. Exit colored start box straight forward
          // ------------------------------------------------------------------
          case STAGE_START_BOX_EXIT: {
            setMotorSpeeds(SPEED_START_EXIT, SPEED_START_EXIT);
            if (millis() - stageStartTime >= TIME_START_EXIT) {
              stopMotors();
              g_missionStage = STAGE_LINE_FOLLOW_TO_BOX;
              Serial.println("[MISSION] Exited start box -> Line Follow to box active");
            }
            break;
          }

          // ------------------------------------------------------------------
          // 2. PID Line following towards the box in curved track zone
          // ------------------------------------------------------------------
          case STAGE_LINE_FOLLOW_TO_BOX: {
            updateLineFollower(lineState.position, lineState.isDottedGap);

            // Check if top front distance sensor identifies box ahead
            uint16_t dist = getFrontDistanceMM();
            if (dist <= BOX_DETECT_DISTANCE_MM) {
              brakeMotors();
              g_missionStage = STAGE_IDENTIFY_COLOR;
              Serial.printf("[MISSION] Box detected at %d mm! Stopping to read color...\n", dist);
            }
            break;
          }

          // ------------------------------------------------------------------
          // 3. Identify box color before gripping
          // ------------------------------------------------------------------
          case STAGE_IDENTIFY_COLOR: {
            stopMotors();
            vTaskDelay(pdMS_TO_TICKS(150)); // Settle robot before optical read
            
            g_detectedBoxColor = readBoxColor();
            const char* colorName = (g_detectedBoxColor == BOX_COLOR_RED)   ? "RED" :
                                    (g_detectedBoxColor == BOX_COLOR_GREEN) ? "GREEN" :
                                    (g_detectedBoxColor == BOX_COLOR_BLUE)  ? "BLUE" : "UNKNOWN";
            Serial.printf("[MISSION] >>> BOX COLOR IDENTIFIED: %s <<<\n", colorName);

            g_missionStage = STAGE_PICKUP_BOX;
            break;
          }

          // ------------------------------------------------------------------
          // 4. Reverse, lower arm, grip box, and raise arm
          // ------------------------------------------------------------------
          case STAGE_PICKUP_BOX: {
            // Reverse a little bit to clear the gripper path
            setMotorSpeeds(-BOX_REVERSE_SPEED, -BOX_REVERSE_SPEED);
            vTaskDelay(pdMS_TO_TICKS(BOX_REVERSE_MS));
            brakeMotors();

            // Lower hand
            Serial.println("[ARM] Lowering arm...");
            setServoAngle(1, SERVO_ARM_DOWN);
            vTaskDelay(pdMS_TO_TICKS(500));

            // Grip box
            Serial.println("[ARM] Gripping box...");
            setServoAngle(2, SERVO_GRIP_CLOSE);
            vTaskDelay(pdMS_TO_TICKS(400));

            // Raise arm holding box
            Serial.println("[ARM] Raising arm with box...");
            setServoAngle(1, SERVO_ARM_UP);
            vTaskDelay(pdMS_TO_TICKS(600));

            g_missionStage = STAGE_CURVE_FOLLOW_WALL;
            stageStartTime = millis();
            Serial.println("[MISSION] Box acquired -> Starting Curved Wall Following (12 cm from right wall)");
            break;
          }

          // ------------------------------------------------------------------
          // 5. Follow curved wall (12 cm on right side, line ignored)
          // ------------------------------------------------------------------
          case STAGE_CURVE_FOLLOW_WALL: {
            // Mock right TOF distance (or read PIN_TOF_XSHUT2)
            float rightDistance = 120.0f; 
            float wallError = rightDistance - WALL_TARGET_DISTANCE_MM; // target 120 mm
            float wallCorrection = wallError * WALL_KP;

            int leftSpeed  = constrain(WALL_BASE_SPEED + (int)wallCorrection, 0, 200);
            int rightSpeed = constrain(WALL_BASE_SPEED - (int)wallCorrection, 0, 200);
            setMotorSpeeds(leftSpeed, rightSpeed);

            // After curved wall (e.g. timeout or line detected again by center sensors)
            if (millis() - stageStartTime > 4000 && lineState.lineFound) {
              g_missionStage = STAGE_LINE_FOLLOW_JUNCTION;
              Serial.println("[MISSION] Curved wall cleared -> Re-acquired line towards 3-way junction");
            }
            break;
          }

          // ------------------------------------------------------------------
          // 6. Line follow towards 3-way junction
          // ------------------------------------------------------------------
          case STAGE_LINE_FOLLOW_JUNCTION: {
            updateLineFollower(lineState.position, lineState.isDottedGap);

            // Junction detected when wide marker seen (both turn sensors or all 5 center black)
            if (lineState.leftTurn && lineState.rightTurn) {
              brakeMotors();
              g_missionStage = STAGE_BRANCH_TO_DROP;
              Serial.println("[MISSION] 3-Way Junction detected! Deciding path based on color...");
            }
            break;
          }

          // ------------------------------------------------------------------
          // 7. Branch routing based on detected color
          // ------------------------------------------------------------------
          case STAGE_BRANCH_TO_DROP: {
            JunctionAction action;
            if (g_detectedBoxColor == BOX_COLOR_RED) {
              action = ACTION_RED_BOX;   // Default: BRANCH_LEFT
            } else if (g_detectedBoxColor == BOX_COLOR_GREEN) {
              action = ACTION_GREEN_BOX; // Default: BRANCH_RIGHT
            } else {
              action = ACTION_BLUE_BOX;  // Default: BRANCH_STRAIGHT
            }

            if (action == BRANCH_LEFT) {
              Serial.println("[JUNCTION] Turning LEFT (Red Box Destination)...");
              setMotorSpeeds(-120, 150);
              vTaskDelay(pdMS_TO_TICKS(500));
            } else if (action == BRANCH_RIGHT) {
              Serial.println("[JUNCTION] Turning RIGHT (Green Box Destination)...");
              setMotorSpeeds(150, -120);
              vTaskDelay(pdMS_TO_TICKS(500));
            } else {
              Serial.println("[JUNCTION] Proceeding STRAIGHT (Blue Box Destination)...");
              setMotorSpeeds(140, 140);
              vTaskDelay(pdMS_TO_TICKS(400));
            }

            g_missionStage = STAGE_PLACE_BOX;
            break;
          }

          // ------------------------------------------------------------------
          // 8. Place box in matching color circle
          // ------------------------------------------------------------------
          case STAGE_PLACE_BOX: {
            stopMotors();

            // Lower hand
            Serial.println("[ARM] Lowering box to drop zone...");
            setServoAngle(1, SERVO_ARM_DOWN);
            vTaskDelay(pdMS_TO_TICKS(500));

            // Release gripper
            Serial.println("[ARM] Opening gripper...");
            setServoAngle(2, SERVO_GRIP_OPEN);
            vTaskDelay(pdMS_TO_TICKS(400));

            // Raise arm back up
            setServoAngle(1, SERVO_ARM_UP);
            vTaskDelay(pdMS_TO_TICKS(400));

            // Reverse out of circle
            setMotorSpeeds(-110, -110);
            vTaskDelay(pdMS_TO_TICKS(400));
            stopMotors();

            g_missionStage = STAGE_MISSION_COMPLETE;
            Serial.println("[MISSION] >>> MISSION COMPLETE! ROBOT STOPPED. <<<");
            break;
          }

          case STAGE_MISSION_COMPLETE: {
            stopMotors();
            break;
          }
        }
        break;
      }

      case IR_CALIBRATE: {
        stopMotors();
        printLineSensorValues();
        break;
      }

      case OTA_MODE:
      default: {
        stopMotors();
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

  // Load stored Wi-Fi credentials from NVM
  loadWiFiCredentials();

  // Create TaskTelemetryOTA running immediately (active for Serial commands & OTA)
  xTaskCreatePinnedToCore(
    TaskTelemetryOTA,
    "TaskTelemetryOTA",
    4096,
    NULL,
    1,
    &hTaskTelemetryOTA,
    0
  );
  Serial.println("[DEBUG] TaskTelemetryOTA created and active for Serial / OTA monitoring");

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
    Serial.println("[DEBUG] TaskRobotControl suspended (waiting for mission start)");
  }

  // Initialize button task
  buttonTaskInit();

  Serial.println("[DEBUG] Tasks initialized.");
  Serial.println("[HINT] Type 'help' in Serial Monitor for available commands.");
  Serial.println("=========================================\n");
  Serial.flush();
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}