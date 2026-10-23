#pragma once

#include "LineSensor.h"

bool initIRDisplay();
void queueIRDisplayUpdate(const LineSensorState &state, bool calibrationMode);
void serviceIRDisplay();
