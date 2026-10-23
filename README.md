# Robotikka - Autonomous Line & Wall Following Robot

Firmware for the **Robotikka** autonomous competition robot built on the **ESP32-S3** microcontroller with a FreeRTOS dual-core architecture, wireless Over-The-Air (OTA) flashing, real-time PID line following, dynamic Time-of-Flight (TOF) wall tracking, TCS34725 color detection, and 3-axis robotic arm control.

---

## 📋 Table of Contents
1. [Arduino IDE & Library Setup](#-arduino-ide--library-setup)
2. [Board Configuration & Tools Menu](#-board-configuration--tools-menu)
3. [Required Libraries & Installation](#-required-libraries--installation)
4. [Headless 8-Color Command Selector](#-headless-8-color-command-selector)
5. [Over-The-Air (OTA) Wireless Flashing](#-over-the-air-ota-wireless-flashing)
6. [Interactive Serial CLI Commands](#-interactive-serial-cli-commands)
7. [Hardware Architecture & Pinout Reference](#-hardware-architecture--pinout-reference)
8. [FreeRTOS Firmware Architecture](#-freertos-firmware-architecture)
9. [PID Line Following & Tuning](#-pid-line-following--tuning)
10. [Troubleshooting & Pit Guide](#-troubleshooting--pit-guide)

---

## 🛠️ Arduino IDE & Library Setup

### 1. Arduino IDE Version
Use **Arduino IDE 2.x** (v2.2.0 or higher recommended). Download from [arduino.cc](https://www.arduino.cc/en/software).

### 2. ESP32 Board Package
The firmware is built for the **ESP32-S3** microcontroller using the official Espressif board definitions:
1. In Arduino IDE, go to **File > Preferences** (or `Ctrl + ,`).
2. In **Additional boards manager URLs**, add:
   ```text
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. Open **Tools > Board > Boards Manager...** (or click the board icon in the left toolbar).
4. Search for **`esp32`** by **Espressif Systems** and install version **`3.x`** (e.g., `3.0.x` or `3.3.x`).
   > [!IMPORTANT]
   > Version 3.x is required because the firmware uses the modern ESP-IDF 5.x / Arduino ESP32 3.x LEDC API (`ledcAttach()`).

---

## ⚙️ Board Configuration & Tools Menu

Select **Tools > Board > esp32 > ESP32S3 Dev Module**, then configure the following settings:

| Setting | Value | Reason / Notes |
| :--- | :--- | :--- |
| **Board** | `ESP32S3 Dev Module` | Target board (`ESP32-S3-DEV-KIT-N8R8`) |
| **USB CDC On Boot** | **`Enabled`** | **CRITICAL**: Enables native USB-C Serial Monitor (`Serial.begin(115200)`) |
| **CPU Frequency** | `240MHz (WiFi)` | Maximum performance for 500 Hz control loop |
| **Flash Mode** | `QIO 80MHz` | Quad SPI Flash speed |
| **Flash Size** | `8MB (64Mb)` or `16MB (128Mb)` | Matches onboard N8R8 (8MB Flash / 8MB PSRAM) |
| **Partition Scheme** | `Default 16MB Flash (for OTA)` or `Minimal SPIFFS (Large APPS with OTA)` | Dual app partitions (`app0` / `app1`) required for OTA |
| **PSRAM** | `OPI PSRAM` | Enables onboard 8MB Octal SPI PSRAM |
| **Upload Speed** | `921600` | Fast flashing speed over native USB |
| **Core Debug Level** | `None` (or `Verbose` during debugging) | Standard console output |

---

## 📦 Required Libraries & Installation

Open **Tools > Manage Libraries...** (or `Ctrl + Shift + I`) in the Arduino IDE and install the following libraries:

### 1. `Adafruit TCS34725`
- **Author**: Adafruit
- **Purpose**: I2C color sensor driver for box color detection (Task 2).
- **Dependencies**: When prompted, click **"Install All"** to include `Adafruit BusIO`.

### 2. `Adafruit_VL53L0X`
- **Author**: Adafruit
- **Purpose**: Time-of-Flight distance sensor driver for the 5-sensor wall following panel (Task 3).
- **Dependencies**: Automatically utilizes `Adafruit BusIO` and `Wire`.

### 3. `ESP32Servo`
- **Author**: Kevin Harrington, John K. Bennett
- **Purpose**: RC servo motor controller with dedicated ESP32 LEDC timers for arm/gripper actuation (Task 2 & 4).

### 4. `U8g2`
- **Author**: olikraus
- **Purpose**: SSD1306 128×64 OLED display for live IR sensor values.

### Core Built-in Libraries (No Installation Needed):
The following libraries are included with the ESP32 board package:
- `WebServer.h` — HTTP Web Server for calibration dashboard and live PID tuning.
- `WiFi.h`, `ESPmDNS.h`, `NetworkUdp.h`, `ArduinoOTA.h` — Over-The-Air wireless updates.
- `Wire.h` — Hardware I2C communication (`SDA=11`, `SCL=12`).
- `freertos/FreeRTOS.h`, `freertos/task.h`, `freertos/semphr.h` — Real-time operating system primitives.

---

## 🚦 Headless 8-Color Command Selector

The single onboard **BOOT button (GPIO 0)** and **WS2812 RGB LED (GPIO 48)** allow quick pit testing and autonomous mode execution without requiring a PC or USB cable:

```
  [BOOT Button Press]
           |
           v
  (Wakes buttonLogicTask via ISR)
           |
           v
  [Cycles 8 Colors on NeoPixel (500ms each)]
           |
           +---> Press BOOT on desired color
                    |
                    v
           [Blinks selected color for 3 seconds]
                    |
                    +---> Press BOOT again within 3s  --> CONFIRMED & DISPATCHED
                    |
                    +---> Timeout (no press)          --> Aborted, resumes cycle
```

### 8-Color Mapping Table:

| # | Color | RGB Value | Mode / Command | Target Action & Subsystem |
| :-: | :--- | :--- | :--- | :--- |
| **0** | **Red** | `(255, 0, 0)` | `linefollow` | **Task 1: Line Following**: Launches Core 1 500 Hz PID control loop (`follow_line()`). |
| **1** | **Green** | `(0, 255, 0)` | `OTA mode` | **Wireless Upload**: Turns on Wi-Fi STA, connects to pit network, arms ArduinoOTA on port 3232. |
| **2** | **Blue** | `(0, 0, 255)` | `IR calibrate` | **Web Calibration & Live PID**: Hosts SoftAP (`Robotikka-Calib`, `192.168.4.1`) with 4-sample array calibration & live PID regulation. |
| **3** | **Yellow** | `(255, 255, 0)` | `command4` | **Task 2: Live Color Detect**: Reads TCS34725 with white LED ON; NeoPixel mirrors dominant color (R, G, or B). |
| **4** | **Magenta** | `(255, 0, 255)` | `command5` | **Motor PWM Sweep**: Triangular ramp ($0 \to 255 \to 0$) driving both motors forward at 20 kHz. |
| **5** | **Cyan** | `(0, 255, 255)` | `command6` | **Servo Swing Test**: Arm Servos 1 & 2 oscillate between 30° and 90° using `ESP32Servo`. |
| **6** | **Purple** | `(128, 0, 255)` | `command7` | Reserved for competition flow integration. |
| **7** | **Orange** | `(255, 128, 0)` | `command8` | **Task 3: TOF Distance Stream**: Polls all 5 VL53L0X sensors in continuous mode every 60 ms. |

> [!TIP]
> **Exiting Any Test Mode**: While a test mode (Color Detect, Motor Sweep, Servo Swing, or TOF Stream) is running, press the **BOOT button** once. The robot will safely halt motors, center servos to 90°, turn off lights, and return to idle.

---

## ⚡ Over-The-Air (OTA) Wireless Flashing

During arena runs, rules mandate **Wi-Fi must be OFF**. During pit tuning, you can upload sketches wirelessly:

1. Configure your network SSID & password in [`robotikka.ino`](file:///c:/projects/Robotikka/robotikka/robotikka.ino#L13-L14):
   ```cpp
   const char *ssid = "YOUR_WIFI_SSID";
   const char *password = "YOUR_WIFI_PASSWORD";
   ```
2. Power on the robot.
3. Press **BOOT (GPIO 0)** to initiate color cycling, and press again when the LED shows **Green**.
4. The LED will flash **Amber** while connecting, then emit a **Slow Green Pulse** when armed.
5. In Arduino IDE:
   - Go to **Tools > Port > Network ports > `robotikka-s3 at <IP_ADDRESS>`**.
   - Click **Upload**.
6. The LED will strobe **Magenta/Red** during write and reboot into the new firmware.

---

## 💻 Interactive Serial CLI Commands

Open the **Serial Monitor** at **115200 baud** with **Newline** ending (`Ctrl + Shift + M`).

| Command | Action |
| :--- | :--- |
| `help` | Prints the interactive diagnostic command menu. |
| `status` | Prints current recorded angles for Servos 1, 2, and 3. |
| `s1 <deg>` | Sets Servo 1 (GPIO 15 / Shoulder) to `<deg>` (0° to 180°). E.g., `s1 90`. |
| `s2 <deg>` | Sets Servo 2 (GPIO 16 / Elbow) to `<deg>` (0° to 180°). E.g., `s2 45`. |
| `s3 <deg>` | Sets Servo 3 (GPIO 17 / Gripper) to `<deg>` (0° to 180°). E.g., `s3 120`. |
| `all <deg>` | Sets all 3 servos simultaneously to `<deg>`. E.g., `all 90`. |
| `sweep <1\|2\|3>` | Executes a smooth automated sweep ($0^\circ \to 180^\circ \to 0^\circ$) on the selected servo. |
| `ir` | Prints an instantaneous snapshot of the five PID IR sensor readings and computed line centroid. |

---

## 🔌 Hardware Architecture & Pinout Reference

Reflecting schematic **Rev 2.0** and [`Config.h`](file:///c:/projects/Robotikka/robotikka/Config.h):

| Subsystem | Pin Name | GPIO | Notes / Operating Characteristics |
| :--- | :--- | :--- | :--- |
| **TB6612FNG Dual DC Motor Driver** | `PWMA` | **GPIO 39** | Left Motor PWM (20 kHz ultrasonic frequency) |
| | `AIN1` | **GPIO 35** | Left Motor Direction 1 |
| | `AIN2` | **GPIO 36** | Left Motor Direction 2 |
| | `PWMB` | **GPIO 9** | Right Motor PWM (20 kHz ultrasonic frequency) |
| | `BIN1` | **GPIO 37** | Right Motor Direction 1 |
| | `BIN2` | **GPIO 38** | Right Motor Direction 2 |
| | `STBY` | *+3.3V* | Hardwired enabled in PCB |
| **Arm Servos (`U7, U8, U9`)** | `PWM1` | **GPIO 15** | Servo 1: Shoulder / Arm (16-bit 50 Hz PWM) |
| | `PWM2` | **GPIO 16** | Servo 2: Elbow / Tilt (16-bit 50 Hz PWM) |
| | `PWM3` | **GPIO 17** | Servo 3: Gripper (16-bit 50 Hz PWM) |
| **Shared I2C Bus** | `SDA` | **GPIO 11** | Shared by Display, TCS34725, and TOF sensor array |
| | `SCL` | **GPIO 12** | I2C Clock line |
| **5x TOF Sensor Panel** | `XSHUT1` | **GPIO 13** | Front Center Sensor $\to$ Dynamically addressed to `0x30` |
| | `XSHUT2` | **GPIO 14** | Right 60° Sensor $\to$ Dynamically addressed to `0x31` |
| | `XSHUT3` | **GPIO 10** | Front Left Sensor $\to$ Dynamically addressed to `0x32` |
| | `XSHUT4` | **GPIO 48** | Left 60° Sensor $\to$ Dynamically addressed to `0x33` (shares net with RGB LED) |
| | *(None)* | — | Front Right Sensor $\to$ Addressed 1st to `0x34` (no XSHUT wired) |
| **TCS34725 Color Sensor** | `INT` | **GPIO 18** | Color detection interrupt |
| | `LED` | **GPIO 21** | White illumination LED control / Onboard status LED |
| **SSD1306 OLED** | `SDA` / `SCL` | **GPIO 11 / 12** | I²C address `0x3C`; live IR values in line-follow and IR calibration modes |
| **8-IR Line Sensor Array** | `IR_2` – `IR_6` | **GPIO 2 – 6** | The line-following PID reads only these five analog sensors |
| **User Interface** | `BOOT` | **GPIO 0** | Active LOW button with internal pull-up and falling-edge ISR |
| | `RGB_LED` | **GPIO 48** | Onboard WS2812 RGB NeoPixel |
| | `STATUS_LED` | **GPIO 21** | External status indicator |

---

## 🔧 FreeRTOS Firmware Architecture

```
 Core 0: Background & UI                      Core 1: Real-Time Control & Calib
 +----------------------------------+         +----------------------------------+
 | buttonLogicTask (Priority 4)     |         | TaskRobotControl (Priority 8)    |
 | - Resumed by GPIO 0 ISR          |         | - Strict 500 Hz (2 ms) loop      |
 | - 8-color FSM on WS2812 NeoPixel |         | - follow_line() PID steering     |
 +-----------------+----------------+         | - TCS34725 box color detection   |
                   | sets mode                | - 5x VL53L0X wall distance PID   |
                   v                          | - 20 kHz TB6612 motor control    |
 +----------------------------------+         +----------------------------------+
 | TaskTelemetryOTA (Priority 1)    |         | TaskCalibration (Priority 3)     |
 | - ArduinoOTA handler (port 3232) |         | - Dedicated IR sensor calibrate  |
 | - handleServoSerialCommands()    |         | - Live dynamic min/max tracking  |
 | - serviceIRDisplay() (OLED loop) |         | - Auto midpoint threshold calc   |
 +----------------------------------+         +----------------------------------+
```

---

## 📈 PID Line Following & Tuning

Line following is executed inside [`follow_line()`](file:///c:/projects/Robotikka/robotikka/LineSensor.cpp#L114) at 500 Hz:

### Global Configuration (`g_pidConfig`):
The PID parameters and thresholds are exposed globally in [`LineSensor.h`](file:///c:/projects/Robotikka/robotikka/LineSensor.h#L19):
```cpp
PIDConfig g_pidConfig = {
  .Kp = 45.0f,          // Proportional gain: corrects position error
  .Ki = 0.0f,           // Integral gain: eliminates steady-state bias
  .Kd = 25.0f,          // Derivative gain: dampens oscillations
  .baseSpeed = 160,     // Nominal PWM speed (0 to 255)
  .maxSpeed = 240,      // Maximum motor speed limit
  .minSpeed = -100,     // Allows reverse motor pivot for 90-degree turns
  .maxIntegral = 50.0f, // Anti-windup clamping threshold
  .threshold = 2000     // 12-bit ADC black line detection threshold
};
```

### Steering Logic:
- **Centroid**: Position is calculated across GPIO 2–6 with weights from $-2$ (far left) to $+2$ (far right).
- **Live sensor display**: The SSD1306 OLED shows each sensor's raw ADC value, whether it is above the configured threshold, the threshold itself, and the current line position. Select **IR calibrate** to view readings with the motors stopped; line-follow mode also refreshes the display.
- **Differential Output**:
  $$\text{Motor A (Left)} = \text{baseSpeed} + \text{correction}$$
  $$\text{Motor B (Right)} = \text{baseSpeed} - \text{correction}$$
- **Dotted Lines**: When the line is lost for $< 350\text{ ms}$, the robot holds forward trajectory across the gap. If lost longer, it pivots towards the last detected side.

---

## 🌐 Web Calibration Dashboard & 4-Point IR Sampling

When **Blue (IR calibrate)** is selected in the 8-color button menu, the robot hosts its own Wi-Fi Access Point and Web Server:

- **Wi-Fi SSID**: `Robotikka-Calib`
- **Wi-Fi Password**: `12345678`
- **Web Dashboard URL**: `http://192.168.4.1/` (also printed on the SSD1306 OLED)

### 1. Live PID Parameter Tuning
- Directly adjust $K_p$, $K_i$, $K_d$, base speed, max speed, min speed, and line threshold from your smartphone or laptop.
- Changes are applied instantly to `g_pidConfig` without needing to reflash or open a Serial Monitor.

### 2. 4-Sample IR Array Calibration
Captures 4 distinct arena positions to determine optical reflection bounds for all 5 sensors (GPIO 2–6):
1. **Sample 1 (Middle / Center)**: Robot centered over the black line.
2. **Sample 2 (Right Shift)**: Line shifted towards right sensors.
3. **Sample 3 (Left Shift)**: Line shifted towards left sensors.
4. **Sample 4 (Middle while Moving Forward)**: Captures dynamic reflectance while driving forward under safe PWM.
   - **Toggle Switch**: Can be enabled or disabled directly from the Web UI (`g_calibMoveForwardEnabled`). If disabled, stationary sampling is used.

### 3. Automatic Threshold Calculation
Clicking **"Calculate & Save Thresholds"** evaluates min/max bounds across the 4 samples:
$$\text{threshold}_i = \frac{\min_i + \max_i}{2}$$
Calibrated thresholds are applied globally to `g_sensorThresholds` and `g_pidConfig.threshold`.

### 4. Headless Physical Button Progression
If testing without a phone, pressing the **BOOT button (GPIO 0)** sequentially advances:
- Press 1 → Records Sample 1 (Center)
- Press 2 → Records Sample 2 (Right)
- Press 3 → Records Sample 3 (Left)
- Press 4 → Records Sample 4 (Moving Forward if enabled)
- Press 5 → Computes thresholds, saves, shuts off Wi-Fi, and exits.

---

## 🔍 Troubleshooting & Pit Guide

### 1. Arduino IDE 2.x OTA Port Bug
- **Symptom**: Arduino IDE 2.x fails upload with `invalid int value: '{upload.port.properties.port}'`.
- **Solution**: Open `C:\Users\<USER>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\<VERSION>\platform.txt`, locate `tools.espota.upload.pattern`, and change `"{upload.port.properties.port}"` to `"3232"`.

### 2. Virtual Adapter / WSL Host IP Conflict
- **Symptom**: `espota.exe` reports `[ERROR]: No response from the ESP` despite successful Wi-Fi connection.
- **Solution**: Virtual switches (WSL2, Docker, Hyper-V) broadcast the wrong IP. Run the upload manually with the explicit Wi-Fi host IP:
  ```powershell
  python $HOME/AppData/Local/Arduino15/packages/esp32/hardware/esp32/<VERSION>/tools/espota.py -i <ESP_IP> -p 3232 -I <PC_WIFI_IP> -f <sketch.bin>
  ```

### 3. Native USB CDC Serial Not Outputting
- **Symptom**: Serial Monitor opens but nothing prints.
- **Solution**: In Arduino IDE, ensure **Tools > USB CDC On Boot** is set to **`Enabled`**.

### 4. ADC2 & Wi-Fi Lock
- **Symptom**: Analog readings freeze or produce erroneous values when Wi-Fi is active.
- **Solution**: ESP32 hardware locks ADC2 during Wi-Fi operation. The firmware automatically calls `WiFi.mode(WIFI_OFF)` at boot and before line tracking. The five IR sensors used by the PID are on ADC1 (GPIO 2 to 6).

### 5. TOF Address Clash (`0x29`)
- **Symptom**: Only one TOF sensor initializes; others fail.
- **Solution**: All VL53L0X sensors boot with address `0x29`. The firmware drives all 4 XSHUT lines LOW at boot, re-addresses `TOF_FRONT_RIGHT` (no XSHUT) to `0x34` first, then sequentially activates and addresses each remaining sensor (`0x30` to `0x33`).
