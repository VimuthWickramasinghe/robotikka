#include "LinePID.h"
#include "MotorDriver.h"

static PIDConfig pidConfig;
static float lastError = 0.0f;
static float integralTerm = 0.0f;

void initLinePID(float kp, float ki, float kd, int baseSpeed) {
  pidConfig.Kp = kp;
  pidConfig.Ki = ki;
  pidConfig.Kd = kd;
  pidConfig.baseSpeed = baseSpeed;
  pidConfig.maxSpeed = 240;
  pidConfig.minSpeed = -100; // Allows negative values for quick pivot on sharp turns
  pidConfig.maxIntegral = 50.0f;

  resetPID();
}

void setPIDGains(float kp, float ki, float kd) {
  pidConfig.Kp = kp;
  pidConfig.Ki = ki;
  pidConfig.Kd = kd;
}

void setBaseSpeed(int speed) {
  pidConfig.baseSpeed = constrain(speed, 0, 255);
}

void resetPID() {
  lastError = 0.0f;
  integralTerm = 0.0f;
}

void updateLineFollower(float lineError, bool isDottedGap) {
  // Proportional term
  float P = pidConfig.Kp * lineError;

  // Integral term (with anti-windup clamp)
  integralTerm += (lineError * 0.01f); // dt = 10ms (100Hz loop)
  integralTerm = constrain(integralTerm, -pidConfig.maxIntegral, pidConfig.maxIntegral);
  float I = pidConfig.Ki * integralTerm;

  // Derivative term (rate of change of error)
  float D = pidConfig.Kd * ((lineError - lastError) / 0.01f);
  lastError = lineError;

  // Total steering correction
  float correction = P + I + D;

  // Differential drive motor speeds:
  // If lineError > 0 (robot is shifted left, line is to the right):
  // Left motor speeds up, Right motor slows down -> turns Right.
  int leftMotorSpeed  = pidConfig.baseSpeed + (int)correction;
  int rightMotorSpeed = pidConfig.baseSpeed - (int)correction;

  // If currently traversing a dotted gap, maintain straight forward momentum
  if (isDottedGap) {
    leftMotorSpeed  = pidConfig.baseSpeed;
    rightMotorSpeed = pidConfig.baseSpeed;
  }

  // Constrain outputs
  leftMotorSpeed  = constrain(leftMotorSpeed, pidConfig.minSpeed, pidConfig.maxSpeed);
  rightMotorSpeed = constrain(rightMotorSpeed, pidConfig.minSpeed, pidConfig.maxSpeed);

  setMotorSpeeds(leftMotorSpeed, rightMotorSpeed);
}

