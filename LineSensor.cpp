#include "LineSensor.h"
#include "MotorDriver.h"

// ==============================================================================
// GLOBAL PID VARIABLES
// ==============================================================================

// Global PID configuration (tuned defaults)
PIDConfig g_pidConfig = {
  45.0f,   // Kp
  0.0f,    // Ki
  25.0f,   // Kd
  160,     // baseSpeed (0 - 255)
  240,     // maxSpeed (0 - 255)
  -100,    // minSpeed (-255 to 255, allows reverse pivot on sharp curves)
  50.0f,   // maxIntegral (anti-windup clamp)
  2000     // threshold (12-bit ADC threshold: 0 - 4095)
};

PIDConfig &pidConfig = g_pidConfig;

// Global runtime tracking variables
float g_lineLastError = 0.0f;
float g_lineIntegral = 0.0f;
LineSensorState g_lastLineState = {};

// Internal static tracking variables
static float s_lastValidPosition = 0.0f;
static unsigned long s_lineLostTime = 0;
static unsigned long s_lastMicros = 0;

// ==============================================================================
// FUNCTIONS
// ==============================================================================

void resetLinePID() {
  g_lineLastError = 0.0f;
  g_lineIntegral = 0.0f;
  s_lastValidPosition = 0.0f;
  s_lineLostTime = 0;
  s_lastMicros = 0;
}

void initLineSensors() {
  // ADC1 is used for GPIO1-8 (IR sensors) on ESP32-S3
  analogReadResolution(12);
  for (int i = 0; i < 8; i++) {
    pinMode(IR_PINS[i], INPUT);
  }
  resetLinePID();
}

void calibrateLineSensors() {
  // Can be extended with multi-sample white/black floor calibration
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

  for (int i = 0; i < 8; i++) {
    state.raw[i] = analogRead(IR_PINS[i]);
    state.isBlack[i] = (state.raw[i] > g_pidConfig.threshold);
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
    s_lastValidPosition = state.position;
    s_lineLostTime = 0;
  } else {
    // No sensor currently detects the line
    state.lineFound = false;
    if (s_lineLostTime == 0) {
      s_lineLostTime = millis();
    }

    // Trajectory hold across dotted gaps (< 350 ms)
    if (millis() - s_lineLostTime < 350) {
      state.isDottedGap = true;
      state.position = s_lastValidPosition;
    } else {
      state.isDottedGap = false;
      // Hard off-track: maintain sign of last seen position to steer back towards line
      state.position = (s_lastValidPosition >= 0.0f) ? 3.5f : -3.5f;
    }
  }

  g_lastLineState = state;
  return state;
}

void follow_line() {
  LineSensorState state = readLineSensors();

  unsigned long nowMicros = micros();
  float dt = (s_lastMicros == 0) ? 0.002f : (float)(nowMicros - s_lastMicros) / 1000000.0f;
  if (dt <= 0.0f || dt > 0.05f) {
    dt = 0.002f; // Fallback to nominal 2 ms (500 Hz) if jitter or first call
  }
  s_lastMicros = nowMicros;

  float lineError = state.position;

  // Proportional term
  float P = g_pidConfig.Kp * lineError;

  // Integral term with anti-windup clamp
  g_lineIntegral += (lineError * dt);
  g_lineIntegral = constrain(g_lineIntegral, -g_pidConfig.maxIntegral, g_pidConfig.maxIntegral);
  float I = g_pidConfig.Ki * g_lineIntegral;

  // Derivative term (rate of change of error)
  float D = g_pidConfig.Kd * ((lineError - g_lineLastError) / dt);
  g_lineLastError = lineError;

  // Total steering correction
  float correction = P + I + D;

  // Differential drive motor speeds:
  // Motor A = Left Motor, Motor B = Right Motor
  // If lineError > 0 (line is to the right): Left speeds up, Right slows down -> turns Right
  int leftMotorSpeed  = g_pidConfig.baseSpeed + (int)correction;
  int rightMotorSpeed = g_pidConfig.baseSpeed - (int)correction;

  // If currently traversing a dotted gap, maintain straight forward momentum
  if (state.isDottedGap) {
    leftMotorSpeed  = g_pidConfig.baseSpeed;
    rightMotorSpeed = g_pidConfig.baseSpeed;
  }

  // Constrain outputs to configured limits
  leftMotorSpeed  = constrain(leftMotorSpeed, g_pidConfig.minSpeed, g_pidConfig.maxSpeed);
  rightMotorSpeed = constrain(rightMotorSpeed, g_pidConfig.minSpeed, g_pidConfig.maxSpeed);

  setMotorASpeed(leftMotorSpeed);
  setMotorBSpeed(rightMotorSpeed);
}

void printLineSensorValues() {
  LineSensorState state = readLineSensors();
  for (int i = 0; i < 8; i++) {
    Serial.print(state.raw[i]);
    if (i < 7) {
      Serial.print(" ");
    }
  }
  Serial.printf(" | Pos: %+.2f | Count: %d | Found: %s\n",
                state.position, state.activeCount, state.lineFound ? "YES" : "NO");
}
