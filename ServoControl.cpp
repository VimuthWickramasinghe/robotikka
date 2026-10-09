#include "ServoControl.h"
#include "LineSensor.h"


// Standard RC Servo PWM parameters:
// Frequency: 50 Hz (20ms period)
// Pulse width: ~500us (0 deg) to ~2500us (180 deg)
// Resolution: 16-bit (0 - 65535) for smooth sub-degree precision
static const uint32_t SERVO_FREQ = 50;
static const uint8_t  SERVO_RES  = 16;
static const uint32_t PWM_MAX_DUTY = (1 << SERVO_RES) - 1; // 65535

// Map microseconds to 16-bit duty cycle at 50Hz (period = 20,000 us)
static inline uint32_t usToDuty(uint32_t us) {
  return (us * PWM_MAX_DUTY) / 20000;
}

// Current recorded angles
static int currentAngle[3] = {90, 90, 90};

void initServos() {
  // Attach pins with 50 Hz and 16-bit resolution
  ledcAttach(PIN_SERVO_1, SERVO_FREQ, SERVO_RES);
  ledcAttach(PIN_SERVO_2, SERVO_FREQ, SERVO_RES);
  ledcAttach(PIN_SERVO_3, SERVO_FREQ, SERVO_RES);

  // Set all servos to safe 90-degree center position at startup
  setServoAngle(1, 90);
  setServoAngle(2, 90);
  setServoAngle(3, 90);
}

void setServoAngle(int servoNum, int angle) {
  angle = constrain(angle, 0, 180);
  // Standard servo pulse: 500us (0 deg) to 2500us (180 deg)
  uint32_t pulseUs = map(angle, 0, 180, 500, 2500);
  uint32_t duty = usToDuty(pulseUs);

  switch (servoNum) {
    case 1:
      ledcWrite(PIN_SERVO_1, duty);
      currentAngle[0] = angle;
      break;
    case 2:
      ledcWrite(PIN_SERVO_2, duty);
      currentAngle[1] = angle;
      break;
    case 3:
      ledcWrite(PIN_SERVO_3, duty);
      currentAngle[2] = angle;
      break;
    default:
      Serial.printf("[SERVO ERROR] Invalid servo number %d (choose 1, 2, or 3)\n", servoNum);
      return;
  }

  Serial.printf("[SERVO] Servo %d -> %d deg (%u us)\n", servoNum, angle, pulseUs);
}

static void printServoHelp() {
  Serial.println("\n--- ROBOTIKKA SERIAL TEST COMMANDS ---");
  Serial.println(" [SERVO COMMANDS]");
  Serial.println("  s1 <angle>    : Move Servo 1 (GPIO 15) e.g., 's1 90'");
  Serial.println("  s2 <angle>    : Move Servo 2 (GPIO 16) e.g., 's2 45'");
  Serial.println("  s3 <angle>    : Move Servo 3 (GPIO 17) e.g., 's3 180'");
  Serial.println("  all <angle>   : Move all 3 servos e.g., 'all 90'");
  Serial.println("  sweep <1|2|3> : Smooth sweep test 0 -> 180 -> 0");
  Serial.println("  status        : Print current servo angles");
  Serial.println(" [IR SENSOR COMMANDS]");
  Serial.println("  ir            : Print one snapshot table of all 8 IR sensor readings");
  Serial.println(" [WIFI / OTA COMMANDS]");
  Serial.println("  wifi cred \"SSID\" \"Password\" : Save Wi-Fi credentials to NVM & reconnect");
  Serial.println("  wifi status                  : Check Wi-Fi connection and ESP32 IP");
  Serial.println("  ota                          : Switch to OTA upload mode immediately");
  Serial.println("  help                         : Show this menu");
  Serial.println("---------------------------------------\n");
}

void handleServoSerialCommandsWithLine(const String &line) {
  String cmd = line;
  cmd.toLowerCase();

  if (cmd == "help") {
    printServoHelp();
  } 
  else if (cmd == "ir") {
    printLineSensorValues();
  }
  else if (cmd == "irstream") {
    bool newState = !isIRDebugStreamEnabled();
    setIRDebugStream(newState);
    Serial.printf("[IR STREAM] Live sensor stream %s\n", newState ? "ENABLED (streaming every 100ms)" : "DISABLED");
  }
  else if (cmd == "status") {
    Serial.printf("[SERVO STATUS] S1 (GPIO15): %d° | S2 (GPIO16): %d° | S3 (GPIO17): %d°\n",
                  currentAngle[0], currentAngle[1], currentAngle[2]);
  }
  else if (cmd.startsWith("s1 ")) {
    int angle = cmd.substring(3).toInt();
    setServoAngle(1, angle);
  } 
  else if (cmd.startsWith("s2 ")) {
    int angle = cmd.substring(3).toInt();
    setServoAngle(2, angle);
  } 
  else if (cmd.startsWith("s3 ")) {
    int angle = cmd.substring(3).toInt();
    setServoAngle(3, angle);
  } 
  else if (cmd.startsWith("all ")) {
    int angle = cmd.substring(4).toInt();
    setServoAngle(1, angle);
    setServoAngle(2, angle);
    setServoAngle(3, angle);
  } 
  else if (cmd.startsWith("sweep ")) {
    int sNum = cmd.substring(6).toInt();
    if (sNum < 1 || sNum > 3) {
      Serial.println("[SERVO] Choose servo 1, 2, or 3 for sweep");
      return;
    }
    Serial.printf("[SERVO] Sweeping Servo %d from 0 to 180 and back...\n", sNum);
    for (int a = 0; a <= 180; a += 5) {
      setServoAngle(sNum, a);
      delay(25);
    }
    for (int a = 180; a >= 0; a -= 5) {
      setServoAngle(sNum, a);
      delay(25);
    }
    setServoAngle(sNum, 90);
    Serial.println("[SERVO] Sweep complete (returned to 90°)");
  } 
  else {
    Serial.printf("[UNKNOWN COMMAND] '%s'. Type 'help' for instructions.\n", line.c_str());
  }
}

void handleServoSerialCommands() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;
  handleServoSerialCommandsWithLine(line);
}
