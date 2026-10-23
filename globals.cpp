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
TaskHandle_t hTaskCalibration = nullptr;

// ==============================================================================
// Shared State Flags
// ==============================================================================

volatile RunMode g_runMode = MODE_IDLE;

// ==============================================================================
// Common Calibration Global Variables
// ==============================================================================

volatile bool g_calibMoveForwardEnabled = true;
volatile uint8_t g_calibCurrentStep = 0;
volatile bool g_calibStepReady[CALIB_SAMPLE_COUNT] = {false, false, false, false};
uint16_t g_calibSamples[CALIB_SAMPLE_COUNT][LINE_SENSOR_COUNT] = {{0}};
uint16_t g_sensorMin[LINE_SENSOR_COUNT] = {4095, 4095, 4095, 4095, 4095};
uint16_t g_sensorMax[LINE_SENSOR_COUNT] = {0, 0, 0, 0, 0};
uint16_t g_sensorThresholds[LINE_SENSOR_COUNT] = {0, 0, 0, 0, 0};
char g_calibStatusMsg[64] = "Ready: Place robot centered over line";
volatile bool g_calibServerActive = false;
