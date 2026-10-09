#include "LineSensor.h"

// Default analog threshold for ADC channels (GPIO1, GPIO2: 12-bit, 0-4095)
// Photodiode over black surface: high resistance / higher voltage depending on circuit
// Adjust via calibration or potentiometer on module
static uint16_t sensorMin[8] = {4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095};
static uint16_t sensorMax[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static uint16_t sensorThreshold[8] = {2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000};

// Track last known line position for memory during dotted lines or sharp curves
static float lastValidPosition = 0.0f;
static unsigned long lineLostTime = 0;

void initLineSensors() {
  analogReadResolution(12);
  for (int i = 0; i < 8; i++) {
    pinMode(IR_PINS[i], INPUT);
  }
}

void calibrateLineSensors() {
  Serial.println("\n[CALIB] Starting 5-second IR Array Calibration sweep...");
  Serial.println("[CALIB] Move sensor array back and forth across black line and white surface!");

  for (int i = 0; i < 8; i++) {
    sensorMin[i] = 4095;
    sensorMax[i] = 0;
  }

  unsigned long startCalib = millis();
  while (millis() - startCalib < 5000) {
    for (int i = 0; i < 8; i++) {
      uint16_t val = analogRead(IR_PINS[i]);
      if (val < sensorMin[i]) sensorMin[i] = val;
      if (val > sensorMax[i]) sensorMax[i] = val;
    }
    delay(10);
  }

  // Calculate midpoints
  Serial.println("[CALIB] Calibration Complete! Computed Thresholds:");
  for (int i = 0; i < 8; i++) {
    sensorThreshold[i] = (sensorMin[i] + sensorMax[i]) / 2;
    Serial.printf(" Ch%d: Min=%4u Max=%4u -> Thresh=%4u\n", i, sensorMin[i], sensorMax[i], sensorThreshold[i]);
  }
  Serial.println();
}


LineSensorState readLineSensors() {
  LineSensorState state;
  state.activeCount = 0;
  float weightedSum = 0.0f;
  float totalWeight = 0.0f;

  // Center 5 PID sensors: A6, A5, A4, A3, A2 (Left to Right across line)
  // Index:   0 (A6)    1 (A5)    2 (A4)    3 (A3)    4 (A2)
  // Weight:  -2.0f     -1.0f      0.0f     +1.0f     +2.0f
  const float pidWeights[5] = {-2.0f, -1.0f, 0.0f, 1.0f, 2.0f};

  // 1. Read all 8 channels as Analog ADC values (0 - 4095)
  // Order: [0]=A6, [1]=A5, [2]=A4, [3]=A3, [4]=A2, [5]=A1, [6]=A7, [7]=A8
  for (int i = 0; i < 8; i++) {
    state.raw[i] = analogRead(IR_PINS[i]);
    state.isBlack[i] = (state.raw[i] > sensorThreshold[i]);
  }


  // 2. Auxiliary navigation sensors
  state.rightTurn  = state.isBlack[5]; // Index 5: A1 (RIGHT Turn sensor)
  state.leftTurn   = state.isBlack[6]; // Index 6: A7 (LEFT Turn sensor)
  state.backSensor = state.isBlack[7]; // Index 7: A8 (BACK reference sensor)

  // 3. Centroid calculation exclusively on center 5 PID sensors (indices 0 to 4)
  for (int i = 0; i < 5; i++) {
    if (state.isBlack[i]) {
      state.activeCount++;
      weightedSum += pidWeights[i];
      totalWeight += 1.0f;
    }
  }

  if (state.activeCount > 0) {
    state.lineFound = true;
    state.isDottedGap = false;
    state.position = weightedSum / totalWeight;
    lastValidPosition = state.position;
    lineLostTime = 0;
  } else {
    // No center PID sensors see the line
    state.lineFound = false;
    if (lineLostTime == 0) {
      lineLostTime = millis();
    }

    // Dotted line handling (maintain trajectory for brief gaps < 350ms)
    if (millis() - lineLostTime < 350) {
      state.isDottedGap = true;
      state.position = lastValidPosition;
    } else {
      state.isDottedGap = false;
      // Hard off-track: maintain sign of last known direction to steer back
      state.position = (lastValidPosition > 0.0f) ? 2.0f : -2.0f;
    }
  }

  return state;
}

static bool irDebugStream = false;

void setIRDebugStream(bool enable) {
  irDebugStream = enable;
}

bool isIRDebugStreamEnabled() {
  return irDebugStream;
}

void printLineSensorValues() {
  // Rate-limit serial output to ~10 Hz (every 100 ms) so it doesn't flood when looping fast
  static unsigned long lastPrintMs = 0;
  if (millis() - lastPrintMs < 100) return;
  lastPrintMs = millis();

  LineSensorState s = readLineSensors();

  // Single-line print with functional roles clearly labeled:
  // Center 5 PID: A6, A5, A4, A3, A2 | Auxiliary: R(A1), L(A7), B(A8)
  Serial.printf("PID:[A6:%4u A5:%4u A4:%4u A3:%4u A2:%4u]  R(A1):%4u  L(A7):%4u  B(A8):%4u\n",
                s.raw[0], s.raw[1], s.raw[2], s.raw[3], s.raw[4],
                s.raw[5], s.raw[6], s.raw[7]);
}


