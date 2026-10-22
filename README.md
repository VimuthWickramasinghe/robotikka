# Robotikka - Autonomous Line & Wall Following Robot

Firmware for the **Robotikka** autonomous competition robot built on the **ESP32-S3** microcontroller with a FreeRTOS dual-core architecture, wireless Over-The-Air (OTA) flashing, real-time PID line following, and live servo calibration.

---

## 🚦 LED Status Indicator & Animations

The onboard **WS2812 RGB LED** (`RGB_LED_PIN` / GPIO 48) and external indicator (`STATUS_LED_PIN` / GPIO 21) provide real-time visual feedback on system state without needing a Serial Monitor connection:

| State | Visual Pattern | Color | Meaning / System Status |
| :--- | :--- | :--- | :--- |
| **Idle / Normal Run** | **Smooth Breathing Pulse**<br>(Fade IN 1s, Fade OUT 1s) | **Purple / Magenta**<br>`RGB(180, 0, 255)` | **Normal Competition Mode.**<br>Wi-Fi is completely turned OFF (100% autonomous & competition-legal). Real-time PID control loop is active on Core 1. |
| **Wi-Fi Connecting** | **Rapid Toggle**<br>(100 ms ON / 100 ms OFF) | **Amber / Yellow**<br>`RGB(150, 80, 0)` | **Triggered.**<br>Triple button press detected; board is attempting to associate with the Wi-Fi network. |
| **OTA Armed & Ready** | **Steady Slow Pulse**<br>(500 ms ON / 500 ms OFF) | **Green**<br>`RGB(0, 150, 0)` | **Wireless Upload Ready.**<br>Connected to Wi-Fi, mDNS advertised as `robotikka-s3`, and listening on port `3232` for wireless sketch uploads. |
| **Active Code Flashing** | **Hyper-Fast Strobe**<br>(40 ms rapid flash) | **Bright Magenta / Red**<br>`RGB(200, 0, 100)` | **Writing Flash Memory.**<br>New firmware sketch is actively transmitting over Wi-Fi and being written into ESP32-S3 flash. |

---

## ⚡ How to Trigger Wireless OTA Uploads

Per **UOK Robot Race rules**, wireless radios (Wi-Fi/Bluetooth) must be turned **OFF** during competition runs.

To flash code wirelessly during pit testing without a USB cable:

1. Power on the robot (battery or USB).
2. Press the onboard **BOOT button (GPIO 0)** once to start the color menu.
3. Press the button again when the LED shows **Green** (OTA Mode).
4. The LED will blink **Amber** while connecting to Wi-Fi, then **Slow Green Pulse** when ready.
5. In Arduino IDE:
   * Select **Tools > Port > Network ports > `robotikka-s3 at <IP_ADDRESS>`**.
   * Click **Upload**.
6. The LED will strobe **Magenta/Red** during the write and automatically reboot into the new firmware!

---

## 🛠️ Serial Monitor Commands - Live Testing & Calibration

Open the Arduino IDE **Serial Monitor** at **115200 baud** (with line ending set to **Newline**). Real-time command interface for testing servos and reading sensor data:

### IR Sensor Commands:
* `ir` : Print a detailed single snapshot table of all 8 IR sensors (raw ADC/digital values, black/white threshold status, active count, and computed centroid position).
* `irstream` : Toggle continuous real-time streaming of all 8 sensor readings (printed every 100 ms). Send `irstream` again to turn off.

### Servo Control Commands:
* `s1 <angle>` : Move **Servo 1** (GPIO 15 / Shoulder) to angle ($0^\circ \text{ to } 180^\circ$). Example: `s1 90`
* `s2 <angle>` : Move **Servo 2** (GPIO 16 / Elbow) to angle ($0^\circ \text{ to } 180^\circ$). Example: `s2 45`
* `s3 <angle>` : Move **Servo 3** (GPIO 17 / Gripper) to angle ($0^\circ \text{ to } 180^\circ$). Example: `s3 120`
* `all <angle>` : Move **all 3 servos** simultaneously to the same angle. Example: `all 90`
* `sweep <1|2|3>` : Run an automatic smooth sweep test ($0^\circ \to 180^\circ \to 0^\circ$) on the selected servo. Example: `sweep 1`
* `status` : Print current angle of all 3 servos.

### General Commands:
* `help` : Display the interactive command menu.

