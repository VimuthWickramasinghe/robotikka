#pragma once
#include <Arduino.h>
#include "Config.h"

// Wall-follow PD test using the two 60-deg TOF sensors (left60 / right60).
// Runs only when a 60-deg sensor reads below the in-method threshold (60 cm,
// averaged over 3 readings); exits when BOTH 60-deg sensors read above the
// threshold (gap passed), after a 5 s timeout, or when BOOT is pressed.
void wallFollowPD();
