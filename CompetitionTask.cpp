#include "CompetitionTask.h"
#include "LineSensor.h"
#include "WallFollow.h"
#include "TOFSensors.h"
#include "ColorSensor.h"
#include "ServoControl.h"
#include "MotorDriver.h"
#include "Display.h"
#include "globals.h"
#include <freertos/task.h>

// Extern button press consumer for onboard BOOT button fallback
extern bool consumeButtonPress();

// ==============================================================================
// STATE TRACKING & EMERGENCY STOP (GPIO 14)
// ==============================================================================

volatile CompetitionStage g_compStage = STAGE_START_PAD_EXIT;
volatile DetectedColor g_detectedFloorColor = COLOR_UNKNOWN;

static volatile bool s_emergencyStop = false;
static volatile unsigned long s_lastButtonEdgeMs = 0;

void IRAM_ATTR onCompetitionButtonISR() {
    unsigned long now = millis();
    if (now - s_lastButtonEdgeMs < 200) return; // 200ms debounce
    s_lastButtonEdgeMs = now;

    if (g_runMode == COMPETITION_RUN && g_compStage != STAGE_COMPLETED) {
        // --- INSTANT HARDWARE EMERGENCY BRAKE (Microsecond response) ---
        digitalWrite(PIN_MOTOR_AIN1, HIGH);
        digitalWrite(PIN_MOTOR_AIN2, HIGH);
        digitalWrite(PIN_MOTOR_BIN1, HIGH);
        digitalWrite(PIN_MOTOR_BIN2, HIGH);
        analogWrite(PIN_MOTOR_PWMA, 0);
        analogWrite(PIN_MOTOR_PWMB, 0);

        s_emergencyStop = true;
        g_compStage = STAGE_COMPLETED;
        g_runMode = MODE_IDLE;
    } else if (g_runMode == MODE_IDLE) {
        // --- START THE COMPETITION PROCESS ---
        s_emergencyStop = false;
        g_runMode = COMPETITION_RUN;
        if (hTaskRobotControl != NULL) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            vTaskResume(hTaskRobotControl);
        }
    }
}

void initCompetitionButton() {
    pinMode(COMPETITION_BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(COMPETITION_BUTTON_PIN), onCompetitionButtonISR, FALLING);
    Serial.printf("[BUTTON] Competition Start / Instant E-Stop armed on GPIO %d\n", COMPETITION_BUTTON_PIN);
}

void triggerEmergencyStop() {
    s_emergencyStop = true;
    brakeMotors();
    neopixelWrite(RGB_LED_PIN, 255, 0, 0); // Solid RED warning
    Serial.println("\n[EMERGENCY STOP] HALTED INSTANTLY BY GPIO 14 BUTTON!\n");
    displayFrame("EMERGENCY STOP!\nHALTED INSTANTLY");
    g_compStage = STAGE_COMPLETED;
    g_runMode = MODE_IDLE;
}

bool isEmergencyStopTriggered() {
    if (s_emergencyStop) return true;

    // Check GPIO 14 button directly
    if (digitalRead(COMPETITION_BUTTON_PIN) == LOW) {
        triggerEmergencyStop();
        return true;
    }

    // Check onboard BOOT button (GPIO 0) as fallback
    if (consumeButtonPress()) {
        triggerEmergencyStop();
        return true;
    }

    return false;
}

const char* getStageName(CompetitionStage stage) {
    switch (stage) {
        case STAGE_START_PAD_EXIT:        return "START_PAD_EXIT";
        case STAGE_LINE_TO_WALLS:         return "LINE_TO_WALLS";
        case STAGE_WALL_NAVIGATION:       return "WALL_NAVIGATION";
        case STAGE_RAMP_CROSSING:         return "RAMP_CROSSING";
        case STAGE_POST_RAMP_CURVE:       return "POST_RAMP_CURVE";
        case STAGE_FORWARD_TO_COLOR:      return "FORWARD_TO_COLOR";
        case STAGE_CREEPING_COLOR_DETECT: return "CREEPING_COLOR";
        case STAGE_TURNAROUND_AND_BRANCH: return "TURN_AND_BRANCH";
        case STAGE_LINE_TO_PICK:          return "LINE_TO_PICK";
        case STAGE_COLLECT_OBJECT:        return "COLLECT_OBJECT";
        case STAGE_CARRY_TO_DROP:         return "CARRY_TO_DROP";
        case STAGE_PLACE_OBJECT:          return "PLACE_OBJECT";
        case STAGE_DRIVE_TO_END:          return "DRIVE_TO_END";
        case STAGE_COMPLETED:             return "COMPLETED";
        default:                          return "UNKNOWN";
    }
}

