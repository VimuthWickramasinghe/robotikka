#pragma once
#include <Arduino.h>
#include "Config.h"

// ==============================================================================
// PID CONFIGURATION & GLOBAL STATE
// ==============================================================================

struct PIDConfig {
  float Kp;            // Proportional gain
  float Ki;            // Integral gain
  float Kd;            // Derivative gain
  int baseSpeed;       // Nominal forward speed (0 - 255)
  int maxSpeed;        // Maximum motor speed limit (0 - 255)
  int minSpeed;        // Minimum motor speed (-255 to 255, allows reverse pivot)
  float maxIntegral;   // Anti-windup limit for integral term
  uint16_t threshold;  // IR black line threshold for analogRead (0 - 4095)
};

// Global PID configuration and tracking variables
extern PIDConfig g_pidConfig;
extern PIDConfig &pidConfig; // Alias for convenience
extern float g_lineLastError;
extern float g_lineIntegral;

// ==============================================================================
// LINE SENSOR DATA TYPES
// ==============================================================================

struct LineSensorState {
  uint16_t raw[LINE_SENSOR_COUNT]; // Raw readings from GPIO2 through GPIO6
  bool isBlack[LINE_SENSOR_COUNT]; // True if over black line
  int activeCount;       // Number of sensors currently seeing the line
  float position;        // Line position (-2 to +2, 0 = center)
  bool lineFound;        // True if at least 1 sensor detects the line
  bool isDottedGap;      // True if line temporarily dropped (dotted section handling)
};

// Global last sensor state snapshot
extern LineSensorState g_lastLineState;

// ==============================================================================
// FUNCTION DECLARATIONS
// ==============================================================================

// Initialize sensor GPIOs and ADC configuration
void initLineSensors();

// Calibrate white/black thresholds (call during startup routine)
void calibrateLineSensors();

// Dedicated FreeRTOS Calibration Task
void TaskCalibration(void *pvParameters);

// Read line sensor array and compute centroid / position
LineSensorState readLineSensors();

// Reset PID integral and derivative tracking
void resetLinePID();
inline void resetPID() { resetLinePID(); }

// Follow line PID routine: reads sensors, calculates PID steering, and drives motors
void follow_line();

// Format and print all five active IR sensor readings and line status
void printLineSensorValues();
