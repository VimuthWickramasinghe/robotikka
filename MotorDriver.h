#pragma once
#include <Arduino.h>
#include "Config.h"

// Initialize TB6612FNG direction pins + 20 kHz analogWrite() PWM on PWMA/PWMB.
// Call once in setup(); motors stay coasted until first speed command.
void initMotors();

// Set Motor A / Motor B speed directly in analogWrite duty units.
// Accepts -255..255; magnitude is constrained to 0..255 for the PWM
// and the sign sets direction (via AIN/BIN pins).
void setMotorASpeed(int pwm);
void setMotorBSpeed(int pwm);

// Convenience: set both to the same value
void setMotorsSpeed(int pwm);

// Coast (both IN pins LOW, PWM 0)
void stopMotors();

// Brake (both IN pins HIGH per driver datasheet)
void brakeMotors();
