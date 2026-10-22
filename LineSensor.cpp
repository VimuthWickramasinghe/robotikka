#include "LineSensor.h"

void initLineSensors() {
  // ADC1 is used for GPIO1-8 (IR sensors) on ESP32
  // No special ADC init needed - analogRead() uses ADC1 by default
}

void calibrateLineSensors() {
  // Placeholder
}

LineSensorState readLineSensors() {
  LineSensorState state;
  for (int i = 0; i < 8; i++) {
    state.raw[i] = analogRead(IR_PINS[i]);
  }
  return state;
}

void printLineSensorValues() {
  LineSensorState state = readLineSensors();
  for (int i = 0; i < 8; i++) {
    Serial.print(state.raw[i]);
    if (i < 7) {
      Serial.print(" ");
    }
  }
  Serial.println();
}


