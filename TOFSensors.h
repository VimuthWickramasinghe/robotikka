#pragma once
#include <Arduino.h>
#include "Config.h"

// ==============================================================================
// TOF sensor panel (H5) - 5x VL53L0X, CONTINUOUS ranging mode only
// ==============================================================================
// Physical placement & I2C address assignment (re-addressed at boot to avoid
// the factory-default 0x29 address clash on the shared bus):
//
//   front_center : XSHUT1 (PIN_TOF_XSHUT1, GPIO13) -> 0x30
//   right60      : XSHUT2 (PIN_TOF_XSHUT2, GPIO14) -> 0x31  (right side, ~60deg)
//   front_left   : XSHUT3 (PIN_TOF_XSHUT3, GPIO10) -> 0x32  (left, ~60deg)
//   left60       : XSHUT4 (PIN_TOF_XSHUT4, GPIO47) -> 0x33  (left side, ~60deg)
//                  NOTE: XSHUT4 moved off GPIO48 to keep it clear of the
//                  shared WS2812 RGB LED net (use of the RGB LED used to
//                  reset the left60 sensor).
//   front_right  : NO XSHUT wired                  -> 0x34  (it is the only
//                  device alive at factory 0x29 while the other four are held
//                  in reset, so it is re-addressed FIRST in the init sequence)
//
// Ranging config: default sense config, continuous period 50 ms
// (matches the hardware-tested reference sketch).
//
// PANEL MOUNT FLIP: the panel is now mounted UPSIDE DOWN on the robot, so
// the physical sensors mirror horizontally: what was labelled right60/
// front_right is physically on the LEFT now and vice versa; front_center
// is unchanged. Set TOF_PANEL_UPSIDE_DOWN to 0 to disable the remap.
// ==============================================================================

// 1 = panel mounted upside down (logical reads mirror left/right and
// front-left/right); 0 = original upright mounting
#define TOF_PANEL_UPSIDE_DOWN 1

enum TofSensor : uint8_t {
    TOF_FRONT_CENTER = 0,  // straight ahead
    TOF_RIGHT60,           // right side mounted, ~60deg off axis
    TOF_FRONT_LEFT,        // left-forward on the panel
    TOF_LEFT60,            // left side mounted, ~60deg off axis
    TOF_FRONT_RIGHT,       // right-forward, no XSHUT control
    TOF_COUNT
};

inline const uint8_t TOF_I2C_ADDR[TOF_COUNT] = {0x30, 0x31, 0x32, 0x33, 0x34};
inline const char *TOF_NAME[TOF_COUNT] = {"FC", "R60", "FL", "L60", "FR"};

// Sentinel stored in mm[] for an out-of-range / error reading
#define TOF_INVALID_MM 0xFFFF

// Wire.begin(PIN_I2C_SDA=11, PIN_I2C_SCL=12) once, then the boot-time reset
// sequence: hold all 4 XSHUT-sensors low, re-address front_right (no XSHUT)
// to 0x34 first, then bring each XSHUT sensor up one at a time with its
// unique address and start continuous ranging. Idempotent (safe on re-entry).
void initTOFSensors();

// Poll all 5 sensors in continuous mode. mm[i] gets the distance in mm,
// or TOF_INVALID_MM when the reading is out of range / error status.
void readTOFContinuous(uint16_t mm[TOF_COUNT]);

// Fresh single-sensor read for tight PD loops (~20 ms). Returns true only
// when the continuous-mode driver completed a range since the last read;
// mm gets the raw distance (possibly far) or TOF_INVALID_MM on API error.
// Unlike readTOFContinuous it does NOT fold out-of-range values into the
// sentinel, so gap/exit logic can compare against its own threshold.
bool readTOFDistance(int idx, uint16_t &mm);
