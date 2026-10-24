#include "ServoControl.h"
#include "LineSensor.h"
#include <ESP32Servo.h>
#include "globals.h"

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

// Current recorded angles for raw LEDC control
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

// ==============================================================================
// DUAL-SERVO ARM & GRIPPER IMPLEMENTATION (ESP32Servo)
// ==============================================================================

static Servo armServo;      // Arm joint (GPIO15 / PIN_SERVO_ARM)
static Servo gripperServo;  // Gripper (GPIO16 / PIN_SERVO_GRIPPER)
static bool s_servosAttached = false;

static int currentArmAngle = ARM_RAISE_ANGLE;
static int currentGripperAngle = GRIPPER_RELEASE;

bool initArmServos() {
  if (s_servosAttached) return true;

  if (!armServo.attach(PIN_SERVO_ARM, 500, 2500)) {
    Serial.println("[SERVO] ERROR: Failed to attach arm servo!");
    return false;
  }

  if (!gripperServo.attach(PIN_SERVO_GRIPPER, 500, 2500)) {
    Serial.println("[SERVO] ERROR: Failed to attach gripper servo!");
    armServo.detach();
    return false;
  }

  s_servosAttached = true;
  armServo.write(ARM_RAISE_ANGLE);
  gripperServo.write(GRIPPER_RELEASE);
  currentArmAngle = ARM_RAISE_ANGLE;
  currentGripperAngle = GRIPPER_RELEASE;
  delay(300);

  Serial.println("[SERVO] Arm & Gripper Servos Initialized.");
  return true;
}

void detachArmServos() {
  if (s_servosAttached) {
    armServo.detach();
    gripperServo.detach();
    s_servosAttached = false;
    Serial.println("[SERVO] Arm & Gripper Servos Detached.");
  }
}

// Smooth transition helper
void moveServoSmooth(int servoPin, int fromAngle, int toAngle, int stepDelayMs) {
  if (!s_servosAttached) initArmServos();

  fromAngle = constrain(fromAngle, 0, 180);
  toAngle = constrain(toAngle, 0, 180);
  const int direction = (toAngle >= fromAngle) ? 1 : -1;

  for (int angle = fromAngle; angle != toAngle; angle += direction) {
    if (servoPin == PIN_SERVO_ARM) {
      armServo.write(angle);
    } else if (servoPin == PIN_SERVO_GRIPPER) {
      gripperServo.write(angle);
    }
    delay(stepDelayMs);
  }

  if (servoPin == PIN_SERVO_ARM) {
    armServo.write(toAngle);
    currentArmAngle = toAngle;
  } else if (servoPin == PIN_SERVO_GRIPPER) {
    gripperServo.write(toAngle);
    currentGripperAngle = toAngle;
  }
}

void setArmAngle(int angle) {
  moveServoSmooth(PIN_SERVO_ARM, currentArmAngle, angle, SERVO_STEP_DELAY_MS);
}

void setGripperAngle(int angle) {
  moveServoSmooth(PIN_SERVO_GRIPPER, currentGripperAngle, angle, SERVO_STEP_DELAY_MS);
}

void gripObject() {
  setGripperAngle(GRIPPER_GRIP);
  delay(200); // Allow physical grip to stabilize
}

void releaseObject() {
  setGripperAngle(GRIPPER_RELEASE);
  delay(200); // Allow physical release to settle
}

// Dedicated method to lower arm and grip
void lowerArmAndGrip() {
  if (!s_servosAttached) initArmServos();

  Serial.println("[SERVO] Lowering arm and gripping object...");
  // 1. Ensure gripper is open before lowering
  setGripperAngle(GRIPPER_RELEASE);
  delay(200);

  // 2. Lower arm to pickup position (from 90 down to 30)
  setArmAngle(ARM_LOWER_ANGLE);
  delay(300);

  // 3. Grip the object (0 -> 180)
  gripObject();
}

// Full routine to collect an object
void collectObject() {
  if (!s_servosAttached) initArmServos();

  Serial.println("[SERVO] === Starting COLLECT OBJECT sequence ===");
  // Step 1 & 2 & 3: Lower arm to 30 and grip (180)
  lowerArmAndGrip();

  // Step 4: Raise arm back up (30 up to 90) with object securely held
  setArmAngle(ARM_RAISE_ANGLE);
  delay(300);
  Serial.println("[SERVO] === COLLECT OBJECT sequence complete ===");
}

