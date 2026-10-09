#pragma once
#include <Arduino.h>

// ==============================================================================
// PIN DEFINITIONS (Extracted from Schematic.pdf - EasyEDA Rev 2.0)
// ==============================================================================

// --- TB6612FNG Dual DC Motor Driver (U6) ---
// Reassigned to pins released from previous IR array (39, 40, 41, 42) to avoid conflict with IR {1,2,3,7,6,5,4,10}
constexpr int PIN_MOTOR_PWMA  = 8;   // GPIO8:  Motor A PWM Speed Control
constexpr int PIN_MOTOR_AIN1  = 39;  // GPIO39: Motor A Direction 1 (Reassigned from GPIO4)
constexpr int PIN_MOTOR_AIN2  = 40;  // GPIO40: Motor A Direction 2 (Reassigned from GPIO5)
constexpr int PIN_MOTOR_PWMB  = 9;   // GPIO9:  Motor B PWM Speed Control
constexpr int PIN_MOTOR_BIN1  = 41;  // GPIO41: Motor B Direction 1 (Reassigned from GPIO6)
constexpr int PIN_MOTOR_BIN2  = 42;  // GPIO42: Motor B Direction 2 (Reassigned from GPIO7)
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
constexpr int PIN_TOF_XSHUT3  = 47;  // GPIO47: TOF Sensor 3 XSHUT (Reassigned from GPIO10)
constexpr int PIN_TOF_XSHUT4  = 48;  // GPIO48: TOF Sensor 4 XSHUT

// --- Color Sensor (H6) ---
constexpr int PIN_COLOR_INT   = 18;  // GPIO18: Color Sensor Interrupt
constexpr int PIN_COLOR_LED   = 21;  // GPIO21: Net "LED" (Color Sensor illumination / Status LED)

// --- 8-Sensor IR Line Following Array ---
// Channels: A1=GPIO1, A2=GPIO2, A3=GPIO3, A4=GPIO7, A5=GPIO6, A6=GPIO5, A7=GPIO4, A8=GPIO10
//
// REORDERED SENSOR ARRAY LAYOUT:
// --------------------------------------------------------------------------------------------------
// Center 5 PID Sensors (Left -> Right): A6 (GPIO 5), A5 (GPIO 6), A4 (GPIO 7), A3 (GPIO 3), A2 (GPIO 2)
// Right-most Turn Sensor              : A1 (GPIO 1)
// Left-most Turn Sensor               : A7 (GPIO 4)
// Back / Rear Reference Sensor        : A8 (GPIO 10)
// --------------------------------------------------------------------------------------------------
constexpr int PIN_IR_A6 = 5;   // GPIO5:  [PID 1/5] Left-most PID sensor
constexpr int PIN_IR_A5 = 6;   // GPIO6:  [PID 2/5] Mid-left PID sensor
constexpr int PIN_IR_A4 = 7;   // GPIO7:  [PID 3/5] Center PID sensor
constexpr int PIN_IR_A3 = 3;   // GPIO3:  [PID 4/5] Mid-right PID sensor
constexpr int PIN_IR_A2 = 2;   // GPIO2:  [PID 5/5] Right-most PID sensor
constexpr int PIN_IR_A1 = 1;   // GPIO1:  [TURN]    Right-most 90° Turn sensor (RIGHT)
constexpr int PIN_IR_A7 = 4;   // GPIO4:  [TURN]    Left-most 90° Turn sensor (LEFT)
constexpr int PIN_IR_A8 = 10;  // GPIO10: [BACK]    Rear reference / Intersection sensor (BACK)

// Backward compatibility channel aliases:
constexpr int PIN_IR_1 = PIN_IR_A1;
constexpr int PIN_IR_2 = PIN_IR_A2;
constexpr int PIN_IR_3 = PIN_IR_A3;
constexpr int PIN_IR_4 = PIN_IR_A4;
constexpr int PIN_IR_5 = PIN_IR_A5;
constexpr int PIN_IR_6 = PIN_IR_A6;
constexpr int PIN_IR_7 = PIN_IR_A7;
constexpr int PIN_IR_8 = PIN_IR_A8;

