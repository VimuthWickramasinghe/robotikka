#pragma once
#include <Arduino.h>
#include "Config.h"

// Init the SH1106 1.3" OLED on the shared I2C bus (PIN_I2C_SDA / PIN_I2C_SCL,
// address 0x3C). Safe to call multiple times; Static-mutex guarded I2C access.
void initDisplay();

// Append one text line to the OLED and redraw (7-line scrolling window,
// 21 chars per line at text size 1). Callable from any task after
// initDisplay().
void displayPrint(const char *text);
void displayPrint(const String &text);

// printf-style wrapper, used just like Serial.printf: formats the arguments
// and writes the result through displayPrint. A '\n' inside the format
// starts a new display row instead of merely wrapping mid-line.
void displayPrintf(const char *fmt, ...);

// printf-style FULL-FRAME variant: clears the text buffer first, then writes
// each '\n'-separated segment as a fixed row from the top (rows 0..6, extras
// after a trailing '\n' are dropped) with a single redraw — no scrolling.
// Ideal for status frames refreshed in a loop (PD debug, telemetry...).
void displayFrame(const char *fmt, ...);