// Full routine to place/deposit an object
void placeObject() {
  if (!s_servosAttached) initArmServos();

  Serial.println("[SERVO] === Starting PLACE OBJECT sequence ===");
  // Step 1: Lower arm down to surface/drop location (90 down to 30)
  setArmAngle(ARM_LOWER_ANGLE);
  delay(300);

  // Step 2: Release gripper (180 -> 0)
  releaseObject();

  // Step 3: Raise arm back up (30 up to 90) leaving object in place
  setArmAngle(ARM_RAISE_ANGLE);
  delay(300);
  Serial.println("[SERVO] === PLACE OBJECT sequence complete ===");
}

// ==============================================================================
// SERVO_TEST swing mode compatibility
// ==============================================================================

bool servoSwingInit() {
  return initArmServos();
}

void servoSwingWrite(int angle) {
  if (!s_servosAttached) return;
  angle = constrain(angle, 0, 180);
  armServo.write(angle);
  gripperServo.write(angle);
}

void servoSwingDetach() {
  if (s_servosAttached) {
    armServo.write(ARM_RAISE_ANGLE);
    gripperServo.write(GRIPPER_RELEASE);
    delay(200);
    detachArmServos();
  }
}

// ==============================================================================
// SERIAL COMMAND INTERFACE
// ==============================================================================

static void printServoHelp() {
  Serial.println("\n--- ROBOTIKKA SERIAL TEST COMMANDS ---");
  Serial.println(" [SERVO COMMANDS]");
  Serial.println("  s1 <angle>    : Move Servo 1 / Arm (GPIO 15) e.g., 's1 90'");
  Serial.println("  s2 <angle>    : Move Servo 2 / Gripper (GPIO 16) e.g., 's2 45'");
  Serial.println("  s3 <angle>    : Move Servo 3 (GPIO 17) e.g., 's3 180'");
  Serial.println("  all <angle>   : Move all 3 servos e.g., 'all 90'");
  Serial.println("  sweep <1|2|3> : Smooth sweep test 0 -> 180 -> 0");
  Serial.println("  collect       : Test collectObject sequence");
  Serial.println("  place         : Test placeObject sequence");
  Serial.println("  status        : Print current servo angles");
  Serial.println(" [IR SENSOR COMMANDS]");
  Serial.println("  ir            : Print one snapshot of the 5 PID IR sensors");
  Serial.println("  help          : Show this menu");
  Serial.println("---------------------------------------\n");
}

void handleServoSerialCommands() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  String cmd = line;
  cmd.toLowerCase();

  if (cmd == "help") {
    printServoHelp();
  } 
  else if (cmd == "ir") {
    printLineSensorValues();
  }
  else if (cmd == "status") {
    Serial.printf("[SERVO STATUS] Arm (GPIO15): %d° | Gripper (GPIO16): %d° | S3 (GPIO17): %d°\n",
                  currentArmAngle, currentGripperAngle, currentAngle[2]);
  }
  else if (cmd == "collect") {
    collectObject();
  }
  else if (cmd == "place") {
    placeObject();
  }
  else if (cmd == "comp" || cmd == "competition" || cmd == "run") {
    Serial.println("[CLI] Launching full autonomous competition run...");
    g_runMode = COMPETITION_RUN;
    if (hTaskRobotControl != NULL) vTaskResume(hTaskRobotControl);
  }
  else if (cmd.startsWith("s1 ")) {
    int angle = cmd.substring(3).toInt();
    setArmAngle(angle);
  } 
  else if (cmd.startsWith("s2 ")) {
    int angle = cmd.substring(3).toInt();
    setGripperAngle(angle);
  } 
  else if (cmd.startsWith("s3 ")) {
    int angle = cmd.substring(3).toInt();
    setServoAngle(3, angle);
  } 
  else if (cmd.startsWith("all ")) {
    int angle = cmd.substring(4).toInt();
    setArmAngle(angle);
    setGripperAngle(angle);
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
      if (sNum == 1) setArmAngle(a);
      else if (sNum == 2) setGripperAngle(a);
      else setServoAngle(sNum, a);
      delay(25);
    }
    for (int a = 180; a >= 0; a -= 5) {
      if (sNum == 1) setArmAngle(a);
      else if (sNum == 2) setGripperAngle(a);
      else setServoAngle(sNum, a);
      delay(25);
    }
    if (sNum == 1) setArmAngle(90);
    else if (sNum == 2) setGripperAngle(90);
    else setServoAngle(sNum, 90);
    Serial.println("[SERVO] Sweep complete (returned to 90°)");
  } 
  else {
    Serial.printf("[UNKNOWN COMMAND] '%s'. Type 'help' for instructions.\n", line.c_str());
  }
}