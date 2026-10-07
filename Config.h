#pragma once
#include <Arduino.h>

// ==============================================================================
// PIN DEFINITIONS (Extracted from Schematic.pdf - EasyEDA Rev 2.0)
// ==============================================================================

// --- TB6612FNG Dual DC Motor Driver (U6) ---
constexpr int PIN_MOTOR_PWMA  = 8;   // GPIO8:  Motor A PWM Speed Control
constexpr int PIN_MOTOR_AIN1  = 4;   // GPIO4:  Motor A Direction 1
constexpr int PIN_MOTOR_AIN2  = 5;   // GPIO5:  Motor A Direction 2
constexpr int PIN_MOTOR_PWMB  = 9;   // GPIO9:  Motor B PWM Speed Control
constexpr int PIN_MOTOR_BIN1  = 6;   // GPIO6:  Motor B Direction 1
constexpr int PIN_MOTOR_BIN2  = 7;   // GPIO7:  Motor B Direction 2
// Note: STBY (Standby) is hardwired to +3.3V in hardware (always enabled)

// --- Arm & Gripper Servos (U7, U8, U9) ---
constexpr int PIN_SERVO_1     = 15;  // GPIO15 (PWM1): Servo U7
constexpr int PIN_SERVO_2     = 16;  // GPIO16 (PWM2): Servo U8
constexpr int PIN_SERVO_3     = 17;  // GPIO17 (PWM3): Servo U9

// --- Shared I2C Bus (Display H1, Color Sensor H6, TOF Panel H5) ---
constexpr int PIN_I2C_SDA     = 11;  // GPIO11: I2C Serial Data
constexpr int PIN_I2C_SCL     = 12;  // GPIO12: I2C Serial Clock

// --- TOF Sensors Panel (H5) - Shutdown / Enable Pins ---
constexpr int PIN_TOF_XSHUT1  = 13;  // GPIO13: TOF Sensor 1 XSHUT
constexpr int PIN_TOF_XSHUT2  = 14;  // GPIO14: TOF Sensor 2 XSHUT
constexpr int PIN_TOF_XSHUT3  = 10;  // GPIO10: TOF Sensor 3 XSHUT
constexpr int PIN_TOF_XSHUT4  = 48;  // GPIO48: TOF Sensor 4 XSHUT

// --- Color Sensor (H6) ---
constexpr int PIN_COLOR_INT   = 18;  // GPIO18: Color Sensor Interrupt
constexpr int PIN_COLOR_LED   = 21;  // GPIO21: Net "LED" (Color Sensor illumination / Status LED)

// --- 8-Sensor IR Line Following Array (J4) ---
constexpr int PIN_IR_1        = 1;   // GPIO1  (ADC1_CH0)
constexpr int PIN_IR_2        = 2;   // GPIO2  (ADC1_CH1)
constexpr int PIN_IR_3        = 38;  // GPIO38 (Digital)
constexpr int PIN_IR_4        = 39;  // GPIO39 / MTCK (Digital)
constexpr int PIN_IR_5        = 40;  // GPIO40 / MTDO (Digital)
constexpr int PIN_IR_6        = 41;  // GPIO41 / MTDI (Digital)
constexpr int PIN_IR_7        = 42;  // GPIO42 / MTMS (Digital)
constexpr int PIN_IR_8        = 47;  // GPIO47 (Digital)

inline const int IR_PINS[8] = {
  PIN_IR_1, PIN_IR_2, PIN_IR_3, PIN_IR_4,
  PIN_IR_5, PIN_IR_6, PIN_IR_7, PIN_IR_8
};

// --- User Button & Status Indicator ---
constexpr int BUTTON_PIN      = 0;   // GPIO0: Onboard BOOT button (Active LOW)
constexpr int STATUS_LED_PIN  = 21;  // GPIO21: Net "LED" (Onboard indicator / Color Sensor LED)

