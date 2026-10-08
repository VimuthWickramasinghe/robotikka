# Robotikka - Autonomous Line & Wall Following Robot

Firmware for the **Robotikka** autonomous competition robot built on the **ESP32-S3** microcontroller with a FreeRTOS dual-core architecture, wireless Over-The-Air (OTA) flashing, real-time PID line following, and live servo calibration.

---

## 🚦 LED Status Indicator & Animations

The onboard **WS2812 RGB LED** (`RGB_BUILTIN`) and external indicator (`STATUS_LED_PIN` / GPIO 21) provide real-time visual feedback on system state without needing a Serial Monitor connection:

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
2. Press the onboard **BOOT button (GPIO 0) 3 times rapidly** (within 1.5 seconds).
3. The LED will switch from **Breathing Purple** to **Flashing Amber** while connecting to Wi-Fi.
4. Once connected, the LED switches to a **Slow Green Pulse**, and the board announces its IP address.
5. In Arduino IDE:
   * Select **Tools > Port > Network ports > `robotikka-s3 at <IP_ADDRESS>`**.
   * Click **Upload**.
6. The LED will strobe **Magenta/Red** during the write and automatically reboot into the new firmware!

---

## 🔍 IR Sensor Array Calibration & Diagnostics

You can test and calibrate the 8-sensor IR array (`J4`) live via the Arduino IDE **Serial Monitor** (set baud to **115200**, line ending to **Newline**):

### Available Diagnostic Commands:
* `ir` : Print a detailed single snapshot table of all 8 sensor channels with pin types, raw ADC/digital levels, line detection threshold status, active count, and computed centroid line position.
* `irstream` : Toggle real-time continuous streaming of all 8 sensor channels (streamed every 100 ms). Send `irstream` again to turn off.

### Sample Snapshot Output (`ir`):
```text
======================================================================
 [IR SENSOR READINGS] (Schematic Rev 2.0 - 8-Sensor Array J4)
----------------------------------------------------------------------
 Sensor :  IR_1    IR_2    IR_3    IR_4    IR_5    IR_6    IR_7    IR_8
 GPIO   :  (G1)    (G2)    (G38)   (G39)   (G40)   (G41)   (G42)   (G47)
 Type   : [ADC1]  [ADC1]   [DIG]   [DIG]   [DIG]   [DIG]   [DIG]   [DIG]
----------------------------------------------------------------------
 Raw Val:  3450    3210       1       0       0       0       0       0
 Status :   BLK     BLK     BLK     ---     ---     ---     ---     ---
----------------------------------------------------------------------
 Active Sensors: 3/8 | Line Found: YES | Dotted Gap: NO
 Computed Position: -2.33  (Left: -3.5 <--- Center: 0.00 ---> Right: +3.5)
======================================================================
```

### Sample Live Stream Output (`irstream`):
```text
[IR LIVE] A1:3420 A2:3100 | D3:1 D4:0 D5:0 D6:0 D7:0 D8:0 | Pos:-2.33 | Found:YES
[IR LIVE] A1:1200 A2:3580 | D3:1 D4:1 D5:0 D6:0 D7:0 D8:0 | Pos:-1.50 | Found:YES
[IR LIVE] A1: 450 A2: 520 | D3:0 D4:1 D5:1 D6:0 D7:0 D8:0 | Pos:+0.00 | Found:YES
```

---

## 🛠️ Testing Servos via the Serial Monitor

Use the same **Serial Monitor** (115200 baud) to calibrate the robotic arm and gripper servos:

### Available Commands:
* `s1 <angle>` : Move **Servo 1** (GPIO 15 / Shoulder) to angle ($0^\circ \text{ to } 180^\circ$). Example: `s1 90`
* `s2 <angle>` : Move **Servo 2** (GPIO 16 / Elbow) to angle ($0^\circ \text{ to } 180^\circ$). Example: `s2 45`
* `s3 <angle>` : Move **Servo 3** (GPIO 17 / Gripper) to angle ($0^\circ \text{ to } 180^\circ$). Example: `s3 120`
* `all <angle>` : Move **all 3 servos** simultaneously to the same angle. Example: `all 90`
* `sweep <1|2|3>` : Run an automatic smooth sweep test ($0^\circ \to 180^\circ \to 0^\circ$) on the selected servo. Example: `sweep 1`
* `status` : Print current angle of all 3 servos.
* `help` : Display the interactive command menu.

---

## 📁 File Structure

```text
robotikka/
├── README.md             # Quick start, LED animations & Serial testing guide
├── AGENT.md              # Deep technical context, competition rules & architecture
├── Config.h              # Central GPIO pin definitions matching Schematic.pdf Rev 2.0
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

