#include "Display.h"
#include <Adafruit_SH110X.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <stdarg.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1          // share reset with the MCU reset
#define OLED_ADDRESS 0x3C        // default 1.3" SH1106 address
#define DISPLAY_LINES        7   // 64 px / 8 px per text row
#define DISPLAY_LINE_CHARS   21  // 6 px per char at size 1 -> 128/6

static Adafruit_SH1106G s_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
static SemaphoreHandle_t s_displayMutex = NULL;
static char s_lines[DISPLAY_LINES][DISPLAY_LINE_CHARS + 1];
static int s_lineCount = 0;
static bool s_displayReady = false;

static void redrawDisplay() {
    s_display.clearDisplay();
    for (int i = 0; i < s_lineCount; i++) {
        s_display.setCursor(0, i * 8);
        s_display.print(s_lines[i]);
    }
    s_display.display();
}

// Append text into the row buffer with wrap + scroll. NO redraw — callers
// decide when to flush the graphics buffer to the panel.
static void addTextLines(const char *text) {
    // Wrap long lines into the fixed-width buffer rows
    while (text[0] != '\0') {
        int len = 0;
        while (text[len] != '\0' && len < DISPLAY_LINE_CHARS) len++;

        if (s_lineCount == DISPLAY_LINES) {
            for (int i = 0; i < DISPLAY_LINES - 1; i++) {
                strlcpy(s_lines[i], s_lines[i + 1], sizeof(s_lines[i]));
            }
            s_lineCount = DISPLAY_LINES - 1;
        }
        strlcpy(s_lines[s_lineCount], text, DISPLAY_LINE_CHARS + 1);
        s_lineCount++;

        text += len;
        // Skip exactly one space at a wrap boundary (avoids leading gap)
        if (text[0] == ' ') text++;
    }
}

void initDisplay() {
    if (s_displayReady) return;

    if (s_displayMutex == NULL) {
        s_displayMutex = xSemaphoreCreateMutex();
    }

    if (s_displayMutex != NULL) {
        xSemaphoreTake(s_displayMutex, portMAX_DELAY);
    }
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    if (s_display.begin(OLED_ADDRESS, true)) {
        s_displayReady = true;
        s_display.clearDisplay();
        s_display.setTextSize(1);
        s_display.setTextColor(SH110X_WHITE);
        s_display.display();
        Serial.printf("[DISPLAY] SH1106 initialized at 0x%02X (SDA %d, SCL %d)\n",
                      OLED_ADDRESS, PIN_I2C_SDA, PIN_I2C_SCL);
    } else {
        Serial.println("[DISPLAY] SH1106 init FAILED - check wiring/power!");
    }

    if (s_displayMutex != NULL) {
        xSemaphoreGive(s_displayMutex);
    }
}

// Internal single-lock core for the printf-style writers. resetFrame:
// true  -> clear the buffer first, write rows from the TOP, one redraw
// false -> append to the scrolling window, one redraw
static void writePrintfText(const char *fmt, va_list args, bool resetFrame) {
    char buf[96];
    vsnprintf(buf, sizeof(buf), fmt, args);

    if (s_displayMutex != NULL) {
        xSemaphoreTake(s_displayMutex, portMAX_DELAY);
    }
    if (!s_displayReady) {
        if (s_displayMutex != NULL) xSemaphoreGive(s_displayMutex);
        return;
    }

    if (resetFrame) {
        s_lineCount = 0;
        memset(s_lines, 0, sizeof(s_lines));
    }

    // '\n' between segments forces a fresh row (vs plain 21-char wrapping)
    char *p = buf;
    while (p[0] != '\0') {
        char *nl = strchr(p, '\n');
        if (nl != NULL) *nl = '\0';
        if (p[0] != '\0') {
            addTextLines(p);
        } else if (resetFrame) {
            p[0] = '\0';  // keep empty placeholder row so rows stay aligned
            if (s_lineCount == DISPLAY_LINES) {
                for (int i = 0; i < DISPLAY_LINES - 1; i++) {
                    strlcpy(s_lines[i], s_lines[i + 1], sizeof(s_lines[i]));
                }
                s_lineCount = DISPLAY_LINES - 1;
            }
            s_lines[s_lineCount][0] = '\0';
            s_lineCount++;
        }
        if (nl == NULL) break;
        p = nl + 1;
    }

    redrawDisplay();
    if (s_displayMutex != NULL) xSemaphoreGive(s_displayMutex);
}

void displayPrint(const char *text) {
    if (!s_displayReady || text == NULL) return;

    if (s_displayMutex != NULL) {
        xSemaphoreTake(s_displayMutex, portMAX_DELAY);
    }

    // One explicit empty row so multi-line frames keep rows they were given
    if (s_lines[0][0] == '\0' && s_lineCount == 0) {
        s_lineCount = 1;
        s_lines[0][0] = '\0';
    }

    addTextLines(text);
    redrawDisplay();

    if (s_displayMutex != NULL) {
        xSemaphoreGive(s_displayMutex);
    }
}

void displayPrint(const String &text) {
    displayPrint(text.c_str());
}

void displayPrintf(const char *fmt, ...) {
    if (fmt == NULL) return;
    va_list args;
    va_start(args, fmt);
    writePrintfText(fmt, args, false);
    va_end(args);
}

void displayFrame(const char *fmt, ...) {
    if (fmt == NULL) return;
    va_list args;
    va_start(args, fmt);
    writePrintfText(fmt, args, true);
    va_end(args);
}
