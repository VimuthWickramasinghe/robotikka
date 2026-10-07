#pragma once
#include <Arduino.h>
#include "Config.h"

struct LineSensorState {
  uint16_t raw[8];       // Raw sensor readings
  bool isBlack[8];       // True if over black line
  int activeCount;       // Number of sensors currently seeing the line
  float position;        // Normalized line position (-3.5 to +3.5, 0 = center)
  bool lineFound;        // True if at least 1 sensor detects the line
  bool isDottedGap;      // True if line temporarily dropped (dotted section handling)
};

// Initialize sensor GPIOs and ADC configuration
void initLineSensors();

// Calibrate white/black thresholds (call during startup routine)
void calibrateLineSensors();

// Read line sensor array and compute centroid / position
LineSensorState readLineSensors();

