# AGENT.md - Robotikka Project Context & Reference

## 1. Project Overview & Competition Context
- **Project Name**: Robotikka (Autonomous Line & Wall Following Robot)
- **Competition**: UOK Robot Games 2K26 - Robot Race (`Robot Race- Full Guidelines.pdf`)
- **Organizers**: Electronics and Computer Science Club (ECSC), University of Kelaniya, Sri Lanka.
- **Competition Tasks & Arena Flow**:
  1. **Task 1: Line Following**: Follow black line ($2\text{ to }3\text{ cm}$ wide) from start square on white background; handle sharp turns and dotted segments ($2\text{ to }5\text{ cm}$ gaps).
  2. **Task 2: Pick Up Box & Color Identification**: Detect and grasp colored wooden/acrylic cube ($5 \times 5 \times 5\text{ cm}$) in curved track zone. Identify color (Red, Green, Blue) and display/indicate color state.
  3. **Task 3: Wall Following**: Navigate curved track section using Time-of-Flight (TOF) distance sensors while maintaining safe distance and avoiding wall collision.
  4. **Task 4: Precision Placement**: Deliver and place box accurately into matching color drop zone ($10 \times 10\text{ cm}$).
- **Critical Competition Rules**:
  - **Size Limit**: Starting footprint $\le 20\text{ cm} \times 20\text{ cm}$; no height limit; physical expansions allowed only after leaving the start box.
  - **Power Source**: Internal onboard battery only, maximum voltage $\le 24\text{ V}$.
  - **Autonomy & Wireless Rule (Section 4.d & 8.a.vi)**: All wireless communication (Wi-Fi, Bluetooth, 2.4 GHz RF) must be strictly terminated before placing on the arena. External wireless interference during a run results in immediate disqualification.
  - **Activation**: Onboard physical button/switch activation only. No remote activation permitted.

---

## 2. Hardware Architecture & Pinout Reference (Schematic Rev 2.0 / [`Config.h`](file:///c:/projects/Robotikka/robotikka/Config.h))

### Microcontroller:
- **Board**: `ESP32-S3-DEV-KIT-N8R8` (ESP32-S3 Xtensa dual-core 32-bit LX7, up to 240 MHz, 8MB Quad SPI Flash, 8MB Octal PSRAM).

### Pin Mapping:

