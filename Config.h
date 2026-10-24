#pragma once
#include <Arduino.h>

// ==============================================================================
// PIN DEFINITIONS (Extracted from Schematic.pdf - EasyEDA Rev 2.0)
// ==============================================================================

// --- TB6612FNG Dual DC Motor Driver (U6) ---
constexpr int PIN_MOTOR_PWMA  = 39;  // GPIO38: Motor A PWM Speed Control
constexpr int PIN_MOTOR_AIN1  = 35;  // GPIO35: Motor A Direction 1
constexpr int PIN_MOTOR_AIN2  = 36;  // GPIO36: Motor A Direction 2
constexpr int PIN_MOTOR_PWMB  = 9;   // GPIO9:  Motor B PWM Speed Control
constexpr int PIN_MOTOR_BIN1  = 37;  // GPIO36: Motor B Direction 1
constexpr int PIN_MOTOR_BIN2  = 38;  // GPIO37: Motor B Direction 2
// Note: STBY (Standby) is hardwired to +3.3V in hardware (always enabled)

// --- Arm & Gripper Servos (U7, U8, U9) ---
constexpr int PIN_SERVO_ARM     = 15;  // GPIO15 (PWM1): Arm joint servo
constexpr int PIN_SERVO_GRIPPER = 16;  // GPIO16 (PWM2): Gripper servo
constexpr int PIN_SERVO_1       = 15;  // GPIO15 (PWM1): Servo U7 (Arm joint)
constexpr int PIN_SERVO_2       = 16;  // GPIO16 (PWM2): Servo U8 (Gripper)
constexpr int PIN_SERVO_3       = 17;  // GPIO17 (PWM3): Servo U9

// --- Arm & Gripper Angles and Timings ---
constexpr int ARM_LOWER_ANGLE   = 30;   // Arm lowered to pick / place position (test range 30-90)
constexpr int ARM_RAISE_ANGLE   = 90;   // Arm raised / resting position
constexpr int GRIPPER_GRIP      = 180;  // Gripper closed / grip state
constexpr int GRIPPER_RELEASE   = 0;    // Gripper open / release state
constexpr int SERVO_STEP_DELAY_MS = 10; // Delay per step (ms) for smooth servo motion

// --- Shared I2C Bus (Display H1, Color Sensor H6, TOF Panel H5) ---
constexpr int PIN_I2C_SDA     = 11;  // GPIO11: I2C Serial Data
constexpr int PIN_I2C_SCL     = 12;  // GPIO12: I2C Serial Clock
constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;

// --- TOF Sensors Panel (H5) - Shutdown / Enable Pins ---
constexpr int PIN_TOF_XSHUT1  = 13;  // GPIO13: TOF Sensor 1 XSHUT
constexpr int PIN_TOF_XSHUT2  = -1;  // GPIO14 reassigned: now dedicated to COMPETITION_BUTTON_PIN
constexpr int PIN_TOF_XSHUT3  = 10;  // GPIO10: TOF Sensor 3 XSHUT
constexpr int PIN_TOF_XSHUT4  = 47;  // GPIO47: TOF Sensor 4 XSHUT

// --- Color Sensor (H6) ---
constexpr int PIN_COLOR_INT   = 18;  // GPIO18: Color Sensor Interrupt
constexpr int PIN_COLOR_LED   = 21;  // GPIO21: Net "LED" (Color Sensor illumination / Status LED)

// --- 8-Sensor IR Line Following Array (J4) ---
// The PID uses only IR_2 through IR_6 (GPIO2 through GPIO6).
constexpr int PIN_IR_1        = 1;   // GPIO1  (ADC1_CH0)
constexpr int PIN_IR_2        = 2;   // GPIO2  (ADC1_CH1)
constexpr int PIN_IR_3        = 3;   // GPIO3  (ADC1_CH2)
constexpr int PIN_IR_4        = 4;   // GPIO4  (ADC1_CH3)
constexpr int PIN_IR_5        = 5;   // GPIO5  (ADC1_CH4)
constexpr int PIN_IR_6        = 6;   // GPIO6  (ADC1_CH5)
constexpr int PIN_IR_7        = 7;   // GPIO7
constexpr int PIN_IR_8        = 8;   // GPIO8

constexpr int LINE_SENSOR_COUNT = 5;
inline const int IR_PINS[LINE_SENSOR_COUNT] = {
  PIN_IR_2, PIN_IR_3, PIN_IR_4, PIN_IR_5, PIN_IR_6
};

// --- User Button & Status Indicator ---
constexpr int BUTTON_PIN      = 0;   // GPIO0: Onboard BOOT button (Active LOW)
constexpr int RGB_LED_PIN     = 48;  // GPIO48: Onboard WS2812 RGB NeoPixel
constexpr int STATUS_LED_PIN  = 21;  // GPIO21: Net "LED" (Onboard indicator / Color Sensor LED)

// ==============================================================================
// WAYA 2.0 COMPETITION ARENA CONFIGURATIONS
// ==============================================================================

// 1. Black Start Pad Exit
constexpr int START_BLIND_PULSE_MS      = 350;  // ms to drive off the black start pad
constexpr int START_BLIND_SPEED         = 90;   // PWM speed during start exit

// 2. Post-Ramp Curved Location Detection
constexpr float CURVE_ERROR_THRESHOLD   = 0.75f; // Centroid error magnitude indicating sharp curve
constexpr int CURVE_CONSECUTIVE_SAMPLES  = 12;   // Number of high-error samples to confirm curve entry
constexpr int CURVE_BASE_SPEED          = 80;    // Reduced base speed for tight curved path cornering

// 3. Creeping Color Search on Floor
constexpr int COLOR_SEARCH_STEP_MS      = 25;   // Forward pulse step duration (ms)
constexpr int COLOR_SEARCH_SPEED        = 55;   // Slow crawl speed while polling color
constexpr int COLOR_CONFIRM_SAMPLES     = 2;    // Consecutive readings to lock in Red or Blue

// 4. Turnaround & Branch Maneuvers
constexpr int TURN_180_MS               = 550;  // Duration to spin 180 degrees
constexpr int TURN_90_MS                = 280;  // Duration to turn 90 degrees
constexpr int TURN_SPEED                = 110;  // Pivot turn PWM speed
constexpr int POST_COLOR_FORWARD_MS     = 300;  // Forward drive before turning towards branch

// --- Competition Start & Emergency Stop Button (GPIO 14) ---
constexpr int COMPETITION_BUTTON_PIN    = 14;   // Pin 14: Start competition & Instant Emergency Kill Switch
