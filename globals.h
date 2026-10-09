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
// Competition Mission Enums & Shared State
// ==============================================================================

enum BoxColor : uint8_t {
    BOX_COLOR_UNKNOWN,
    BOX_COLOR_RED,
    BOX_COLOR_GREEN,
    BOX_COLOR_BLUE
};

enum MissionStage : uint8_t {
    STAGE_START_BOX_EXIT,     // Drive out of colored start box to reach initial line
    STAGE_LINE_FOLLOW_TO_BOX, // PID Line follow until top front sensor sees box
    STAGE_IDENTIFY_COLOR,     // Color sensor detects Red, Green, or Blue
    STAGE_PICKUP_BOX,         // Reverse a bit, lower arm, grip, raise arm
    STAGE_CURVE_FOLLOW_WALL,  // Follow curved wall on right (12 cm distance, line ignored)
    STAGE_LINE_FOLLOW_JUNCTION, // Re-acquire line, approach 3-way junction
    STAGE_BRANCH_TO_DROP,     // Turn according to box color (Red=Left, Blue=Straight, Green=Right)
    STAGE_PLACE_BOX,          // Lower arm, release gripper, step back
    STAGE_MISSION_COMPLETE    // Stop robot, mission complete
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
// Shared State Flags & Mission Context
// ==============================================================================

extern volatile RunMode g_runMode;
extern volatile BoxColor g_detectedBoxColor;
extern volatile MissionStage g_missionStage;



// ==============================================================================
// Functions
// ==============================================================================

void buttonTaskInit();

#endif // GLOBALS_H