---

## 📁 File Structure

```text
robotikka/
├── README.md             # Quick start, LED animations & serial command guide
├── AGENT.md              # Deep technical context, competition rules & architecture
├── Config.h              # Central GPIO pin definitions (updated pin scheme)
├── MotorDriver.h / .cpp  # TB6612FNG 20 kHz silent PWM driver (forward, reverse, brake)
├── LineSensor.h / .cpp   # 8-IR sensor array reader, centroid calculator & dotted line gap hold
├── LinePID.h / .cpp      # High-speed differential steering PID controller
├── ServoControl.h / .cpp # 16-bit 50 Hz RC servo controller with interactive command interface
└── robotikka.ino         # Main firmware entry point with dual-core FreeRTOS supervisor
```

---

## ⚙️ IDE Setup & Board Settings

When using the **Arduino IDE 2.x**:

* **Board**: `ESP32S3 Dev Module`
* **Upload Speed**: `921600`
* **USB CDC On Boot**: **`Enabled`** *(Required for native USB Serial Monitor output)*
* **Baud Rate**: **`115200`**
* **Partition Scheme**: `Default 16MB Flash` (for OTA support)

---

## 🏆 Competition Tasks & Robot States

### 4 Competition Tasks (UOK Robot Race 2K26):

1. **Task 1: Line Following** - Follow black line (2-3 cm wide) from start square; handle dotted segments (2-5 cm gaps).
2. **Task 2: Pick Up Box & Color Identification** - Detect and grasp colored box (5 × 5 × 5 cm) in curved track zone. Identify color (Red, Green, Blue) and indicate via LEDs/Display.
3. **Task 3: Wall Following** - Navigate curved track section using Time-of-Flight (TOF) distance sensors while maintaining safe distance and avoiding wall collision.
4. **Task 4: Precision Placement** - Deliver and place box accurately into matching color drop zone (10 × 10 cm).

### Critical Rules:
* Size limit: 20 cm × 20 cm at start; no height limit; extensions allowed only during operation.
* Power: Internal battery only, max 24 V.
* **Autonomy & Wireless Rule**: All wireless communication (Wi-Fi, Bluetooth, radio) must be terminated before placing on the arena. External wireless interference during a run results in immediate disqualification.
* Activation: Onboard physical buttons only.

---

## 🔧 FreeRTOS Dual-Core Architecture

### Core 0: Wireless Telemetry & UI (`TaskTelemetryOTA`, Priority 1)
- **Wi-Fi / ArduinoOTA**: Powers up **OFF** (`WiFi.mode(WIFI_OFF)`) for competition compliance. Activated on-demand via **color selection button menu** (select Green = OTA Mode). Listens on port `3232` with hostname `robotikka-s3`.
- **Status LED Animations** (`setLedOutput`, `updateLedAnimation`):
  - Offline Idle: Breathing vibrant Purple/Magenta (2s cycle).
  - Wi-Fi Connecting: Fast Amber/Yellow toggle (100ms).
  - OTA Ready: Steady Green pulse (500ms ON / 500ms OFF).
  - Active Flashing: Hyper-fast Magenta/Red strobe (40ms).
  - Color Menu: Each color represents a command (Red=linefollow, Green=OTA, Blue=IR calibrate, others=future).
- **Button Color Menu** (`buttonLogicTask`):
  - Press BOOT button → cycles through 8 colors (500ms each) on RGB LED.
  - Press again at desired color → enters confirmation mode.
  - Press again within 3s → executes selected command.
  - Timeout → resumes color cycling.
- **Serial Test Command Parser** (`handleServoSerialCommands`): Real-time command parsing for live servo adjustment (`s1 90`, `sweep 1`, `status`, etc.).

### Core 1: Real-Time Robot Control (`TaskRobotControl`, Priority 8)
- **Deterministic Loop Rate**: Strict **500 Hz (2 ms)** loop via `vTaskDelayUntil()` (defined as `pdMS_TO_TICKS(2)` in `robotikka.ino:134`).
- **Subsystems Handled**:
  - Sensor acquisition (8-IR array).
  - High-speed differential PID steering (P=45, I=0, D=25, limit=160).
  - TB6612 motor PWM generation at **20 kHz** (silent, no acoustic whine).
  - TOF sensor reading and wall following distance regulation.

