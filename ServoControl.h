#pragma once
#include <Arduino.h>
#include "Config.h"

// Initialize the 3 servo PWM outputs (50 Hz RC servo timing)
void initServos();

// Set individual servo angle in degrees (0 to 180)
// servoNum: 1, 2, or 3 (corresponding to PIN_SERVO_1, PIN_SERVO_2, PIN_SERVO_3)
void setServoAngle(int servoNum, int angle);

// Process interactive serial commands for servo testing
void handleServoSerialCommands();

// --- Dedicated Arm & Gripper Servos (ESP32Servo) ---
bool initArmServos();
void detachArmServos();

// Smooth servo motion helper
void moveServoSmooth(int servoPin, int fromAngle, int toAngle, int stepDelayMs = SERVO_STEP_DELAY_MS);

// Individual control functions
void setArmAngle(int angle);
void setGripperAngle(int angle);
void gripObject();
void releaseObject();

// Combined arm action methods
void lowerArmAndGrip();
void collectObject();
void placeObject();

// --- Legacy swing test functions (maintained for test mode compatibility) ---
bool servoSwingInit();
void servoSwingWrite(int angle);
void servoSwingDetach();