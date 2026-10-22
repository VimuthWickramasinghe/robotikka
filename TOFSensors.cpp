#include "TOFSensors.h"
#include <Wire.h>
#include "Adafruit_VL53L0X.h"

// One driver instance per physical sensor (index order == enum TofSensor)
static Adafruit_VL53L0X tofLox[TOF_COUNT];
static bool s_tofInit = false;

// XSHUT control lines, index order matches enum TofSensor. front_right has
// no XSHUT wired (-1). left60 shares GPIO48 with the WS2812 RGB LED.
static const int TOF_XSHUT_PIN[TOF_COUNT] = {
    PIN_TOF_XSHUT1,  // front_center -> GPIO13
    PIN_TOF_XSHUT2,  // right60      -> GPIO14
    PIN_TOF_XSHUT3,  // front_left   -> GPIO10
    PIN_TOF_XSHUT4,  // left60       -> GPIO48 (RGB LED net!)
    -1               // front_right  -> none wired
};

// Short-range wall detection: HIGH_SPEED sense config passed through
// begin(); this device model reaches far beyond 2 m but our use is
// wall proximity < 40..50 cm. Continuous inter-measurement = 50 ms.
static const uint16_t TOF_CONT_PERIOD_MS = 50;

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

    // --- front_right FIRST: the only device at factory 0x29, so re-address
    // it to 0x34 before any other sensor comes out of reset ---
    if (tofLox[TOF_FRONT_RIGHT].begin(TOF_I2C_ADDR[TOF_FRONT_RIGHT], false,
                                      &Wire,
                                      Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_SPEED)) {
        tofLox[TOF_FRONT_RIGHT].startRangeContinuous(TOF_CONT_PERIOD_MS);
        Serial.printf("[TOF] %-3s (no XSHUT)  -> 0x%02X OK [CONT]\n",
                      TOF_NAME[TOF_FRONT_RIGHT], TOF_I2C_ADDR[TOF_FRONT_RIGHT]);
    } else {
        Serial.printf("[TOF] %-3s (no XSHUT)  -> 0x%02X FAIL - check wiring\n",
                      TOF_NAME[TOF_FRONT_RIGHT], TOF_I2C_ADDR[TOF_FRONT_RIGHT]);
    }

    // --- Then walk the XSHUT sensors: raise one reset line at a time and
    // begin() at its unique address while all siblings are still held low ---
    for (int i = 0; i < TOF_COUNT; i++) {
        if (TOF_XSHUT_PIN[i] < 0) continue;
        digitalWrite(TOF_XSHUT_PIN[i], HIGH);
        delay(10);

        if (tofLox[i].begin(TOF_I2C_ADDR[i], false, &Wire,
                            Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_SPEED)) {
            tofLox[i].startRangeContinuous(TOF_CONT_PERIOD_MS);
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
        uint16_t range = tofLox[i].readRangeResult();  // latest continuous sample
        uint8_t  st    = tofLox[i].readRangeStatus();

        if (st == 0 && range < 8190) {
            mm[i] = range;
        } else {
            mm[i] = TOF_INVALID_MM;  // signal failure vs valid 0 mm
        }
    }
}
