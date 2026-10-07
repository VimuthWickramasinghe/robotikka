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

## 2. Hardware Architecture & Schematic Reference (`Schematic.pdf` Rev 2.0)

### Microcontroller:
- **Board**: `ESP32-S3-DEV-KIT-N8R8` (ESP32-S3 Xtensa dual-core LX7, 8MB Flash, 8MB PSRAM).

### Pin Mapping:
| Subsystem | Net Name | Schematic Ref | ESP32-S3 GPIO | Function / Notes |
| :--- | :--- | :--- | :--- | :--- |
| **TB6612FNG Motors (`U6`)** | `PWMA` | Pin 12 | **GPIO 8** | Left Motor PWM Speed |
| | `AIN1` | Pin 4 | **GPIO 4** | Left Motor Direction 1 |
| | `AIN2` | Pin 5 | **GPIO 5** | Left Motor Direction 2 |
| | `PWMB` | Pin 15 | **GPIO 9** | Right Motor PWM Speed |
| | `BIN1` | Pin 6 | **GPIO 6** | Right Motor Direction 1 |
| | `BIN2` | Pin 7 | **GPIO 7** | Right Motor Direction 2 |
| | `ST` (STBY) | Pin 13 | *Tied to +3.3V* | Standby disabled in hardware |
| **Arm Servos (`U7, U8, U9`)** | `PWM1` | Pin 8 | **GPIO 15** | Servo 1 (Shoulder / Arm) |
| | `PWM2` | Pin 9 | **GPIO 16** | Servo 2 (Elbow / Tilt) |
| | `PWM3` | Pin 10 | **GPIO 17** | Servo 3 (Gripper) |
| **Shared I2C Bus** | `SDA` | Pin 17 | **GPIO 11** | Shared by Display, Color Sensor, TOF Panel |
| | `SCL` | Pin 18 | **GPIO 12** | Shared I2C Clock |
| **TOF Distance Panel (`H5`)** | `XSHUT1` | Pin 19 | **GPIO 13** | VL53L0X / VL53L1X Sensor 1 Enable |
| | `XSHUT2` | Pin 20 | **GPIO 14** | Sensor 2 Enable |
| | `XSHUT3` | Pin 16 | **GPIO 10** | Sensor 3 Enable |
| | `XSHUT4` | Pin 29 | **GPIO 48** | Sensor 4 Enable |
| **Color Sensor (`H6`)** | `INT` | Pin 11 | **GPIO 18** | Color detection interrupt |
| | `LED` | Pin 27 | **GPIO 21** | Color sensor illumination / Status LED |
| **8-IR Sensor Array (`J4`)** | `IR_1` | Pin 41 | **GPIO 1** | Analog channel (`ADC1_CH0`) |
| | `IR_2` | Pin 40 | **GPIO 2** | Analog channel (`ADC1_CH1`) |
| | `IR_3` | Pin 35 | **GPIO 38** | Digital input |
| | `IR_4` | Pin 36 | **GPIO 39** (MTCK) | Digital input |
| | `IR_5` | Pin 37 | **GPIO 40** (MTDO) | Digital input |
| | `IR_6` | Pin 38 | **GPIO 41** (MTDI) | Digital input |
| | `IR_7` | Pin 39 | **GPIO 42** (MTMS) | Digital input |
| | `IR_8` | Pin 28 | **GPIO 47** | Digital input |
| **User Interface** | `BOOT` | Pin 31 | **GPIO 0** | Triple-click button for OTA |
| | `RGB_BUILTIN` | Internal | *Board dependent* | Onboard WS2812 RGB NeoPixel |

### Power Rails:
- **Battery Input (`J3`)**: 2-pin connector through `SW1` (Motor power) to `+12V` rail and `H2` (REG12).
- **`+5V` Rail**: Powered by `H4` (`REG5V`) through `SW2` (Power switch) -> powers Servos (`U7`, `U8`, `U9`).
- **`+3.3V` Rail**: Powered by `H3` (`REG3V3`) -> powers ESP32-S3, Display, TOF sensors, Color sensor, IR panel.
- **Bulk & Bypass Capacitors**: C1 (470uF), C8 (100uF), C10 (47uF), C2-C5 & C9 (100nF).

---

