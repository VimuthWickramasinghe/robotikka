#include "LineSensor.h"

// Default analog threshold for ADC channels (GPIO1, GPIO2: 12-bit, 0-4095)
// Photodiode over black surface: high resistance / higher voltage depending on circuit
// Adjust via calibration or potentiometer on module
static uint16_t analogThreshold = 2000;

// Track last known line position for memory during dotted lines or sharp curves
static float lastValidPosition = 0.0f;
static unsigned long lineLostTime = 0;

void initLineSensors() {
  // ADC channels
  analogReadResolution(12);
  pinMode(PIN_IR_1, INPUT);
  pinMode(PIN_IR_2, INPUT);

  // Digital channels
  pinMode(PIN_IR_3, INPUT);
  pinMode(PIN_IR_4, INPUT);
  pinMode(PIN_IR_5, INPUT);
  pinMode(PIN_IR_6, INPUT);
  pinMode(PIN_IR_7, INPUT);
  pinMode(PIN_IR_8, INPUT);
}

void calibrateLineSensors() {
  // Can be extended with a routine that sweeps over line
}

LineSensorState readLineSensors() {
  LineSensorState state;
  state.activeCount = 0;
  float weightedSum = 0.0f;
  float totalWeight = 0.0f;

  // Sensor array weights from left to right:
  // Index:   0     1     2     3     4     5     6     7
  // Weight: -3.5  -2.5  -1.5  -0.5  +0.5  +1.5  +2.5  +3.5
  const float weights[8] = {-3.5f, -2.5f, -1.5f, -0.5f, 0.5f, 1.5f, 2.5f, 3.5f};

  // 1. Read Analog sensors (IR_1, IR_2)
  state.raw[0] = analogRead(PIN_IR_1);
  state.raw[1] = analogRead(PIN_IR_2);

  // 2. Read Digital sensors (IR_3 to IR_8)
  for (int i = 2; i < 8; i++) {
    state.raw[i] = digitalRead(IR_PINS[i]);
  }

  // Determine black line status:
  // Active = HIGH on most IR sensor modules with comparator (or LOW depending on active-low setting)
  // Default: digital HIGH = black detected, ADC > threshold = black detected
  state.isBlack[0] = (state.raw[0] > analogThreshold);
  state.isBlack[1] = (state.raw[1] > analogThreshold);
  for (int i = 2; i < 8; i++) {
    state.isBlack[i] = (state.raw[i] == HIGH);
  }

  // Centroid Calculation:
  // Computes weighted average position
  for (int i = 0; i < 8; i++) {
    if (state.isBlack[i]) {
      state.activeCount++;
      weightedSum += weights[i];
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
    // No sensors currently see the line
    state.lineFound = false;
    if (lineLostTime == 0) {
      lineLostTime = millis();
    }

    // Per competition guidelines: "Dotted segments: Gaps and lengths between 2 to 5 cm"
    // If line is lost for a brief duration (< 350ms), hold last valid position (dotted line handling)
    if (millis() - lineLostTime < 350) {
      state.isDottedGap = true;
      state.position = lastValidPosition; // Maintain trajectory across gap
    } else {
      state.isDottedGap = false;
      // Hard off-track: maintain sign of last known direction to steer back
      state.position = (lastValidPosition > 0.0f) ? 3.5f : -3.5f;
    }
  }

  return state;
}