void resetCompetitionState() {
    g_compStage = STAGE_START_PAD_EXIT;
    g_detectedFloorColor = COLOR_UNKNOWN;
    s_emergencyStop = false;
    stopMotors();
}

// ==============================================================================
// LOW-LEVEL MANEUVER PRIMITIVES
// ==============================================================================

void executeForwardPulse(int ms, int pwm) {
    if (isEmergencyStopTriggered()) return;
    setMotorASpeed(pwm);
    setMotorBSpeed(pwm);
    vTaskDelay(pdMS_TO_TICKS(ms));
    stopMotors();
}

void executeTurn180() {
    if (isEmergencyStopTriggered()) return;
    Serial.println("[MANEUVER] Executing 180-degree turnaround...");
    // Pivot spin: Right backward, Left forward
    setMotorASpeed(-TURN_SPEED);
    setMotorBSpeed(TURN_SPEED);
    vTaskDelay(pdMS_TO_TICKS(TURN_180_MS));
    brakeMotors();
    vTaskDelay(pdMS_TO_TICKS(150));
}

void executeTurnLeft90() {
    if (isEmergencyStopTriggered()) return;
    Serial.println("[MANEUVER] Executing 90-degree LEFT turn...");
    setMotorASpeed(TURN_SPEED);
    setMotorBSpeed(-TURN_SPEED);
    vTaskDelay(pdMS_TO_TICKS(TURN_90_MS));
    brakeMotors();
    vTaskDelay(pdMS_TO_TICKS(150));
}

void executeTurnRight90() {
    if (isEmergencyStopTriggered()) return;
    Serial.println("[MANEUVER] Executing 90-degree RIGHT turn...");
    setMotorASpeed(-TURN_SPEED);
    setMotorBSpeed(TURN_SPEED);
    vTaskDelay(pdMS_TO_TICKS(TURN_90_MS));
    brakeMotors();
    vTaskDelay(pdMS_TO_TICKS(150));
}

// ==============================================================================
// STAGE IMPLEMENTATIONS
// ==============================================================================

