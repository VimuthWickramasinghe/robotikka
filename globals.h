#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "Config.h"

// ==============================================================================
// PIN DEFINITIONS
// ==============================================================================

#define BOOT_BUTTON_PIN   0

// ==============================================================================
// Enums
// ==============================================================================

enum RunMode : uint8_t {
    MODE_IDLE,     // No mode selected yet (waiting for button menu)
    LINE_FOLLOW,
    OTA_MODE,
    IR_CALIBRATE,
    COLOR_DETECT,  // Live TCS34725 color sensor detect (command4)
    MOTOR_TEST,    // Motor PWM sweep test (command5)
    SERVO_TEST,    // Servo 30-90deg swing test, servos 1 & 2 (command6)
    TOF_STREAM,    // TOF continuous ranging stream (command8)
    COMMAND4,
    COMMAND5,
    COMMAND6
};

// ==============================================================================
// FreeRTOS Handles (defined in globals.cpp)
// ==============================================================================

extern QueueHandle_t logQueue;
extern QueueHandle_t cmdQueue;
extern QueueHandle_t sensorQueue;

extern SemaphoreHandle_t varMutex;

extern TaskHandle_t hTaskTelemetryOTA;
extern TaskHandle_t hTaskRobotControl;
extern TaskHandle_t hTaskCalibration;

// ==============================================================================
// Shared State Flags
// ==============================================================================

extern volatile RunMode g_runMode;

// ==============================================================================
// Common Calibration Global Variables
// 4 Samples: 0: Middle, 1: Right, 2: Left, 3: Middle while moving forward
// ==============================================================================

#define CALIB_SAMPLE_COUNT 4

extern volatile bool g_calibMoveForwardEnabled;
extern volatile uint8_t g_calibCurrentStep;      // 0 = Ready, 1 = Middle Done, 2 = Right Done, 3 = Left Done, 4 = Forward Done, 5 = Calibrated
extern volatile bool g_calibStepReady[CALIB_SAMPLE_COUNT];
extern uint16_t g_calibSamples[CALIB_SAMPLE_COUNT][LINE_SENSOR_COUNT];
extern uint16_t g_sensorMin[LINE_SENSOR_COUNT];
extern uint16_t g_sensorMax[LINE_SENSOR_COUNT];
extern uint16_t g_sensorThresholds[LINE_SENSOR_COUNT];
extern char g_calibStatusMsg[64];
extern volatile bool g_calibServerActive;

// Wi-Fi Credentials for OTA & Calibration (defined in robotikka.ino)
extern const char *ssid;
extern const char *password;

// ==============================================================================
// Functions
// ==============================================================================

void buttonTaskInit();

// Poll & consume a pending BOOT button press (set by the ISR).
// Thread-safe-enough single-consumer pattern used by test/calib modes.
bool consumeButtonPress();

#endif // GLOBALS_H
