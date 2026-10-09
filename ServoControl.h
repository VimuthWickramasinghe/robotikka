#pragma once
#include <Arduino.h>
#include "Config.h"

// Initialize the 3 servo PWM outputs (50 Hz RC servo timing)
void initServos();

// Set individual servo angle in degrees (0 to 180)
// servoNum: 1, 2, or 3 (corresponding to PIN_SERVO_1, PIN_SERVO_2, PIN_SERVO_3)
void setServoAngle(int servoNum, int angle);

// Process interactive serial commands for servo testing
// Command formats:
//   "s1 90"    -> Move Servo 1 to 90 degrees
//   "s2 45"    -> Move Servo 2 to 45 degrees
//   "s3 120"   -> Move Servo 3 to 120 degrees
//   "all 90"   -> Move all servos to 90 degrees
//   "sweep 1"  -> Run a sweep test (0 -> 180 -> 0) on Servo 1
void handleServoSerialCommands();
void handleServoSerialCommandsWithLine(const String &line);
