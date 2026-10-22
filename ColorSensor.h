#pragma once
#include <Arduino.h>
#include "Config.h"

// Period between samples: 600 ms integration time + small margin
#define COLOR_SAMPLE_PERIOD_MS 620

struct ColorReading {
    uint16_t r, g, b, c;   // Raw 16-bit channel values + clear
    uint16_t colorTemp;    // Correlated color temperature (Kelvin)
    uint16_t lux;
    uint8_t  nR, nG, nB;   // Normalized 0-255 RGB scale
};

// Wire.begin(PIN_I2C_SDA=11, PIN_I2C_SCL=12) + tcs.begin() (idempotent)
void initColorSensor();

// True if tcs.begin() succeeded on last init attempt
bool colorSensorFound();

// Single raw read (illumination LED must already be ON; it stays ON for the
// whole session — see setColorLight). Normalizes + computes temp/lux.
// Returns true on a valid reading (c > 0).
bool readColorSensor(ColorReading &out);

// Light the onboard RGB LED (GPIO 48) with exactly ONE pure color
// (red, green or blue) — the channel with the majority value wins.
void applyColorToLed(const ColorReading &col);

// Control the TCS34725 illumination LED (GPIO 21) so readings
// don't depend on ambient light. Keep ON for the whole session.
void setColorLight(bool on);