// --- STAGE 0: Exit Black Starting Pad ---
static void stageExitBlackStartPad() {
    if (isEmergencyStopTriggered()) return;
    Serial.println("[COMP] STAGE: START_PAD_EXIT");
    displayFrame("COMPETITION\nSTART PAD EXIT\nMoving Forward...");

    neopixelWrite(RGB_LED_PIN, 0, 255, 0); // Green: active!

    // 1. Initial blind forward pulse off the black pad
    executeForwardPulse(START_BLIND_PULSE_MS, START_BLIND_SPEED);

    // 2. Drive forward until sensors transition from all-black to white with line
    unsigned long startTimeout = millis();
    while (millis() - startTimeout < 1500) {
        if (isEmergencyStopTriggered()) return;

        LineSensorState state = readLineSensors();

        // If line is acquired and not all sensors are black
        if (state.lineFound && state.activeCount <= 3) {
            Serial.println("[COMP] Left black start pad. Line locked!");
            break;
        }

        // Creep forward slowly
        setMotorASpeed(START_BLIND_SPEED);
        setMotorBSpeed(START_BLIND_SPEED);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    g_compStage = STAGE_LINE_TO_WALLS;
}

// --- STAGE 1: Line Following to Walls ---
static void stageLineFollowToWalls() {
    if (isEmergencyStopTriggered()) return;
    Serial.println("[COMP] STAGE: LINE_TO_WALLS");
    displayFrame("COMPETITION\nFOLLOW TO WALLS");

    initTOFSensors();
    resetLinePID();

    while (true) {
        if (isEmergencyStopTriggered()) return;

        // Follow line normally
        follow_line();

        // Check TOF 60-deg sensors for wall presence ahead
        uint16_t mmL = TOF_INVALID_MM;
        uint16_t mmR = TOF_INVALID_MM;
        bool lOk = readTOFDistance(TOF_LEFT60, mmL);
        bool rOk = readTOFDistance(TOF_RIGHT60, mmR);

        // Entering wall zone: both walls detected within 500 mm
        if (lOk && rOk && mmL < 500 && mmR < 500) {
            Serial.printf("[COMP] Dual walls detected! L60:%u mm, R60:%u mm. Switching to Wall Nav.\n", mmL, mmR);
            brakeMotors();
            vTaskDelay(pdMS_TO_TICKS(100));
            g_compStage = STAGE_WALL_NAVIGATION;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

// --- STAGE 2: Wall Navigation Corridor ---
static void stageWallNavigation() {
    if (isEmergencyStopTriggered()) return;
    Serial.println("[COMP] STAGE: WALL_NAVIGATION");
    displayFrame("COMPETITION\nWALL NAVIGATION");

    // Execute wall following PD routine
    wallFollowPD();

    if (isEmergencyStopTriggered()) return;

    // After exiting walls, re-acquire black line
    Serial.println("[COMP] Wall navigation finished. Re-acquiring line for ramp...");
    resetLinePID();
    g_compStage = STAGE_RAMP_CROSSING;
}

// --- STAGE 3: Ramp Crossing ---
static void stageRampCrossing() {
    if (isEmergencyStopTriggered()) return;
    Serial.println("[COMP] STAGE: RAMP_CROSSING");
    displayFrame("COMPETITION\nRAMP CROSSING");

    unsigned long rampStartMs = millis();
    const unsigned long rampMinDurationMs = 2000;

    while (true) {
        if (isEmergencyStopTriggered()) return;

        follow_line();

        // After minimum ramp time, prepare to detect post-ramp curve
        if (millis() - rampStartMs > rampMinDurationMs) {
            Serial.println("[COMP] Cleared ramp. Transitioning to curve detection...");
            g_compStage = STAGE_POST_RAMP_CURVE;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

// --- STAGE 4: Post-Ramp Curved Line Detection ---
static void stageDetectPostRampCurve() {
    if (isEmergencyStopTriggered()) return;
    Serial.println("[COMP] STAGE: POST_RAMP_CURVE");
    displayFrame("COMPETITION\nCURVED TRACK");

    int highErrorCount = 0;
    bool inCurve = false;
    unsigned long curveExitCheckMs = 0;

    while (true) {
        if (isEmergencyStopTriggered()) return;

        LineSensorState state = readLineSensors();

        // Check if line error magnitude is high (curved track pattern)
        if (fabs(state.position) >= CURVE_ERROR_THRESHOLD) {
            highErrorCount++;
            if (highErrorCount >= CURVE_CONSECUTIVE_SAMPLES && !inCurve) {
                inCurve = true;
                Serial.println("[COMP] Curved location detected! Adapting cornering speed.");
            }
        } else {
            if (highErrorCount > 0) highErrorCount--;
        }

        // While negotiating the curve, adapt motor base speed
        if (inCurve) {
            int originalBase = g_pidConfig.baseSpeed;
            g_pidConfig.baseSpeed = CURVE_BASE_SPEED;
            follow_line();
            g_pidConfig.baseSpeed = originalBase;

            // Check if curve has completed (error centered for stable window)
            if (fabs(state.position) < 0.35f) {
                if (curveExitCheckMs == 0) curveExitCheckMs = millis();
                else if (millis() - curveExitCheckMs > 350) {
                    Serial.println("[COMP] Curved track passed! Moving straight to Color Selector.");
                    g_compStage = STAGE_FORWARD_TO_COLOR;
                    break;
                }
            } else {
                curveExitCheckMs = 0;
            }
        } else {
            follow_line();
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

// --- STAGE 5: Forward to Color Selector (Ignore Right Branch) ---
static void stageForwardToColor() {
    if (isEmergencyStopTriggered()) return;
    Serial.println("[COMP] STAGE: FORWARD_TO_COLOR");
    displayFrame("COMPETITION\nTO COLOR SELECTOR");

    unsigned long forwardStartMs = millis();
    const unsigned long forwardDurationMs = 1200;

    while (millis() - forwardStartMs < forwardDurationMs) {
        if (isEmergencyStopTriggered()) return;
        follow_line();
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    g_compStage = STAGE_CREEPING_COLOR_DETECT;
}

// --- STAGE 6: Creeping Color Detection on Floor ---
static void stageCreepingColorDetect() {
    if (isEmergencyStopTriggered()) return;
    Serial.println("[COMP] STAGE: CREEPING_COLOR_DETECT");
    displayFrame("COMPETITION\nDETECTING COLOR...");

    initColorSensor();
    setColorLight(true); // Illumination LED ON
    vTaskDelay(pdMS_TO_TICKS(50));

    int redConfirmCount = 0;
    int blueConfirmCount = 0;
    unsigned long searchStartMs = millis();
    const unsigned long maxSearchTimeMs = 5000;

    while (millis() - searchStartMs < maxSearchTimeMs) {
        if (isEmergencyStopTriggered()) {
            setColorLight(false);
            return;
        }

        ColorReading cr;
        if (readColorSensor(cr)) {
            Serial.printf("[COLOR DETECT] R:%u G:%u B:%u C:%u -> normRGB(%u,%u,%u)\n",
                          cr.r, cr.g, cr.b, cr.c, cr.nR, cr.nG, cr.nB);

            // Distinct RED
            if (cr.nR > 110 && cr.nR > (uint8_t)(cr.nG * 1.4f) && cr.nR > (uint8_t)(cr.nB * 1.4f)) {
                redConfirmCount++;
                if (redConfirmCount >= COLOR_CONFIRM_SAMPLES) {
                    g_detectedFloorColor = COLOR_TARGET_RED;
                    neopixelWrite(RGB_LED_PIN, 255, 0, 0);
                    Serial.println("[COMP] *** TARGET CONFIRMED: RED ***");
                    displayFrame("COLOR DETECTED:\n[ RED ]");
                    break;
                }
            }
            // Distinct BLUE
            else if (cr.nB > 110 && cr.nB > (uint8_t)(cr.nR * 1.3f) && cr.nB > (uint8_t)(cr.nG * 1.2f)) {
                blueConfirmCount++;
                if (blueConfirmCount >= COLOR_CONFIRM_SAMPLES) {
                    g_detectedFloorColor = COLOR_TARGET_BLUE;
                    neopixelWrite(RGB_LED_PIN, 0, 0, 255);
                    Serial.println("[COMP] *** TARGET CONFIRMED: BLUE ***");
                    displayFrame("COLOR DETECTED:\n[ BLUE ]");
                    break;
                }
            } else {
                redConfirmCount = 0;
                blueConfirmCount = 0;
            }
        }

        setMotorASpeed(COLOR_SEARCH_SPEED);
        setMotorBSpeed(COLOR_SEARCH_SPEED);
        vTaskDelay(pdMS_TO_TICKS(COLOR_SEARCH_STEP_MS));
        stopMotors();
        vTaskDelay(pdMS_TO_TICKS(80));
    }

    if (g_detectedFloorColor == COLOR_UNKNOWN) {
        g_detectedFloorColor = COLOR_TARGET_RED;
        neopixelWrite(RGB_LED_PIN, 255, 0, 0);
        Serial.println("[COMP] Warning: Color search timeout, defaulting to RED.");
    }

    setColorLight(false); // Illumination LED OFF
    vTaskDelay(pdMS_TO_TICKS(300));
    g_compStage = STAGE_TURNAROUND_AND_BRANCH;
}

// --- STAGE 7: Turnaround and Branch Maneuver ---
static void stageTurnaroundAndBranch() {
    if (isEmergencyStopTriggered()) return;
    Serial.println("[COMP] STAGE: TURNAROUND_AND_BRANCH");
    displayFrame("COMPETITION\nTURNAROUND (180)");

    // 1. Rotate 180 degrees
    executeTurn180();

    // 2. Drive forward along line back to junction
    Serial.println("[COMP] Moving forward back to branch junction...");
    executeForwardPulse(POST_COLOR_FORWARD_MS, 70);

    // 3. Turn LEFT into branch
    Serial.println("[COMP] Turning LEFT into assigned branch...");
    executeTurnLeft90();

    // Re-acquire line
    resetLinePID();
    g_compStage = STAGE_LINE_TO_PICK;
}

// --- STAGE 8: Line Follow to Pick Point ---
static void stageLineToPick() {
    if (isEmergencyStopTriggered()) return;
    Serial.println("[COMP] STAGE: LINE_TO_PICK");
    displayFrame("COMPETITION\nTO PICK POINT");

    initArmServos();
    setArmAngle(ARM_RAISE_ANGLE);
    setGripperAngle(GRIPPER_RELEASE);

    while (true) {
        if (isEmergencyStopTriggered()) return;

        follow_line();

        uint16_t mmFront = TOF_INVALID_MM;
        bool frontOk = readTOFDistance(TOF_FRONT_CENTER, mmFront);

        LineSensorState state = g_lastLineState;
        bool crossLine = (state.activeCount >= 4);
        bool objectClose = (frontOk && mmFront < 80);

        if (objectClose || crossLine) {
            Serial.printf("[COMP] Arrived at Pick Point! (TOF: %u mm, crossLine: %d)\n", mmFront, crossLine);
            brakeMotors();
            vTaskDelay(pdMS_TO_TICKS(200));
            g_compStage = STAGE_COLLECT_OBJECT;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

// --- STAGE 9: Collect Object ---
static void stageCollectObject() {
    if (isEmergencyStopTriggered()) return;
    Serial.println("[COMP] STAGE: COLLECT_OBJECT");
    displayFrame("COMPETITION\nCOLLECTING OBJECT");

    brakeMotors();
    vTaskDelay(pdMS_TO_TICKS(200));

    collectObject();

    Serial.println("[COMP] Object securely grasped and lifted! Resuming navigation...");
    resetLinePID();
    g_compStage = STAGE_CARRY_TO_DROP;
}

// --- STAGE 10: Carry Object to Drop Area ---
static void stageCarryToDrop() {
    if (isEmergencyStopTriggered()) return;
    Serial.println("[COMP] STAGE: CARRY_TO_DROP");
    displayFrame("COMPETITION\nCARRYING TO DROP");

    while (true) {
        if (isEmergencyStopTriggered()) return;

        follow_line();

        LineSensorState state = g_lastLineState;
        if (state.activeCount >= 4) {
            Serial.println("[COMP] Arrived at 20x20cm Drop Area (cross-line detected)!");
            brakeMotors();
            vTaskDelay(pdMS_TO_TICKS(200));
            g_compStage = STAGE_PLACE_OBJECT;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

// --- STAGE 11: Place Object ---
static void stagePlaceObject() {
    if (isEmergencyStopTriggered()) return;
    Serial.println("[COMP] STAGE: PLACE_OBJECT");
    displayFrame("COMPETITION\nPLACING OBJECT");

    brakeMotors();
    vTaskDelay(pdMS_TO_TICKS(200));

    placeObject();

    Serial.println("[COMP] Object placed inside Drop Area. Heading to End Point...");
    resetLinePID();
    g_compStage = STAGE_DRIVE_TO_END;
}

// --- STAGE 12: Drive to End Point ---
static void stageDriveToEnd() {
    if (isEmergencyStopTriggered()) return;
    Serial.println("[COMP] STAGE: DRIVE_TO_END");
    displayFrame("COMPETITION\nFINAL RUN TO END");

    while (true) {
        if (isEmergencyStopTriggered()) return;

        follow_line();

        LineSensorState state = g_lastLineState;
        if (state.activeCount >= 4) {
            Serial.println("[COMP] End Point reached! Active braking.");
            brakeMotors();
            vTaskDelay(pdMS_TO_TICKS(300));
            g_compStage = STAGE_COMPLETED;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

// ==============================================================================
// MAIN COMPETITION SEQUENCER
// ==============================================================================

void runCompetitionMasterSequence() {
    Serial.println("\n==============================================");
    Serial.println("  STARTING WAYA 2.0 COMPETITION MASTER RUNNER ");
    Serial.println("==============================================\n");

    resetCompetitionState();

    while (g_compStage != STAGE_COMPLETED) {
        if (isEmergencyStopTriggered()) {
            Serial.println("[COMP] ABORTED by Emergency Stop!");
            brakeMotors();
            g_compStage = STAGE_COMPLETED;
            break;
        }

        switch (g_compStage) {
            case STAGE_START_PAD_EXIT:
                stageExitBlackStartPad();
                break;
            case STAGE_LINE_TO_WALLS:
                stageLineFollowToWalls();
                break;
            case STAGE_WALL_NAVIGATION:
                stageWallNavigation();
                break;
            case STAGE_RAMP_CROSSING:
                stageRampCrossing();
                break;
            case STAGE_POST_RAMP_CURVE:
                stageDetectPostRampCurve();
                break;
            case STAGE_FORWARD_TO_COLOR:
                stageForwardToColor();
                break;
            case STAGE_CREEPING_COLOR_DETECT:
                stageCreepingColorDetect();
                break;
            case STAGE_TURNAROUND_AND_BRANCH:
                stageTurnaroundAndBranch();
                break;
            case STAGE_LINE_TO_PICK:
                stageLineToPick();
                break;
            case STAGE_COLLECT_OBJECT:
                stageCollectObject();
                break;
            case STAGE_CARRY_TO_DROP:
                stageCarryToDrop();
                break;
            case STAGE_PLACE_OBJECT:
                stagePlaceObject();
                break;
            case STAGE_DRIVE_TO_END:
                stageDriveToEnd();
                break;
            case STAGE_COMPLETED:
            default:
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }

    brakeMotors();

    if (s_emergencyStop) {
        Serial.println("\n[COMP] Competition Run HALTED by Emergency Stop.");
        displayFrame("COMPETITION\nEMERGENCY STOP\nHALTED");
    } else {
        Serial.println("\n[COMP] Competition Run Finished! Status: SUCCESS.");
        displayFrame("COMPETITION\nRUN FINISHED!\nSUCCESS");

        // Victory LED sequence: 3 green flashes
        for (int i = 0; i < 3; i++) {
            neopixelWrite(RGB_LED_PIN, 0, 255, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
            neopixelWrite(RGB_LED_PIN, 0, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}