| Subsystem | Net / Function | Schematic Pin | ESP32-S3 GPIO | Software Constant | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **TB6612FNG Motors (`U6`)** | `PWMA` | Pin 12 | **GPIO 39** | [`PIN_MOTOR_PWMA`](file:///c:/projects/Robotikka/robotikka/Config.h#L9) | Left Motor PWM (20 kHz silent) |
| | `AIN1` | Pin 4 | **GPIO 35** | [`PIN_MOTOR_AIN1`](file:///c:/projects/Robotikka/robotikka/Config.h#L10) | Left Motor Direction 1 |
| | `AIN2` | Pin 5 | **GPIO 36** | [`PIN_MOTOR_AIN2`](file:///c:/projects/Robotikka/robotikka/Config.h#L11) | Left Motor Direction 2 |
| | `PWMB` | Pin 15 | **GPIO 9** | [`PIN_MOTOR_PWMB`](file:///c:/projects/Robotikka/robotikka/Config.h#L12) | Right Motor PWM (20 kHz silent) |
| | `BIN1` | Pin 6 | **GPIO 37** | [`PIN_MOTOR_BIN1`](file:///c:/projects/Robotikka/robotikka/Config.h#L13) | Right Motor Direction 1 |
| | `BIN2` | Pin 7 | **GPIO 38** | [`PIN_MOTOR_BIN2`](file:///c:/projects/Robotikka/robotikka/Config.h#L14) | Right Motor Direction 2 |
| | `STBY` | Pin 13 | *Tied to +3.3V* | — | Hardwired enabled in hardware |
| **Arm Servos (`U7, U8, U9`)** | `PWM1` | Pin 8 | **GPIO 15** | [`PIN_SERVO_1`](file:///c:/projects/Robotikka/robotikka/Config.h#L18) | Servo 1: Shoulder / Arm |
| | `PWM2` | Pin 9 | **GPIO 16** | [`PIN_SERVO_2`](file:///c:/projects/Robotikka/robotikka/Config.h#L19) | Servo 2: Elbow / Tilt |
| | `PWM3` | Pin 10 | **GPIO 17** | [`PIN_SERVO_3`](file:///c:/projects/Robotikka/robotikka/Config.h#L20) | Servo 3: Gripper |
| **Shared I2C Bus** | `SDA` | Pin 17 | **GPIO 11** | [`PIN_I2C_SDA`](file:///c:/projects/Robotikka/robotikka/Config.h#L23) | Shared by Display, TCS34725, VL53L0X panel |
| | `SCL` | Pin 18 | **GPIO 12** | [`PIN_I2C_SCL`](file:///c:/projects/Robotikka/robotikka/Config.h#L24) | Shared I2C Clock line |
| **TOF Sensors Panel (`H5`)** | `XSHUT1` | Pin 19 | **GPIO 13** | [`PIN_TOF_XSHUT1`](file:///c:/projects/Robotikka/robotikka/Config.h#L27) | Sensor 1 Enable (`TOF_FRONT_CENTER` $\to$ 0x30) |
| | `XSHUT2` | Pin 20 | **GPIO 14** | [`PIN_TOF_XSHUT2`](file:///c:/projects/Robotikka/robotikka/Config.h#L28) | Sensor 2 Enable (`TOF_RIGHT60` $\to$ 0x31) |
| | `XSHUT3` | Pin 16 | **GPIO 10** | [`PIN_TOF_XSHUT3`](file:///c:/projects/Robotikka/robotikka/Config.h#L29) | Sensor 3 Enable (`TOF_FRONT_LEFT` $\to$ 0x32) |
| | `XSHUT4` | Pin 29 | **GPIO 48** | [`PIN_TOF_XSHUT4`](file:///c:/projects/Robotikka/robotikka/Config.h#L30) | Sensor 4 Enable (`TOF_LEFT60` $\to$ 0x33, shared with RGB) |
| | *(None)* | — | *(Unwired)* | — | Sensor 5 (`TOF_FRONT_RIGHT` $\to$ 0x34, readdressed 1st) |
| **Color Sensor (`H6`)** | `INT` | Pin 11 | **GPIO 18** | [`PIN_COLOR_INT`](file:///c:/projects/Robotikka/robotikka/Config.h#L33) | TCS34725 Interrupt pin |
| | `LED` | Pin 27 | **GPIO 21** | [`PIN_COLOR_LED`](file:///c:/projects/Robotikka/robotikka/Config.h#L34) | Color sensor illumination / Status LED |
| **8-IR Sensor Array (`J4`)** | `IR_1` | Pin 41 | **GPIO 1** | [`PIN_IR_1`](file:///c:/projects/Robotikka/robotikka/Config.h#L38) | ADC1_CH0 |
| | `IR_2` | Pin 40 | **GPIO 2** | [`PIN_IR_2`](file:///c:/projects/Robotikka/robotikka/Config.h#L39) | ADC1_CH1 |
| | `IR_3` | Pin 35 | **GPIO 3** | [`PIN_IR_3`](file:///c:/projects/Robotikka/robotikka/Config.h#L40) | ADC1_CH2 |
| | `IR_4` | Pin 36 | **GPIO 4** | [`PIN_IR_4`](file:///c:/projects/Robotikka/robotikka/Config.h#L41) | ADC1_CH3 |
| | `IR_5` | Pin 37 | **GPIO 5** | [`PIN_IR_5`](file:///c:/projects/Robotikka/robotikka/Config.h#L42) | ADC1_CH4 |
| | `IR_6` | Pin 38 | **GPIO 6** | [`PIN_IR_6`](file:///c:/projects/Robotikka/robotikka/Config.h#L43) | ADC1_CH5 |
| | `IR_7` | Pin 39 | **GPIO 7** | [`PIN_IR_7`](file:///c:/projects/Robotikka/robotikka/Config.h#L44) | ADC1_CH6 |
| | `IR_8` | Pin 28 | **GPIO 8** | [`PIN_IR_8`](file:///c:/projects/Robotikka/robotikka/Config.h#L45) | ADC1_CH7 |
| **User Interface** | `BOOT` | Pin 31 | **GPIO 0** | [`BUTTON_PIN`](file:///c:/projects/Robotikka/robotikka/Config.h#L53) | BOOT button, Active LOW, falling-edge ISR |
| | `RGB_LED` | Pin 29 | **GPIO 48** | [`RGB_LED_PIN`](file:///c:/projects/Robotikka/robotikka/Config.h#L54) | Onboard WS2812 NeoPixel (shares net with XSHUT4) |
| | `STATUS_LED` | Pin 27 | **GPIO 21** | [`STATUS_LED_PIN`](file:///c:/projects/Robotikka/robotikka/Config.h#L55) | External status LED / illumination control |

### Important Hardware Multiplexing / Sharing Notes:
1. **GPIO 48 Sharing (`RGB_LED_PIN` & `PIN_TOF_XSHUT4`)**:
   - GPIO 48 connects to both the WS2812 NeoPixel data input and TOF Sensor 4 XSHUT.
   - During boot [`initTOFSensors()`](file:///c:/projects/Robotikka/robotikka/TOFSensors.cpp#L24), GPIO 48 is held low as an OUTPUT pin to reset the sensor and then driven high to bring it up at address `0x33`. Once TOF address assignment completes, GPIO 48 is safely driven by `neopixelWrite()`.
2. **GPIO 21 Sharing (`STATUS_LED_PIN` & `PIN_COLOR_LED`)**:
   - GPIO 21 drives the white illumination LED on the TCS34725 board as well as serving as a general onboard indicator.

### Power Distribution:
- **Battery Rail (`J3`)**: 2-pin connector through `SW1` (Motor power) to `+12V` rail and `H2` (`REG12`).
- **`+5V` Rail**: Powered by `H4` (`REG5V`) through `SW2` (Logic power switch) $\to$ Powers servos (`U7`, `U8`, `U9`).
- **`+3.3V` Rail**: Powered by `H3` (`REG3V3`) $\to$ Powers ESP32-S3, OLED Display, TOF sensors, TCS34725 Color sensor, IR sensor array.
- **Decoupling & Bulk Capacitors**: C1 ($470\,\mu\text{F}$), C8 ($100\,\mu\text{F}$), C10 ($47\,\mu\text{F}$), C2–C5 & C9 ($100\,\text{nF}$).

---

## 3. Firmware Architecture (FreeRTOS Dual-Core v0.2)

### Task Execution Model:
The system initializes all tasks in a deterministic **suspended state** at boot. Tasks are awakened on demand through the button selector or interrupt service routines.

```
       +-------------------------------------------------------------+
       |                         ESP32-S3                            |
       |                                                             |
       |  Core 0                                    Core 1           |
       |  +-------------------------+               +-------------+  |
       |  | buttonLogicTask (Prio 4)|               | TaskRobot-  |  |
       |  | - BOOT button ISR wake  |               | Control     |  |
       |  | - 8-color state machine |               | (Prio 8)    |  |
       |  +------------+------------+               | - 500 Hz    |  |
       |               | sets mode / resumes        |   control   |  |
       |               v                            |   loop      |  |
       |  +-------------------------+               | - Line IR   |  |
       |  | TaskTelemetryOTA(Prio 1)|               | - Color Det |  |
       |  | - Wi-Fi / ArduinoOTA    |               | - TOF Wall  |  |
       |  | - Serial Command Parser |               | - TB6612    |  |
       |  +-------------------------+               +-------------+  |
       +-------------------------------------------------------------+
```

### 1. Core 0: User Interface & Telemetry
- **[`buttonLogicTask`](file:///c:/projects/Robotikka/robotikka/button_task.cpp#L160) (Priority 4, Stack 4096)**:
  - Created suspended; resumed by falling-edge interrupt [`onBootButtonISR()`](file:///c:/projects/Robotikka/robotikka/button_task.cpp#L82) on GPIO 0.
  - Manages the 8-color command selection state machine on the WS2812 NeoPixel.
  - Consumes zero CPU cycles while idle.
- **[`TaskTelemetryOTA`](file:///c:/projects/Robotikka/robotikka/robotikka.ino#L102) (Priority 1, Stack 4096)**:
  - Wi-Fi defaults to **OFF** (`WiFi.mode(WIFI_OFF)`) at boot for competition legality.
  - When resumed in `OTA_MODE`, connects to configured Wi-Fi (`Dialog 4G`), advertises hostname `robotikka-s3` on port `3232`, and handles incoming OTA updates via `ArduinoOTA.handle()`.
  - Continuously executes [`handleServoSerialCommands()`](file:///c:/projects/Robotikka/robotikka/ServoControl.cpp#L123) for real-time serial calibration (`115200` baud).

### 2. Core 1: Real-Time High-Speed Control
- **[`TaskRobotControl`](file:///c:/projects/Robotikka/robotikka/robotikka.ino#L130) (Priority 8, Stack 4096)**:
  - Deterministic **500 Hz (2 ms)** periodic loop paced by `vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2))`.
  - Dispatches execution based on the shared [`g_runMode`](file:///c:/projects/Robotikka/robotikka/globals.h#L50):
    - `LINE_FOLLOW`: High-speed IR sensor reading and line following.
    - `COLOR_DETECT`: Real-time TCS34725 acquisition and LED mirroring.
    - `MOTOR_TEST`: Triangular PWM ramp sweep test.
    - `SERVO_TEST`: 30° to 90° arm servo swing cycle.
    - `TOF_STREAM`: 5-sensor continuous distance stream.
  - All test modes implement safe exit via [`consumeButtonPress()`](file:///c:/projects/Robotikka/robotikka/button_task.cpp#L67), stopping motors, parking servos at 90°, extinguishing lights, resetting mode to `MODE_IDLE`, and self-suspending.

---

## 4. Button UI & 8-Color Command Selector

The single onboard `BOOT` button (GPIO 0) and WS2812 RGB LED (GPIO 48) provide headless pit testing and runtime mode selection without requiring a connected computer.

### State Flow:
1. **Wake**: Pressing `BOOT` fires [`onBootButtonISR()`](file:///c:/projects/Robotikka/robotikka/button_task.cpp#L82), which calls `xTaskResumeFromISR()`.
2. **Cycle (`BTN_COLOR_SELECT`)**: The RGB LED steps through 8 distinct colors (500 ms dwell per color).
3. **Select**: Pressing `BOOT` during a color transitions to `BTN_CONFIRM_BLINK`.
4. **Confirm**: The chosen color blinks for 3 seconds (15 ticks $\times$ 200 ms).
   - Pressing `BOOT` during blink confirms the command $\to$ sets [`g_runMode`](file:///c:/projects/Robotikka/robotikka/globals.h#L50), resumes appropriate task, and suspends the button task.
   - If 3 seconds elapse without press, confirmation aborts and cycling resumes.
5. **Exit Test Mode**: Pressing `BOOT` while inside a test mode exits immediately back to idle via [`consumeButtonPress()`](file:///c:/projects/Robotikka/robotikka/button_task.cpp#L67) with a 300 ms debounce anchor filter.

### 8-Color Command Mapping:

| # | Color | RGB Value | Command Name | [`RunMode`](file:///c:/projects/Robotikka/robotikka/globals.h#L19) Enum | Target Action / Task |
| :-: | :--- | :--- | :--- | :--- | :--- |
| **0** | **Red** | `(255, 0, 0)` | `linefollow` | `LINE_FOLLOW` | Starts Core 1 line following control loop |
| **1** | **Green** | `(0, 255, 0)` | `OTA mode` | `OTA_MODE` | Activates Wi-Fi STA, joins network, enables ArduinoOTA (port 3232) |
| **2** | **Blue** | `(0, 0, 255)` | `IR calibrate` | `IR_CALIBRATE` | Calibrates line sensor white/black floor thresholds |
| **3** | **Yellow** | `(255, 255, 0)` | `command4` | `COLOR_DETECT` | Live TCS34725 reading; onboard RGB mirrors dominant box color |
| **4** | **Magenta** | `(255, 0, 255)` | `command5` | `MOTOR_TEST` | Triangular wave PWM sweep ($0 \to 255 \to 0$, both motors forward) |
| **5** | **Cyan** | `(0, 255, 255)` | `command6` | `SERVO_TEST` | Arm swing test: Servos 1 & 2 oscillate between 30° and 90° |
| **6** | **Purple** | `(128, 0, 255)` | `command7` | `COMMAND6` | Reserved for future competition behavior |
| **7** | **Orange** | `(255, 128, 0)` | `command8` | `TOF_STREAM` | Continuous distance streaming across all 5 TOF sensors (60 ms poll) |

---

## 5. Subsystem Details & Drivers

### 1. Motor Driver ([`MotorDriver.h`](file:///c:/projects/Robotikka/robotikka/MotorDriver.h) / [`.cpp`](file:///c:/projects/Robotikka/robotikka/MotorDriver.cpp))
- **H-Bridge**: Toshiba TB6612FNG dual motor driver.
- **PWM Frequency**: **20 kHz** ultrasonic frequency via `analogWriteFrequency()` (completely silent, prevents audible motor whine).
- **Functions**:
  - [`setMotorASpeed(int pwm)`](file:///c:/projects/Robotikka/robotikka/MotorDriver.cpp#L26) / [`setMotorBSpeed(int pwm)`](file:///c:/projects/Robotikka/robotikka/MotorDriver.cpp#L38): Supports values from $-255$ to $+255$ (sign determines direction, magnitude sets duty).
  - [`setMotorsSpeed(int pwm)`](file:///c:/projects/Robotikka/robotikka/MotorDriver.cpp#L50): Drive both motors concurrently.
  - [`stopMotors()`](file:///c:/projects/Robotikka/robotikka/MotorDriver.cpp#L55): Coast stop (all input lines LOW, PWM 0).
  - [`brakeMotors()`](file:///c:/projects/Robotikka/robotikka/MotorDriver.cpp#L64): Active brake (all input lines HIGH, PWM 0).

### 2. Time-of-Flight Panel ([`TOFSensors.h`](file:///c:/projects/Robotikka/robotikka/TOFSensors.h) / [`.cpp`](file:///c:/projects/Robotikka/robotikka/TOFSensors.cpp))
- **Sensors**: 5 $\times$ ST VL53L0X Time-of-Flight distance sensors on shared I2C bus (`SDA=11`, `SCL=12`).
- **Dynamic Address Allocation Sequence**:
  - Factory default I2C address is `0x29` for all VL53L0X chips.
  - `TOF_FRONT_RIGHT` has no XSHUT pin connected.
  - [`initTOFSensors()`](file:///c:/projects/Robotikka/robotikka/TOFSensors.cpp#L24) drives `XSHUT1`, `XSHUT2`, `XSHUT3`, and `XSHUT4` LOW, keeping those 4 devices in hardware reset.
  - Only `TOF_FRONT_RIGHT` remains powered at `0x29` $\to$ re-addressed **first** to `0x34`.
  - Next, each XSHUT pin is brought HIGH one at a time and assigned its unique address:
    - `TOF_FRONT_CENTER` (XSHUT1 / GPIO 13) $\to$ **`0x30`**
    - `TOF_RIGHT60` (XSHUT2 / GPIO 14) $\to$ **`0x31`**
    - `TOF_FRONT_LEFT` (XSHUT3 / GPIO 10) $\to$ **`0x32`**
    - `TOF_LEFT60` (XSHUT4 / GPIO 48) $\to$ **`0x33`**
- **Operating Mode**: `VL53L0X_SENSE_HIGH_SPEED` continuous ranging with a 50 ms cycle time. Polled via [`readTOFContinuous(uint16_t mm[5])`](file:///c:/projects/Robotikka/robotikka/TOFSensors.cpp#L70).

### 3. Color Sensor ([`ColorSensor.h`](file:///c:/projects/Robotikka/robotikka/ColorSensor.h) / [`.cpp`](file:///c:/projects/Robotikka/robotikka/ColorSensor.cpp))
- **Sensor**: Adafruit TCS34725 RGB + Clear sensor with IR blocking filter.
- **Timing & Gain**: 600 ms integration time, 4x gain (`COLOR_SAMPLE_PERIOD_MS = 620`).
- **Illumination Control**: Controlled via GPIO 21 ([`setColorLight(bool on)`](file:///c:/projects/Robotikka/robotikka/ColorSensor.cpp#L49)), held steady during detection sessions to eliminate ambient light fluctuations.
- **Classification & Display**: [`applyColorToLed()`](file:///c:/projects/Robotikka/robotikka/ColorSensor.cpp#L58) normalizes channel readings against the Clear channel and applies majority voting to drive the onboard NeoPixel to pure Red, Green, or Blue.

### 4. Arm Servos ([`ServoControl.h`](file:///c:/projects/Robotikka/robotikka/ServoControl.h) / [`.cpp`](file:///c:/projects/Robotikka/robotikka/ServoControl.cpp))
- **Servos**: 3 $\times$ micro servos (Shoulder GPIO 15, Elbow GPIO 16, Gripper GPIO 17).
- **Control Interface**:
  - Direct 16-bit 50 Hz PWM via [`ledcAttach()`](file:///c:/projects/Robotikka/robotikka/ServoControl.cpp#L24) ($500\,\mu\text{s}$ at 0° to $2500\,\mu\text{s}$ at 180°).
  - ESP32Servo swing mode ([`servoSwingInit()`](file:///c:/projects/Robotikka/robotikka/ServoControl.cpp#L85), [`servoSwingWrite()`](file:///c:/projects/Robotikka/robotikka/ServoControl.cpp#L106), [`servoSwingDetach()`](file:///c:/projects/Robotikka/robotikka/ServoControl.cpp#L113)) for automated test sweeps.
  - Interactive Serial CLI parser ([`handleServoSerialCommands()`](file:///c:/projects/Robotikka/robotikka/ServoControl.cpp#L123)) accepting `s1 <deg>`, `s2 <deg>`, `s3 <deg>`, `all <deg>`, `sweep <1|2|3>`, `status`, `ir`, `help`.

### 5. Line Sensor Array & PID Steering ([`LineSensor.h`](file:///c:/projects/Robotikka/robotikka/LineSensor.h) / [`.cpp`](file:///c:/projects/Robotikka/robotikka/LineSensor.cpp))
- **Sensors**: 8 IR reflective sensor channels mapped to **GPIO 1 through GPIO 8** (`PIN_IR_1` to `PIN_IR_8`) on ADC1.
- **Global PID Variables**: [`g_pidConfig`](file:///c:/projects/Robotikka/robotikka/LineSensor.h#L18) (`Kp=45.0`, `Ki=0.0`, `Kd=25.0`, `baseSpeed=160`, `maxSpeed=240`, `minSpeed=-100`, `maxIntegral=50.0`, `threshold=2000`), [`g_lineLastError`](file:///c:/projects/Robotikka/robotikka/LineSensor.h#L20), [`g_lineIntegral`](file:///c:/projects/Robotikka/robotikka/LineSensor.h#L21), [`g_lastLineState`](file:///c:/projects/Robotikka/robotikka/LineSensor.h#L38).
- **Steering PID Routine ([`follow_line()`](file:///c:/projects/Robotikka/robotikka/LineSensor.cpp#L125))**:
  - Centroid weighted average position calculation ($-3.5$ to $+3.5$).
  - Full proportional-integral-derivative differential motor speed correction.
  - Trajectory hold across dotted gaps ($< 350\text{ ms}$) and pivot memory when off-track.
  - Outputs directly to TB6612 motor driver (`setMotorASpeed`, `setMotorBSpeed`).
- **Functions**: [`initLineSensors()`](file:///c:/projects/Robotikka/robotikka/LineSensor.cpp#L42), [`readLineSensors()`](file:///c:/projects/Robotikka/robotikka/LineSensor.cpp#L55), [`follow_line()`](file:///c:/projects/Robotikka/robotikka/LineSensor.cpp#L125), [`resetLinePID()`](file:///c:/projects/Robotikka/robotikka/LineSensor.cpp#L34), and [`printLineSensorValues()`](file:///c:/projects/Robotikka/robotikka/LineSensor.cpp#L170).

---

## 6. Codebase File Index

| File | Purpose / Contents |
| :--- | :--- |
| [`robotikka.ino`](file:///c:/projects/Robotikka/robotikka/robotikka.ino) | System setup, FreeRTOS task initializations, `TaskTelemetryOTA` (Core 0), and 500 Hz `TaskRobotControl` loop (Core 1). |
| [`Config.h`](file:///c:/projects/Robotikka/robotikka/Config.h) | Central hardware pin mapping (Rev 2.0 schematic), IR sensor array pins, and timing constants. |
| [`globals.h`](file:///c:/projects/Robotikka/robotikka/globals.h) / [`globals.cpp`](file:///c:/projects/Robotikka/robotikka/globals.cpp) | System-wide enums ([`RunMode`](file:///c:/projects/Robotikka/robotikka/globals.h#L19)), FreeRTOS task handles, queues, and shared flags. |
| [`button_task.cpp`](file:///c:/projects/Robotikka/robotikka/button_task.cpp) | BOOT button interrupt service routine, 8-color menu FSM, debouncing, and [`consumeButtonPress()`](file:///c:/projects/Robotikka/robotikka/button_task.cpp#L67). |
| [`MotorDriver.h`](file:///c:/projects/Robotikka/robotikka/MotorDriver.h) / [`MotorDriver.cpp`](file:///c:/projects/Robotikka/robotikka/MotorDriver.cpp) | TB6612FNG H-bridge motor driver with 20 kHz ultrasonic PWM, coasting, and active braking. |
| [`TOFSensors.h`](file:///c:/projects/Robotikka/robotikka/TOFSensors.h) / [`TOFSensors.cpp`](file:///c:/projects/Robotikka/robotikka/TOFSensors.cpp) | 5x VL53L0X TOF sensors: dynamic addressing sequence (`0x30`-`0x34`) and continuous 50 ms distance streaming. |
| [`ColorSensor.h`](file:///c:/projects/Robotikka/robotikka/ColorSensor.h) / [`ColorSensor.cpp`](file:///c:/projects/Robotikka/robotikka/ColorSensor.cpp) | Adafruit TCS34725 RGB color reader, LED illumination toggle, and majority RGB classifier for NeoPixel. |
| [`ServoControl.h`](file:///c:/projects/Robotikka/robotikka/ServoControl.h) / [`ServoControl.cpp`](file:///c:/projects/Robotikka/robotikka/ServoControl.cpp) | 3-servo robotic arm controller (16-bit 50 Hz PWM), Serial CLI interactive menu, and swing test routine. |
| [`LineSensor.h`](file:///c:/projects/Robotikka/robotikka/LineSensor.h) / [`LineSensor.cpp`](file:///c:/projects/Robotikka/robotikka/LineSensor.cpp) | 8-sensor analog IR reading array on ADC1 pins (GPIO 1 to 8), weighted centroid, differential PID line follower (`follow_line()`), and global PID config (`g_pidConfig`). |
| [`README.md`](file:///c:/projects/Robotikka/robotikka/README.md) | User-facing documentation, quick-start guide, LED animation charts, and pit calibration reference. |

---

## 7. Development, Flashing & Pit Troubleshooting

1. **Arduino IDE 2.x Board Configuration**:
   - **Board**: `ESP32S3 Dev Module`
   - **USB CDC On Boot**: **`Enabled`** *(Critical: native USB-C port will not output Serial Monitor prints if this is disabled)*
   - **Flash Size**: `16MB (128Mb)` / `8MB`
   - **Partition Scheme**: `Default 16MB Flash (for OTA)` or any scheme providing dual app partitions (`app0` / `app1`).
   - **Upload Speed**: `921600`
   - **Baud Rate**: `115200`
2. **OTA Port Bug in Arduino IDE 2.x**:
   - Arduino IDE 2.x may report `invalid int value: '{upload.port.properties.port}'`.
   - Permanent fix: In `C:\Users\tpawi\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.8\platform.txt`, change `"{upload.port.properties.port}"` to `"3232"` in `tools.espota.upload.pattern`.
3. **OTA Host IP Multi-Adapter / WSL Conflict**:
   - When a computer runs WSL2, Docker, or Hyper-V, virtual switches (`vEthernet`) may cause `espota.exe` to broadcast the wrong host IP, resulting in `[ERROR]: No response from the ESP`.
   - Workaround: Explicitly invoke the Python OTA tool specifying the host Wi-Fi interface IP:
     ```powershell
     python $HOME/AppData/Local/Arduino15/packages/esp32/hardware/esp32/3.3.8/tools/espota.py -i <ESP_IP> -p 3232 -I <PC_WIFI_IP> -f <sketch.bin>
     ```
4. **ADC2 & Wi-Fi Incompatibility**:
   - On the ESP32 family, ADC2 channels cannot be read reliably while the Wi-Fi subsystem is active.
   - The firmware explicitly executes `WiFi.mode(WIFI_OFF)` at the start of [`setup()`](file:///c:/projects/Robotikka/robotikka/robotikka.ino#L324) and prior to line following modes to keep all analog channels responsive.
5. **GPIO 48 & GPIO 21 Multi-Function Precedence**:
   - Do not send `neopixelWrite()` commands during the initial boot phase before [`initTOFSensors()`](file:///c:/projects/Robotikka/robotikka/TOFSensors.cpp#L24) has released `XSHUT4` on GPIO 48.
   - GPIO 21 is shared by the status indicator and TCS34725 white LED; turning off status lighting turns off sensor illumination.
