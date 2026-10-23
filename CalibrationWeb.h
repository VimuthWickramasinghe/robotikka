#pragma once
#include <Arduino.h>
#include "Config.h"
#include "globals.h"
#include "LineSensor.h"

// ==============================================================================
// CALIBRATION WEB SERVER & 4-SAMPLE SAMPLING INTERFACE
// ==============================================================================

// Start SoftAP ("Robotikka-Calib", IP 192.168.4.1) and Web Server on port 80
void startCalibrationServer();

// Stop Web Server and disable Wi-Fi to maintain offline competition autonomy
void stopCalibrationServer();

// Service HTTP requests - called frequently inside calibration loop
void serviceCalibrationServer();

// Record one of the 4 samples:
//   step 0: Middle (Center)
//   step 1: Right
//   step 2: Left
//   step 3: Middle while moving forward (if enabled, else stationary)
void recordCalibrationSample(uint8_t stepIndex);

// Compute optimal min, max, and mid-point thresholds across the 4 samples
bool computeCalibrationThresholds();

// Reset all recorded samples and step counters
void resetCalibration();

