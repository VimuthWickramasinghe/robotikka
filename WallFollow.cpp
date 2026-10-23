#include "WallFollow.h"
#include "TOFSensors.h"
#include "MotorDriver.h"
#include "globals.h"
#include "Display.h"
#include <freertos/task.h>

// Motor mapping mirrors follow_line(): Motor A = Right, Motor B = Left.
// error > 0 always means "too close to the LEFT wall" -> steer right.
// The 20 ms loop reads each 60-deg sensor directly (no bulk array copy);
// when no fresh sample is available the last value feeds the PD again.
void wallFollowPD() {
    int threshold = 600;       // 60 cm wall-detect threshold
    int mid = threshold / 2;   // target clearance for single-wall behaviour
    float kp = 0.05f;
    float kd = 0.3f;
    float lastError = 0.0f;
    int baseSpeed = 50;
    int minSpeed = 0;
    int maxSpeed = 100;

    initTOFSensors();

    // Pre-loop wall check: average 3 direct readings per 60-deg sensor
    auto average3 = [](int idx, int &out) {
        uint32_t sum = 0;
        int samples = 0;
        unsigned long start = millis();
        while (samples < 3 && millis() - start < 600) {
            uint16_t mm;
            if (readTOFDistance(idx, mm) && mm != TOF_INVALID_MM) {
                sum += mm;
                samples++;
            }
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        out = (samples == 3) ? (int)(sum / 3) : -1;
        return samples == 3;
    };

    int leftAvg = -1;
    int rightAvg = -1;
    bool leftOk = average3(TOF_LEFT60, leftAvg);
    bool rightOk = average3(TOF_RIGHT60, rightAvg);

    bool wallClose = (leftOk && leftAvg < threshold) || (rightOk && rightAvg < threshold);
    if (!wallClose) {
        Serial.printf("[WALL] Start denied - L60 avg:%d R60 avg:%d (threshold %dmm)\n",
                      leftAvg, rightAvg, threshold);
        return;
    }

    // Wall presence is locked in here, before the loop starts
    bool leftWall = leftOk && leftAvg < threshold;
    bool rightWall = rightOk && rightAvg < threshold;

    Serial.printf("[WALL] Wall follow start (kp %.2f kd %.2f thr %dmm, 5s timeout) Lavg:%d Ravg:%d walls:%s%s\n",
                  kp, kd, threshold,
                  leftWall ? leftAvg : -1, rightWall ? rightAvg : -1,
                  leftWall ? "L" : "", rightWall ? "R" : "");

    float leftVal = leftWall ? (float)leftAvg : 0.0f;
    float rightVal = rightWall ? (float)rightAvg : 0.0f;

    unsigned long loopStartMs = millis();
    const unsigned long timeoutMs = 5000;

    while (true) {
        // Direct per-sensor reads: true = fresh completed range this poll
        uint16_t mmL = TOF_INVALID_MM;
        uint16_t mmR = TOF_INVALID_MM;
        bool lFresh = readTOFDistance(TOF_LEFT60, mmL) && mmL != TOF_INVALID_MM;
        bool rFresh = readTOFDistance(TOF_RIGHT60, mmR) && mmR != TOF_INVALID_MM;

        if (lFresh) leftVal = (float)mmL;
        if (rFresh) rightVal = (float)mmR;

        // Gap exit: BOTH 60-deg side readings must be beyond the threshold
        if (leftWall && rightWall &&
            leftVal > threshold && rightVal > threshold) {
            Serial.println("[WALL] Gap passed (both 60-deg clear) - exit");
            break;
        }

        // Safety timeout: end the run after 5 s regardless of sensors
        if (millis() - loopStartMs >= timeoutMs) {
            Serial.println("[WALL] 5s timeout reached - exit");
            break;
        }

        float error;
        if (leftWall && rightWall) {
            error = rightVal - leftVal;            // left closer -> steer right
        } else if (leftWall) {
            error = (float)mid - leftVal;          // hold mid clearance off left
        } else if (rightWall) {
            error = rightVal - (float)mid;         // hold mid clearance off right
        } else {
            error = 0.0f;                          // walls lost: keep straight
        }

        float correction = kp * error + kd * (error - lastError);
        lastError = error;

        int leftSpeed  = constrain(baseSpeed + (int)correction, minSpeed, maxSpeed);
        int rightSpeed = constrain(baseSpeed - (int)correction, minSpeed, maxSpeed);

        setMotorBSpeed(leftSpeed);   // Motor B = Left
        setMotorASpeed(rightSpeed);  // Motor A = Right

        // OLED refresh is throttled to 100 ms: a full SH1106 frame takes
        // ~20 ms of shared-bus I2C time, which would starve the 50 ms TOF
        // ranges if flushed on every 20 ms control tick
        static unsigned long s_lastDisplayMs = 0;
        if (millis() - s_lastDisplayMs >= 100) {
            s_lastDisplayMs = millis();
            displayFrame("L60:%d R60:%d\nerr:%.2f corr:%.2f\nL:%d R:%d",
                         leftWall ? (int)leftVal : -1,
                         rightWall ? (int)rightVal : -1,
                         (double)error, (double)correction,
                         leftSpeed, rightSpeed);
        }

        vTaskDelay(pdMS_TO_TICKS(20));

        if (consumeButtonPress()) {
            Serial.println("[WALL] Wall follow exited (BOOT pressed)");
            break;
        }
    }

    stopMotors();
}
