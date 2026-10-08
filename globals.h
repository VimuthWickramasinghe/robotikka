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
    LINE_FOLLOW,
    OTA_MODE,
    IR_CALIBRATE,
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

#endif // GLOBALS_H