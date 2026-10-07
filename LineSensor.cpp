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

static bool irDebugStream = false;

void setIRDebugStream(bool enable) {
  irDebugStream = enable;
}

bool isIRDebugStreamEnabled() {
  return irDebugStream;
}

void printLineSensorValues() {
  LineSensorState s = readLineSensors();

  // Print nicely formatted table / values
  Serial.println("======================================================================");
  Serial.println(" [IR SENSOR READINGS] (Schematic Rev 2.0 - 8-Sensor Array J4)");
  Serial.println("----------------------------------------------------------------------");
  Serial.println(" Sensor :  IR_1    IR_2    IR_3    IR_4    IR_5    IR_6    IR_7    IR_8");
  Serial.println(" GPIO   :  (G1)    (G2)    (G38)   (G39)   (G40)   (G41)   (G42)   (G47)");
  Serial.println(" Type   : [ADC1]  [ADC1]   [DIG]   [DIG]   [DIG]   [DIG]   [DIG]   [DIG]");
  Serial.println("----------------------------------------------------------------------");

  // Raw readings (IR_1 & IR_2 are 12-bit ADC 0-4095; IR_3 to IR_8 are digital 0 or 1)
  Serial.printf(" Raw Val:  %4u    %4u       %d       %d       %d       %d       %d       %d\n",
                s.raw[0], s.raw[1], s.raw[2], s.raw[3], s.raw[4], s.raw[5], s.raw[6], s.raw[7]);

  // Detected Black/White status
  Serial.printf(" Status :   %s      %s      %s      %s      %s      %s      %s      %s\n",
                s.isBlack[0] ? "BLK" : "---",
                s.isBlack[1] ? "BLK" : "---",
                s.isBlack[2] ? "BLK" : "---",
                s.isBlack[3] ? "BLK" : "---",
                s.isBlack[4] ? "BLK" : "---",
                s.isBlack[5] ? "BLK" : "---",
                s.isBlack[6] ? "BLK" : "---",
                s.isBlack[7] ? "BLK" : "---");

  Serial.println("----------------------------------------------------------------------");
  Serial.printf(" Active Sensors: %d/8 | Line Found: %s | Dotted Gap: %s\n",
                s.activeCount, 
                s.lineFound ? "YES" : "NO",
                s.isDottedGap ? "YES" : "NO");
  Serial.printf(" Computed Position: %+.2f  (Left: -3.5 <--- Center: 0.00 ---> Right: +3.5)\n", s.position);
  Serial.println("======================================================================\n");
}


