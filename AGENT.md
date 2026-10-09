# AGENT.md - Robotikka Project Context & Reference

## 1. Project Overview & Competition Context
- **Project Name**: Robotikka (Autonomous Line & Wall Following Robot)
- **Competition**: UOK Robot Games 2K26 - Robot Race (`Robot Race- Full Guidelines.pdf`)
- **Organizers**: Electronics and Computer Science Club (ECSC), University of Kelaniya, Sri Lanka.
- **Competition Tasks & Flow**:
  1. **Task 1: Line Following**: Follow black line ($2\text{ to }3\text{ cm}$ wide) from start square; handle dotted segments ($2\text{ to }5\text{ cm}$ gaps).
  2. **Task 2: Pick Up Box & Color Identification**: Detect and grasp colored box ($5 \times 5 \times 5\text{ cm}$) in curved track zone. Identify color (Red, Green, Blue) and indicate via LEDs / Display.
  3. **Task 3: Wall Following**: Navigate curved track section using Time-of-Flight (TOF) distance sensors while maintaining safe distance and avoiding wall collision.
  4. **Task 4: Precision Placement**: Deliver and place box accurately into matching color drop zone ($10 \times 10\text{ cm}$).
- **Critical Rules**:
  - Size limit: $20\text{ cm} \times 20\text{ cm}$ at start; no height limit; extensions allowed only during operation.
  - Power: Internal battery only, max $24\text{ V}$.
  - **Autonomy & Wireless Rule (Section 4.d & 8.a.vi)**: All wireless communication (Wi-Fi, Bluetooth, radio) must be terminated before placing on the arena. External wireless interference during a run results in immediate disqualification.
  - Activation: Onboard physical switch/buttons only.

---

## 2. Hardware Architecture & Pin Mapping

### Microcontroller:
- **Board**: `ESP32-S3-DEV-KIT-N8R8` (ESP32-S3 Xtensa dual-core LX7, 8MB Flash, 8MB PSRAM).

### Pin Mapping:
| Subsystem | Net / Role | Current ESP32-S3 GPIO | Function / Notes |
| :--- | :--- | :--- | :--- |
| **TB6612FNG Motors (`U6`)** | `PWMA` | **GPIO 8** | Left Motor PWM Speed Control |
| | `AIN1` | **GPIO 39** | Left Motor Direction 1 *(Reassigned from GPIO 4 to free ADC1)* |
| | `AIN2` | **GPIO 40** | Left Motor Direction 2 *(Reassigned from GPIO 5 to free ADC1)* |
| | `PWMB` | **GPIO 9** | Right Motor PWM Speed Control |
| | `BIN1` | **GPIO 41** | Right Motor Direction 1 *(Reassigned from GPIO 6 to free ADC1)* |
| | `BIN2` | **GPIO 42** | Right Motor Direction 2 *(Reassigned from GPIO 7 to free ADC1)* |
| | `ST` (STBY) | *Tied to +3.3V* | Standby disabled in hardware |
| **Arm Servos (`U7, U8, U9`)** | `PWM1` | **GPIO 15** | Servo 1 (Shoulder / Arm) |
| | `PWM2` | **GPIO 16** | Servo 2 (Elbow / Tilt) |
| | `PWM3` | **GPIO 17** | Servo 3 (Gripper) |
| **Shared I2C Bus** | `SDA` | **GPIO 11** | Shared by Display, Color Sensor, TOF Panel |
| | `SCL` | **GPIO 12** | Shared I2C Clock |
| **TOF Distance Panel (`H5`)** | `XSHUT1` | **GPIO 13** | VL53L0X / VL53L1X Sensor 1 Enable |
| | `XSHUT2` | **GPIO 14** | Sensor 2 Enable |
| | `XSHUT3` | **GPIO 47** | Sensor 3 Enable *(Reassigned from GPIO 10 to free ADC1)* |
| | `XSHUT4` | **GPIO 48** | Sensor 4 Enable |
| **Color Sensor (`H6`)** | `INT` | **GPIO 18** | Color detection interrupt |
| | `LED` | **GPIO 21** | Color sensor illumination / Status indicator LED |
| **8-IR Sensor Array (`J4`)** | `A6` [PID 1/5] | **GPIO 5** | Left-most PID sensor (`ADC1_CH4`, Weight: -2.0) |
| *(Reordered Array)* | `A5` [PID 2/5] | **GPIO 6** | Mid-left PID sensor (`ADC1_CH5`, Weight: -1.0) |
| | `A4` [PID 3/5] | **GPIO 7** | Center PID sensor (`ADC1_CH6`, Weight: 0.0) |
| | `A3` [PID 4/5] | **GPIO 3** | Mid-right PID sensor (`ADC1_CH2`, Weight: +1.0) |
| | `A2` [PID 5/5] | **GPIO 2** | Right-most PID sensor (`ADC1_CH1`, Weight: +2.0) |
| | `A1` [RIGHT] | **GPIO 1** | Right 90° Turn sensor (`ADC1_CH0`, auxiliary) |
| | `A7` [LEFT] | **GPIO 4** | Left 90° Turn sensor (`ADC1_CH3`, auxiliary) |
| | `A8` [BACK] | **GPIO 10** | Rear reference / Intersection sensor (`ADC1_CH9`, auxiliary) |
| **User Interface** | `BOOT` | **GPIO 0** | Mode selector & confirmation button (active LOW) |
| | `RGB_BUILTIN` | **GPIO 48 / 38** | Onboard WS2812 RGB NeoPixel (`RGB_BUILTIN` macro) |
| | `STATUS_LED` | **GPIO 21** | Board indicator LED (synchronized with mode changes) |

