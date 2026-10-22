#include "ColorSensor.h"
#include <Wire.h>
#include "Adafruit_TCS34725.h"

// NOTE: GPIO8/9 are the IR array in this firmware (Config.h), so the
// color sensor uses the shared I2C bus from the schematic (11/12)
static Adafruit_TCS34725 tcs(TCS34725_INTEGRATIONTIME_600MS, TCS34725_GAIN_4X);
static bool s_tcsFound = false;

void initColorSensor() {
    if (s_tcsFound) return;
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    if (tcs.begin()) {
        s_tcsFound = true;
        Serial.println("[COLOR] TCS34725 found on shared I2C bus");
    } else {
        Serial.println("[COLOR] ERROR: TCS34725 not found - check wiring/power!");
    }
}

bool colorSensorFound() {
    return s_tcsFound;
}

bool readColorSensor(ColorReading &out) {
    if (!s_tcsFound) return false;

    // Single read; illumination LED is left untouched (set ON once at
    // session start so readings stay under consistent lighting)
    tcs.getRawData(&out.r, &out.g, &out.b, &out.c);

    if (out.c == 0) {
        out.nR = out.nG = out.nB = 0;
        out.colorTemp = 0;
        out.lux = 0;
        return false;
    }

    // Normalize to 0-255 RGB scale (division by c guarded above)
    out.nR = (uint8_t)((uint32_t)out.r * 255 / out.c);
    out.nG = (uint8_t)((uint32_t)out.g * 255 / out.c);
    out.nB = (uint8_t)((uint32_t)out.b * 255 / out.c);

    out.colorTemp = tcs.calculateColorTemperature_dn40(out.r, out.g, out.b, out.c);
    out.lux       = tcs.calculateLux(out.r, out.g, out.b);
    return true;
}

void setColorLight(bool on) {
    static bool s_pinSetup = false;
    if (!s_pinSetup) {
        pinMode(PIN_COLOR_LED, OUTPUT);   // GPIO21: sensor illumination LED
        s_pinSetup = true;
    }
    digitalWrite(PIN_COLOR_LED, on ? HIGH : LOW);
}

void applyColorToLed(const ColorReading &col) {
    // Majority wins: display exactly one pure full color, no mixing
    // (ties resolved red > green > blue)
    uint8_t r = 0, g = 0, b = 0;
    if (col.nR == 0 && col.nG == 0 && col.nB == 0) {
        // nothing above the clear-dominant baseline: keep LED dark
    } else if (col.nR >= col.nG && col.nR >= col.nB) {
        r = 150;
    } else if (col.nG >= col.nB) {
        g = 150;
    } else {
        b = 150;
    }
    neopixelWrite(RGB_LED_PIN, r, g, b);
}
