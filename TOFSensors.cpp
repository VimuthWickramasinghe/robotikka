#include "TOFSensors.h"
#include <Wire.h>
#include "Adafruit_VL53L0X.h"

// One driver instance per physical sensor (index order == enum TofSensor)
static Adafruit_VL53L0X tofLox[TOF_COUNT];
static bool s_tofInit = false;

// XSHUT control lines, index order matches enum TofSensor. front_right has
// no XSHUT wired (-1). left60 uses GPIO47 (moved off the WS2812 net).
static const int TOF_XSHUT_PIN[TOF_COUNT] = {
    PIN_TOF_XSHUT1,  // front_center -> GPIO13
    PIN_TOF_XSHUT2,  // right60      -> GPIO14
    PIN_TOF_XSHUT3,  // front_left   -> GPIO10
    PIN_TOF_XSHUT4,  // left60       -> GPIO47
    -1               // front_right  -> none wired
};

// Proven-enabled sequence (mirrors the tested sketch): the library's
// begin(addr) does DataInit at factory 0x29 then setAddress(addr) internally
// (Adafruit v1.2.4); the no-XSHUT sensor is re-addressed first, then each
// XSHUT sensor is brought up one at a time with its unique address.
// Default sense config, continuous period 50 ms (startRangeContinuous()).

// Panel-mount remap: logical index -> physical driver index. Upside-down
// mounting mirrors left/right 60deg and front-left/right; front_center is
// unaffected (see TOF_PANEL_UPSIDE_DOWN in TOFSensors.h).
static int physicalIndex(int logicalIdx) {
#if TOF_PANEL_UPSIDE_DOWN
    switch (logicalIdx) {
        case TOF_RIGHT60:     return TOF_LEFT60;      // physical left is right60
        case TOF_LEFT60:      return TOF_RIGHT60;     // physical right is left60
        case TOF_FRONT_LEFT:  return TOF_FRONT_RIGHT; // forward diagonal mirror
        case TOF_FRONT_RIGHT: return TOF_FRONT_LEFT;
        default:              return logicalIdx;      // front_center unchanged
    }
#else
    return logicalIdx;
#endif
}

void initTOFSensors() {
    if (s_tofInit) return;
    s_tofInit = true;

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);  // shared I2C bus (idempotent)

    // Hold every resettable sensor in reset -> only front_right is alive
    for (int i = 0; i < TOF_COUNT; i++) {
        if (TOF_XSHUT_PIN[i] < 0) continue;
        pinMode(TOF_XSHUT_PIN[i], OUTPUT);
        digitalWrite(TOF_XSHUT_PIN[i], LOW);
    }
    delay(10);

    // --- front_right FIRST: the only device at factory 0x29; begin() at
    // 0x29, then re-address it to 0x34 before any other sensor wakes up ---
    if (tofLox[TOF_FRONT_RIGHT].begin(0x29)) {
        tofLox[TOF_FRONT_RIGHT].setAddress(TOF_I2C_ADDR[TOF_FRONT_RIGHT]);
        delay(10);
        tofLox[TOF_FRONT_RIGHT].startRangeContinuous();
        Serial.printf("[TOF] %-3s (no XSHUT)  -> 0x%02X OK [CONT]\n",
                      TOF_NAME[TOF_FRONT_RIGHT], TOF_I2C_ADDR[TOF_FRONT_RIGHT]);
    } else {
        Serial.printf("[TOF] %-3s (no XSHUT)  -> 0x29 FAIL - check wiring\n",
                      TOF_NAME[TOF_FRONT_RIGHT]);
    }

    // --- Then walk the XSHUT sensors: raise one reset line at a time and
    // begin() at its unique address while all siblings are still held low ---
    for (int i = 0; i < TOF_COUNT; i++) {
        if (TOF_XSHUT_PIN[i] < 0) continue;
        digitalWrite(TOF_XSHUT_PIN[i], HIGH);
        delay(10);

        if (tofLox[i].begin(TOF_I2C_ADDR[i])) {
            tofLox[i].startRangeContinuous();
            Serial.printf("[TOF] %-3s (XSHUT GPIO%-2d) -> 0x%02X OK [CONT]\n",
                          TOF_NAME[i], TOF_XSHUT_PIN[i], TOF_I2C_ADDR[i]);
        } else {
            Serial.printf("[TOF] %-3s (GPIO%-2d)   -> 0x%02X FAIL\n",
                          TOF_NAME[i], TOF_XSHUT_PIN[i], TOF_I2C_ADDR[i]);
        }
    }
}

void readTOFContinuous(uint16_t mm[TOF_COUNT]) {
    for (int i = 0; i < TOF_COUNT; i++) {
        int phys = physicalIndex(i);
        if (tofLox[phys].isRangeComplete()) {
            uint16_t dist = tofLox[phys].readRange();
            // readRange() returns 0xFFFF on API error; anything >= 8190
            // means out of range / error status in continuous mode
            mm[i] = (dist >= 8190) ? TOF_INVALID_MM : dist;
        } else {
            mm[i] = TOF_INVALID_MM;  // not yet complete -> "---mm"
        }
    }
}

// For tight PD loops: exactly one isRangeComplete()/readRange() round trip.
// Returns true only on a fresh completed range this poll so callers can
// distinguish "new data" from "stale"; out-of-range passes through so
// gap logic can compare the raw value against its own threshold.
// idx is LOGICAL (robot-frame); physicalIndex() applies the mount flip.

bool readTOFDistance(int idx, uint16_t &mm) {
    mm = TOF_INVALID_MM;
    if (idx < 0 || idx >= TOF_COUNT) return false;
    int phys = physicalIndex(idx);
    if (!tofLox[phys].isRangeComplete()) return false;
    mm = tofLox[phys].readRange();
    return true;
}