### Power Rails:
- **Battery Input (`J3`)**: 2-pin connector through `SW1` (Motor power) to `+12V` rail and `H2` (REG12).
- **`+5V` Rail**: Powered by `H4` (`REG5V`) through `SW2` (Power switch) -> powers Servos (`U7`, `U8`, `U9`).
- **`+3.3V` Rail**: Powered by `H3` (`REG3V3`) -> powers ESP32-S3, Display, TOF sensors, Color sensor, IR panel.
- **Bulk & Bypass Capacitors**: C1 (470uF), C8 (100uF), C10 (47uF), C2-C5 & C9 (100nF).

---

## 3. Firmware Architecture (FreeRTOS Dual-Core)

### Core 0: Button Mode Selector & Telemetry (`buttonLogicTask`, `TaskTelemetryOTA`)
- **8-Mode Command Selector (`buttonLogicTask`, Priority 2, 50 Hz)**:
  - Continuously polls BOOT button with a 40 ms debounce window.
  - State machine: `BTN_IDLE` -> `BTN_COLOR_SELECT` -> `BTN_CONFIRM_BLINK`.
  - Cycles 8 command modes every 600 ms via onboard WS2812 and GPIO 21 status LED.
  - 3-second confirmation window before locking in selection and resuming suspended task.
- **Wireless Telemetry & OTA (`TaskTelemetryOTA`, Priority 1)**:
  - Wi-Fi starts **OFF** (`WiFi.mode(WIFI_OFF)`) for ADC2 stability and competition compliance.
  - Activated when **OTA Mode (Green)** is confirmed via button selector.
  - Listens on port `3232` with mDNS hostname `robotikka-s3`.
  - Serial test command parser for live servo and IR debugging (`help`, `ir`, `s1 <angle>`, `sweep`, etc.).

### Core 1: Real-Time Robot Control (`TaskRobotControl`, Priority 8)
- **Deterministic Loop Rate**: Strict **500 Hz (2 ms)** loop via `vTaskDelayUntil()`.
- **Subsystems Handled**:
  - **8-IR Array Acquisition**: Reads all 8 sensors via ESP32-S3 `ADC1` channels with 12-bit resolution.
  - **PID Steering Centroid**: Computed **strictly over center 5 sensors** (`A6, A5, A4, A3, A2`, weights -2.0 to +2.0).
  - **Turn & Auxiliary Detection**: Exposes `rightTurn` (`A1`), `leftTurn` (`A7`), and `backSensor` (`A8`) flags.
  - **Dotted Gap Bridging**: Holds trajectory for gaps up to 350 ms.
  - **TB6612 Motor PWM**: 20 kHz silent drive.

---

## 4. Codebase Modular File Index

| File | Purpose |
| :--- | :--- |
| [`robotikka.ino`](file:///c:/projects/Robotikka/robotikka/robotikka.ino) | Main application entry point, FreeRTOS dual-core task lifecycle setup, supervisor loop. |
| [`Config.h`](file:///c:/projects/Robotikka/robotikka/Config.h) | Centralized GPIO constants, resolved motor/TOF pin mappings, reordered IR sensor array. |
| [`button_task.cpp`](file:///c:/projects/Robotikka/robotikka/button_task.cpp) | 8-color command selector state machine, debounced BOOT button reader, dual LED driver. |
| [`globals.h`](file:///c:/projects/Robotikka/robotikka/globals.h) / [`.cpp`](file:///c:/projects/Robotikka/robotikka/globals.cpp) | Shared RTOS handles, `RunMode` enums, cross-task state variables. |
| [`LineSensor.h`](file:///c:/projects/Robotikka/robotikka/LineSensor.h) / [`.cpp`](file:///c:/projects/Robotikka/robotikka/LineSensor.cpp) | 8-channel ADC reader, 5-sensor PID centroid (-2.0 to +2.0), turn flags, single-line serial output. |
| [`LinePID.h`](file:///c:/projects/Robotikka/robotikka/LinePID.h) / [`.cpp`](file:///c:/projects/Robotikka/robotikka/LinePID.cpp) | Differential steering PID algorithm with anti-windup clamp and turn pivot limits. |
| [`MotorDriver.h`](file:///c:/projects/Robotikka/robotikka/MotorDriver.h) / [`.cpp`](file:///c:/projects/Robotikka/robotikka/MotorDriver.cpp) | TB6612FNG H-bridge driver (`ledcAttach` 20 kHz, forward, reverse, brake, coast). |
| [`ServoControl.h`](file:///c:/projects/Robotikka/robotikka/ServoControl.h) / [`.cpp`](file:///c:/projects/Robotikka/robotikka/ServoControl.cpp) | 16-bit 50 Hz RC servo controller and interactive Serial testing interface. |

---

## 5. Development & Troubleshooting Notes

1. **IR Sensor Analog ADC1 Alignment**:
   - Pins `{1, 2, 3, 7, 6, 5, 4, 10}` all map to ESP32-S3 **ADC1**.
   - Because all IR sensors are on ADC1, Wi-Fi can remain isolated or operate in OTA mode without disrupting analog line acquisition.
2. **Motor Direction & TOF Pin Relocation**:
   - Motor direction pins were moved to GPIO 39, 40, 41, 42 and TOF XSHUT3 to GPIO 47 to avoid electrical conflicts with the ADC1 IR sensors.
3. **ESP32-S3 USB CDC Serial**:
   - Ensure **USB CDC On Boot: Enabled** in Arduino IDE Tools menu to communicate over native USB-C at 115200 baud.
