#pragma once
#include <Arduino.h>
#include "Config.h"

// Sensor layout order in raw[8] and isBlack[8]:
// [0..4]: Center 5 PID sensors (Left -> Right: A6, A5, A4, A3, A2)
// [5]   : Right-most 90° turn sensor (A1)
// [6]   : Left-most 90° turn sensor (A7)
// [7]   : Back / rear reference sensor (A8)
struct LineSensorState {
  uint16_t raw[8];       // Raw ADC sensor readings (0-4095) in order [A6, A5, A4, A3, A2, A1, A7, A8]
  bool isBlack[8];       // True if over black line
  int activeCount;       // Number of center PID sensors currently seeing the line (out of 5)
  float position;        // Normalized PID line position (-2.0 to +2.0, 0 = dead center)
  bool lineFound;        // True if at least 1 center PID sensor detects the line
  bool isDottedGap;      // True if line temporarily dropped (dotted section handling)

  // Auxiliary Navigation Flags:
  bool rightTurn;        // True if A1 (Right Turn sensor) detects black line
  bool leftTurn;         // True if A7 (Left Turn sensor) detects black line
  bool backSensor;       // True if A8 (Back sensor) detects black line
};

// Initialize sensor GPIOs and ADC configuration
void initLineSensors();

// Calibrate white/black thresholds (call during startup routine)
void calibrateLineSensors();

// Read line sensor array and compute centroid / position
LineSensorState readLineSensors();

// Format and print all 8 IR sensor readings (raw analog/digital + binary line status + centroid)
void printLineSensorValues();

// Enable or disable continuous IR sensor printing
void setIRDebugStream(bool enable);
bool isIRDebugStreamEnabled();


