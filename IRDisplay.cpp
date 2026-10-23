#include "IRDisplay.h"
#include <U8g2lib.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "Config.h"
#include "globals.h"

struct IRDisplayFrame {
  LineSensorState state;
  bool calibrationMode;
};

static U8G2_SSD1306_128X64_NONAME_F_HW_I2C s_display(U8G2_R0, U8X8_PIN_NONE);
static bool s_displayReady = false;
static unsigned long s_lastRefreshMs = 0;
static constexpr unsigned long DISPLAY_REFRESH_MS = 100;
static QueueHandle_t s_pendingFrame = nullptr;

bool initIRDisplay() {
  Wire.beginTransmission(OLED_I2C_ADDRESS);
  if (Wire.endTransmission() != 0) {
    Serial.printf("[OLED] SSD1306 not detected at I2C address 0x%02X\n",
                  OLED_I2C_ADDRESS);
    return false;
  }

  s_display.setI2CAddress(OLED_I2C_ADDRESS << 1);
  s_display.begin();
  s_pendingFrame = xQueueCreate(1, sizeof(IRDisplayFrame));
  if (s_pendingFrame == nullptr) {
    Serial.println("[OLED] ERROR: failed to create display update queue");
    return false;
  }
  s_displayReady = true;
  Serial.printf("[OLED] SSD1306 initialized at I2C address 0x%02X\n",
                OLED_I2C_ADDRESS);
  return true;
}

void queueIRDisplayUpdate(const LineSensorState &state, bool calibrationMode) {
  if (!s_displayReady) {
    return;
  }

  IRDisplayFrame frame = {state, calibrationMode};
  xQueueOverwrite(s_pendingFrame, &frame);
}

void serviceIRDisplay() {
  if (!s_displayReady || millis() - s_lastRefreshMs < DISPLAY_REFRESH_MS) {
    return;
  }

  IRDisplayFrame frame;
  if (xQueueReceive(s_pendingFrame, &frame, 0) != pdTRUE) {
    return;
  }
  s_lastRefreshMs = millis();

  s_display.clearBuffer();
  s_display.setFont(u8g2_font_6x10_tf);
  char text[24];

  if (frame.calibrationMode) {
    s_display.drawStr(0, 9, "CALIB: 192.168.4.1");
    char calibInfo[28];
    snprintf(calibInfo, sizeof(calibInfo), "STP:%d/4 FWD:%s",
             (int)g_calibCurrentStep, g_calibMoveForwardEnabled ? "ON" : "OFF");
    s_display.setFont(u8g2_font_5x8_tf);
    s_display.drawStr(0, 19, calibInfo);
  } else {
    s_display.drawStr(0, 9, "LINE FOLLOW");
    snprintf(text, sizeof(text), "THR %u *=ACTIVE", g_pidConfig.threshold);
    s_display.setFont(u8g2_font_5x8_tf);
    s_display.drawStr(0, 19, text);
  }

  for (int i = 0; i < LINE_SENSOR_COUNT; i++) {
    snprintf(text, sizeof(text), "%d%c:%4u",
             IR_PINS[i], frame.state.isBlack[i] ? '*' : ' ', frame.state.raw[i]);
    const uint8_t x = (i % 2) * 64;
    const uint8_t y = 31 + (i / 2) * 10;
    s_display.drawStr(x, y, text);
  }

  const char *lineStatus = frame.state.lineFound ? "LINE" :
                           frame.state.isDottedGap ? "GAP" : "LOST";
  snprintf(text, sizeof(text), "POS %+.2f %s", frame.state.position, lineStatus);
  s_display.drawStr(0, 62, text);
  s_display.sendBuffer();
}
