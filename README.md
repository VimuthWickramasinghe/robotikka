# Robotikka - Autonomous Line & Wall Following Robot

Firmware for the **Robotikka** autonomous competition robot built on the **ESP32-S3** microcontroller with a FreeRTOS dual-core architecture, 8-color mode selection, wireless Over-The-Air (OTA) flashing, real-time PID line following, and live servo calibration.

---

## 🚦 System Modes & 8-Color Button Selector

Mode selection and system control use the onboard **BOOT button (GPIO 0)** and **RGB NeoPixel** (`RGB_BUILTIN` / GPIO 48/38) alongside the external indicator ([`STATUS_LED_PIN`](file:///c:/projects/Robotikka/robotikka/Config.h#L60) / GPIO 21).

### Operating Instructions:
1. **At Power-On / Idle**: All status LEDs remain OFF.
2. **Start Mode Selector**: Click the onboard **BOOT button** once. The LED will immediately start cycling through the 8 modes (one color every 600 ms).
3. **Select Mode**: Click the BOOT button when the desired color is shown. The LED enters a **fast blink (150 ms)** confirmation state for **3 seconds**.
4. **Confirm Mode**: Click the BOOT button once more within the 3-second window. The mode is locked in, the corresponding FreeRTOS tasks start, and the LED turns off.
5. **Cancel / Timeout**: If you do not click within 3 seconds, confirmation times out and cycling resumes.

### Available System Modes:

| Mode # | Color | RGB Value | Mode Name | System Behavior |
| :---: | :--- | :--- | :--- | :--- |
| **1** | **Red** | `(255, 0, 0)` | **Line Follow** | Wi-Fi completely OFF. Dual-core PID line tracking active on Core 1 at 500 Hz. |
| **2** | **Green** | `(0, 255, 0)` | **OTA Mode** | Wi-Fi connects to AP, listens on port `3232` for wireless code flashing. Motors suspended. |
| **3** | **Blue** | `(0, 0, 255)` | **IR Calibrate** | Real-time IR array testing and analog threshold verification. |
| **4** | **Yellow** | `(255, 255, 0)` | **Command 4** | User customizable mission slot. |
| **5** | **Magenta** | `(255, 0, 255)` | **Command 5** | User customizable mission slot. |
| **6** | **Cyan** | `(0, 255, 255)` | **Command 6** | User customizable mission slot. |
| **7** | **Purple** | `(128, 0, 255)` | **Command 7** | User customizable mission slot. |
| **8** | **Orange** | `(255, 128, 0)` | **Command 8** | User customizable mission slot. |

---

## 📡 IR Line Following Array Layout & Single-Line Telemetry

The custom 8-sensor analog photodiode array connects directly to ESP32-S3 `ADC1` pins.

### Physical & Functional Mapping:
```text
           [ FRONT OF ROBOT ]
   Left Turn                         Right Turn
    [ A7 ]                             [ A1 ]
           [ A6 ]  [ A5 ]  [ A4 ]  [ A3 ]  [ A2 ]
          ( -2.0 ) (-1.0 ) ( 0.0 ) (+1.0 ) (+2.0 )
           <----- Center 5 PID Line Sensors ----->
                           
                           [ A8 ]
                        ( Back Ref )
```

| Array Index | Sensor | ESP32-S3 GPIO | ADC Channel | Functional Role | PID Weight |
| :---: | :---: | :---: | :---: | :--- | :---: |
| **`[0]`** | **A6** | **GPIO 5** | `ADC1_CH4` | Center PID (Far Left) | `-2.0` |
| **`[1]`** | **A5** | **GPIO 6** | `ADC1_CH5` | Center PID (Mid Left) | `-1.0` |
| **`[2]`** | **A4** | **GPIO 7** | `ADC1_CH6` | Center PID (Dead Center) | `0.0` |
| **`[3]`** | **A3** | **GPIO 3** | `ADC1_CH2` | Center PID (Mid Right) | `+1.0` |
| **`[4]`** | **A2** | **GPIO 2** | `ADC1_CH1` | Center PID (Far Right) | `+2.0` |
| **`[5]`** | **A1** | **GPIO 1** | `ADC1_CH0` | **RIGHT** 90° Turn Marker | *Turn flag* |
| **`[6]`** | **A7** | **GPIO 4** | `ADC1_CH3` | **LEFT** 90° Turn Marker | *Turn flag* |
| **`[7]`** | **A8** | **GPIO 10** | `ADC1_CH9` | **BACK** / Rear Intersection Marker | *Aux flag* |

### Real-Time Serial Stream Format:
Readings print as a rate-limited, single-line telemetry string (10 Hz):
```text
PID:[A6:2450 A5:2300 A4:1950 A3:2104 A2:2859]  R(A1):2613  L(A7):2100  B(A8):1950
```

---

## 🛠️ Testing & Calibration via Serial Monitor

Open the Arduino IDE **Serial Monitor** at **115200 baud** (with line ending set to **Newline**):

### Available Commands:
* `ir` : Print immediate snapshot of all 8 analog sensor values.
* `irstream` : Toggle continuous real-time single-line sensor streaming (every 100 ms).
* `s1 <angle>` : Move **Servo 1** (GPIO 15 / Shoulder) to angle ($0^\circ \text{ to } 180^\circ$). Example: `s1 90`
* `s2 <angle>` : Move **Servo 2** (GPIO 16 / Elbow) to angle ($0^\circ \text{ to } 180^\circ$). Example: `s2 45`
* `s3 <angle>` : Move **Servo 3** (GPIO 17 / Gripper) to angle ($0^\circ \text{ to } 180^\circ$). Example: `s3 120`
* `all <angle>` : Move **all 3 servos** simultaneously to the same angle. Example: `all 90`
* `sweep <1|2|3>` : Run an automatic smooth sweep test ($0^\circ \to 180^\circ \to 0^\circ$) on the selected servo.
* `status` : Print current angles of all 3 servos.
* `help` : Display the interactive command menu.

---

## 📁 File Structure

```text
robotikka/
├── README.md             # Mode selector, IR array pinout & Serial testing guide
├── AGENT.md              # Deep technical context, competition rules & architecture
├── Config.h              # Central GPIO pin definitions & resolved pin mapping
├── button_task.cpp       # 8-color mode selector state machine (FreeRTOS Core 0)
├── LineSensor.h / .cpp   # 8-channel analog reader, 5-sensor centroid & single-line print
├── LinePID.h / .cpp      # High-speed differential steering PID controller
├── MotorDriver.h / .cpp  # TB6612FNG 20 kHz silent PWM driver (forward, reverse, brake)
├── ServoControl.h / .cpp # 16-bit 50 Hz RC servo controller with interactive command interface
├── globals.h / .cpp      # FreeRTOS task handles and shared RunMode state
└── robotikka.ino         # Main firmware entry point with dual-core FreeRTOS supervisor
```

---

## ⚙️ IDE Setup & Board Settings

When using **Arduino IDE 2.x**:
* **Board**: `ESP32S3 Dev Module`
* **Upload Speed**: `921600`
* **USB CDC On Boot**: **`Enabled`** *(Required for native USB Serial Monitor communication)*
* **Baud Rate**: **`115200`**