### Run Modes:
* `LINE_FOLLOW` - Primary autonomous mode, 500 Hz control loop on Core 1.
* `OTA_MODE` - Wireless upload mode, triggered by selecting Green color in button menu.
* `IR_CALIBRATE` - IR sensor calibration mode.
* Default: Tasks suspended until button selection enables them.

---

## 🔌 Pin Definitions (Updated Scheme)

Reflecting the merged pin definitions from `Config.h`:

| Subsystem | Pin | GPIO | Function |
|-----------|-----|------|----------|
| **Motor Driver (TB6612FNG)** | PWMA | **38** | Motor A PWM Speed Control |
| | AIN1 | **34** | Motor A Direction 1 |
| | AIN2 | **35** | Motor A Direction 2 |
| | PWMB | 9 | Motor B PWM Speed Control |
| | BIN1 | **36** | Motor B Direction 1 |
| | BIN2 | **37** | Motor B Direction 2 |
| **8-Sensor IR Array** | IR_1 | **1** | GPIO1 (ADC1_CH0) |
| | IR_2 | **2** | GPIO2 (ADC1_CH1) |
| | IR_3 | **3** | GPIO3 |
| | IR_4 | **4** | GPIO4 |
| | IR_5 | **5** | GPIO5 |
| | IR_6 | **6** | GPIO6 |
| | IR_7 | **7** | GPIO7 |
| | IR_8 | **8** | GPIO8 |
| **Arm Servos (U7, U8, U9)** | PWM1 | 15 | Servo U7 (Shoulder/Arm) |
| | PWM2 | 16 | Servo U8 (Elbow/Tilt) |
| | PWM3 | 17 | Servo U9 (Gripper) |
| **Shared I2C Bus** | SDA | 11 | Display, Color Sensor, TOF Panel |
| | SCL | 12 | I2C Clock |
| **TOF Distance Sensors** | XSHUT1 | 13 | Sensor 1 Enable |
| | XSHUT2 | 14 | Sensor 2 Enable |
| | XSHUT3 | 10 | Sensor 3 Enable |
| | XSHUT4 | 48 | Sensor 4 Enable |
| **Color Sensor** | INT | 18 | Color Sensor Interrupt |
| | LED | 21 | Color Sensor Illumination / Status LED |
| **User Interface** | BUTTON | 0 | BOOT button (color selection menu) |
| | RGB_LED | 48 | Onboard WS2812 RGB NeoPixel |
| | STATUS_LED | 21 | Onboard indicator / Color Sensor LED |

**Note**: GPIO21 and GPIO48 are shared between color sensor LED and status/NeoPixel functions. WiFi must be OFF (`WiFi.mode(WIFI_OFF)`) before using ADC2 channels for IR sensors.

---

## 📝 Development & Troubleshooting Notes

1. **OTA Port Bug in Arduino IDE 2.x**:
   - Arduino IDE 2.x may report `invalid int value: '{upload.port.properties.port}'`.
   - Permanent fix: In `C:\Users\tpawi\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.8\platform.txt`, change `"{upload.port.properties.port}"` to `"3232"` in `tools.espota.upload.pattern`.

2. **OTA Host IP Multi-Adapter Conflict**:
   - If PC has WSL/Hyper-V virtual network adapters (`vEthernet`), `espota.exe` may broadcast the virtual adapter IP causing `[ERROR]: No response from the ESP`.
   - Explicitly run `espota.py` with `-I <PC_WIFI_IP>` or disable virtual switches during pit calibration if needed.

3. **ESP32-S3 USB CDC Serial**:
   - In Arduino IDE **Tools** menu, ensure **USB CDC On Boot: Enabled** so `Serial.begin(115200)` prints directly over the native USB-C port.

4. **ADC2 Unlock Requirement**:
   - Firmware explicitly sets `WiFi.mode(WIFI_OFF)` at startup (line 163 of `robotikka.ino`) to allow IR sensor ADC2 operations.

5. **Motor Driver Note**:
    - STBY (Standby) is hardwired to +3.3V in hardware (always enabled). Motor PWM runs at 20 kHz for silent operation.

6. **Color-Based Button System**:
    - Replaces triple-click OTA trigger.
    - 8 color options mapped to run modes (Red=linefollow, Green=OTA, Blue=IR calibrate).
    - Confirmation requires 2nd press within 3 seconds to prevent accidental activation.