// Ordered array: indices 0..4 = PID Left to Right, 5 = RIGHT, 6 = LEFT, 7 = BACK
inline const int IR_PINS[8] = {
  PIN_IR_A6,  // Index 0: A6 - Left-most PID
  PIN_IR_A5,  // Index 1: A5 - Mid-left PID
  PIN_IR_A4,  // Index 2: A4 - Center PID
  PIN_IR_A3,  // Index 3: A3 - Mid-right PID
  PIN_IR_A2,  // Index 4: A2 - Right-most PID
  PIN_IR_A1,  // Index 5: A1 - RIGHT Turn Sensor
  PIN_IR_A7,  // Index 6: A7 - LEFT Turn Sensor
  PIN_IR_A8   // Index 7: A8 - BACK Sensor
};

// --- User Button & Status Indicator ---
constexpr int BUTTON_PIN      = 0;   // GPIO0: Onboard BOOT button (Active LOW)
#if defined(RGB_BUILTIN)
constexpr int RGB_LED_PIN     = RGB_BUILTIN; // Board-defined NeoPixel pin
#else
constexpr int RGB_LED_PIN     = 48;  // GPIO48: Default ESP32-S3 DevKit WS2812 (or 38 on some clones)
#endif
constexpr int STATUS_LED_PIN  = 21;  // GPIO21: Net "LED" (Onboard indicator / Color Sensor LED)

// ==============================================================================
// COMPETITION MISSION & TASK CONFIGURATION
// ==============================================================================

// --- Task 1: Start Box Exit ---
constexpr int SPEED_START_EXIT          = 130;  // Straight motor speed when exiting start box
constexpr unsigned long TIME_START_EXIT = 600;  // Milliseconds to drive forward out of colored start box

// --- Task 1: Arm & Gripper Servos ---
// Large Servo: Arm Elevation (PIN_SERVO_1 / GPIO 15)
constexpr int SERVO_ARM_UP              = 30;   // Arm raised position (safe transit / carrying)
constexpr int SERVO_ARM_DOWN            = 125;  // Arm lowered position (for picking up / placing box)
// Small Servo: Gripper (PIN_SERVO_2 / GPIO 16)
constexpr int SERVO_GRIP_OPEN           = 40;   // Gripper jaws opened
constexpr int SERVO_GRIP_CLOSE          = 135;  // Gripper clamped on 5x5x5 cm box

// --- Task 1: Obstacle & Box Detection (Top Front Distance Sensor) ---
// PIN_TOF_XSHUT1 / TOF 1 is pointing directly forward above color sensor
constexpr int BOX_DETECT_DISTANCE_MM    = 70;   // Distance in mm to trigger box presence
constexpr int BOX_REVERSE_SPEED         = 110;  // Reversing speed before lowering arm
constexpr unsigned long BOX_REVERSE_MS  = 300;  // Reversing duration in ms

// --- Task 2: Curved Wall Following (Right Wall/Ball Following) ---
// Right side distance sensor is TOF 2 (or configured side sensor)
constexpr float WALL_TARGET_DISTANCE_MM = 120.0f; // Target distance = 12 cm (120 mm)
constexpr float WALL_KP                 = 1.5f;   // Wall following proportional steering gain
constexpr int WALL_BASE_SPEED           = 140;    // Forward speed along curved wall

// --- Task 3: Junction Branch Routing by Detected Color ---
// Configure branch directions at 3-way junction:
// Default: Red -> LEFT, Blue -> FORWARD, Green -> RIGHT
enum JunctionAction : uint8_t {
    BRANCH_LEFT,
    BRANCH_STRAIGHT,
    BRANCH_RIGHT
};

constexpr JunctionAction ACTION_RED_BOX   = BRANCH_LEFT;     // Red Box -> Turn Left
constexpr JunctionAction ACTION_BLUE_BOX  = BRANCH_STRAIGHT; // Blue Box -> Go Straight
constexpr JunctionAction ACTION_GREEN_BOX = BRANCH_RIGHT;    // Green Box -> Turn Right


