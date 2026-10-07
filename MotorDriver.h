#pragma once
#include <Arduino.h>
#include "Config.h"

// Initialize Motor Driver PWM and direction pins
void initMotors();

// Drive motors: Left Speed (-255 to 255), Right Speed (-255 to 255)
// Positive = Forward, Negative = Reverse, 0 = Stop
void setMotorSpeeds(int leftSpeed, int rightSpeed);

// Active brake (short motor terminals to ground)
void brakeMotors();

// Coast stop
void stopMotors();

