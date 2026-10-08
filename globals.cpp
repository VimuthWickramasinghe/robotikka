#include "globals.h"

// ==============================================================================
// FreeRTOS Handles
// ==============================================================================

QueueHandle_t logQueue = nullptr;
QueueHandle_t cmdQueue = nullptr;
QueueHandle_t sensorQueue = nullptr;

SemaphoreHandle_t varMutex = nullptr;

TaskHandle_t hTaskTelemetryOTA = nullptr;
TaskHandle_t hTaskRobotControl = nullptr;

// ==============================================================================
// Shared State Flags
// ==============================================================================

volatile RunMode g_runMode = LINE_FOLLOW;
