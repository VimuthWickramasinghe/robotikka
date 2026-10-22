#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

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

// ==============================================================================
// Shared State Flags
// ==============================================================================

extern volatile RunMode g_runMode;


// ==============================================================================
// Functions
// ==============================================================================

void buttonTaskInit();

// Poll & consume a pending BOOT button press (set by the ISR).
// Thread-safe-enough single-consumer pattern used by
// TaskRobotControl during COLOR_DETECT mode.
bool consumeButtonPress();

#endif // GLOBALS_H