## 3. Firmware Architecture (FreeRTOS Dual-Core)

### Core 0: Wireless Telemetry & UI (`TaskTelemetryOTA`, Priority 1)
- **Wi-Fi / ArduinoOTA**:
  - Wi-Fi powers up **OFF** (`WiFi.mode(WIFI_OFF)`) for competition compliance.
  - Activated strictly on-demand by **pressing the BOOT button (GPIO 0) 3 times within 1.5 seconds**.
  - Listens on port `3232` with hostname `robotikka-s3`.
- **Status LED Animations (`setLedOutput`, `updateLedAnimation`)**:
  - Offline Idle: Breathing vibrant Purple/Magenta (2s cycle).
  - Wi-Fi Connecting: Fast Amber/Yellow toggle (100ms).
  - OTA Ready: Steady Green pulse (500ms ON / 500ms OFF).
  - Active Flashing: Hyper-fast Magenta/Red strobe (40ms).
- **Serial Test Command Parser (`handleServoSerialCommands`)**:
  - Real-time command parsing for live servo adjustment (`s1 90`, `sweep 1`, `status`, etc.).

### Core 1: Real-Time Robot Control (`TaskRobotControl`, Priority 3)
- **Deterministic Loop Rate**: Strict **100 Hz (10 ms)** loop via `vTaskDelayUntil()`.
- **Subsystems Handled**:
  - Sensor acquisition (8-IR array).
  - High-speed differential PID steering.
  - TB6612 motor PWM generation at **20 kHz** (silent, no acoustic whine).
  - TOF sensor reading and wall following distance regulation.

---

## 4. Codebase Modular File Index

| File | Purpose |
| :--- | :--- |
| [`robotikka.ino`](file:///c:/projects/Robotikka/robotikka/robotikka.ino) | Main application entry point, FreeRTOS task creation, Core 0 telemetry/OTA loop, Core 1 control supervisor. |
| [`Config.h`](file:///c:/projects/Robotikka/robotikka/Config.h) | Centralized GPIO pin constants, IR array index mapping, and hardware settings. |
| [`MotorDriver.h`](file:///c:/projects/Robotikka/robotikka/MotorDriver.h) / [`.cpp`](file:///c:/projects/Robotikka/robotikka/MotorDriver.cpp) | TB6612FNG H-bridge driver (`ledcAttach` 20 kHz, forward, reverse, active brake, coast stop). |
| [`LineSensor.h`](file:///c:/projects/Robotikka/robotikka/LineSensor.h) / [`.cpp`](file:///c:/projects/Robotikka/robotikka/LineSensor.cpp) | 8-sensor centroid calculator ($-3.5$ to $+3.5$), dotted line gap detector and trajectory hold. |
| [`LinePID.h`](file:///c:/projects/Robotikka/robotikka/LinePID.h) / [`.cpp`](file:///c:/projects/Robotikka/robotikka/LinePID.cpp) | Differential steering PID algorithm with anti-windup clamp and sharp-turn pivot limits. |
| [`ServoControl.h`](file:///c:/projects/Robotikka/robotikka/ServoControl.h) / [`.cpp`](file:///c:/projects/Robotikka/robotikka/ServoControl.cpp) | 16-bit 50 Hz RC servo controller and interactive Serial testing interface. |

---

## 5. Development & Troubleshooting Notes

1. **OTA Port Bug in Arduino IDE 2.x**:
   - Arduino IDE 2.x may report `invalid int value: '{upload.port.properties.port}'`.
   - Permanent fix: In `C:\Users\tpawi\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.8\platform.txt`, change `"{upload.port.properties.port}"` to `"3232"` in `tools.espota.upload.pattern`.
2. **OTA Host IP Multi-Adapter Conflict**:
   - If PC has WSL/Hyper-V virtual network adapters (`vEthernet`), `espota.exe` may broadcast the virtual adapter IP causing `[ERROR]: No response from the ESP`.
   - Explicitly run `espota.py` with `-I <PC_WIFI_IP>` or disable virtual switches during pit calibration if needed.
3. **ESP32-S3 USB CDC Serial**:
   - In Arduino IDE **Tools** menu, ensure **USB CDC On Boot: Enabled** so `Serial.begin(115200)` prints directly over the native USB-C port.

