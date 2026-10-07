#pragma once
#include <Arduino.h>

struct PIDConfig {
  float Kp;            // Proportional gain
  float Ki;            // Integral gain
  float Kd;            // Derivative gain
  int baseSpeed;       // Nominal forward speed (0 - 255)
  int maxSpeed;        // Maximum motor speed limit (e.g. 255)
  int minSpeed;        // Minimum motor speed (e.g. -150 for reverse pivot)
  float maxIntegral;   // Anti-windup limit for integral term
};

// Initialize PID controller parameters
void initLinePID(float kp = 45.0f, float ki = 0.0f, float kd = 25.0f, int baseSpeed = 160);

// Set PID gains dynamically
void setPIDGains(float kp, float ki, float kd);

// Set base motor forward speed
void setBaseSpeed(int speed);

// Reset PID integral and derivative history
void resetPID();

// Update PID calculation given current line position error (-3.5 to +3.5)
// and apply motor speeds
void updateLineFollower(float lineError, bool isDottedGap);

