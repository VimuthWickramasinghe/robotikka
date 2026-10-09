#include "globals.h"
#include "Config.h"
#include <Arduino.h>
#include <WiFi.h>

// Task handle references for control
extern TaskHandle_t hTaskTelemetryOTA;
extern TaskHandle_t hTaskRobotControl;

// ============================================================
// BUTTON TASK — 8-Color Command Selector
// ============================================================
//
// How it works:
//   1. Runs on Core 0 as a lightweight 50 Hz (20 ms) FreeRTOS task.
//   2. Uses software debouncing on the BOOT button (GPIO 0).
//   3. In IDLE mode, all LEDs remain off.
//   4. Pressing the BOOT button starts color cycling through 8 modes.
//   5. Pressing again during a color enters 3-second CONFIRM blink.
//   6. Pressing within 3 seconds confirms the command, applies the
//      mode, and returns to IDLE.
//   7. If 3 seconds pass without confirmation, cycling resumes.
//   8. Dual visual feedback: drives onboard WS2812 NeoPixel
//      AND toggles STATUS_LED_PIN (GPIO 21).
//
// ============================================================

// --- State Machine ---
enum ButtonState : uint8_t {
    BTN_IDLE,
    BTN_COLOR_SELECT,
    BTN_CONFIRM_BLINK
};

// --- 8 Colors (R, G, B) ---
#define NUM_COLORS 8

struct Color {
    uint8_t r, g, b;
    const char *name;
};

static const Color COLORS[NUM_COLORS] = {
    {255,   0,   0, "Line Follow"},     // Mode 1: Red
    {  0, 255,   0, "OTA Mode"},        // Mode 2: Green
    {  0,   0, 255, "IR Calibrate"},    // Mode 3: Blue
    {255, 255,   0, "Command 4"},       // Mode 4: Yellow
    {255,   0, 255, "Command 5"},       // Mode 5: Magenta
    {  0, 255, 255, "Command 6"},       // Mode 6: Cyan
    {128,   0, 255, "Command 7"},       // Mode 7: Purple
    {255, 128,   0, "Command 8"}        // Mode 8: Orange
};

static TaskHandle_t s_buttonTaskHandle = NULL;

// --- Helper: turn all indicator LEDs off ---
static void ledsOff() {
#if defined(RGB_BUILTIN)
    neopixelWrite(RGB_BUILTIN, 0, 0, 0);
#endif
    neopixelWrite(RGB_LED_PIN, 0, 0, 0);
    digitalWrite(STATUS_LED_PIN, LOW);
}

// --- Helper: set RGB LED and optional status LED ---
static void ledsSet(uint8_t r, uint8_t g, uint8_t b, bool statusLed = true) {
#if defined(RGB_BUILTIN)
    neopixelWrite(RGB_BUILTIN, r, g, b);
#endif
    neopixelWrite(RGB_LED_PIN, r, g, b);
    digitalWrite(STATUS_LED_PIN, statusLed ? HIGH : LOW);
}

// --- Helper: map color index to RunMode ---
static void setRunMode(uint8_t idx) {
    switch (idx) {
        case 0: // Line Follow
            g_runMode = LINE_FOLLOW;
            WiFi.mode(WIFI_OFF); // Disable WiFi for ADC2 unlock
            if (hTaskRobotControl != NULL) vTaskResume(hTaskRobotControl);
            if (hTaskTelemetryOTA != NULL) vTaskResume(hTaskTelemetryOTA);
            Serial.println("[MODE] Line Follow active (WiFi OFF, PID active)");
            break;

        case 1: // OTA mode
            g_runMode = OTA_MODE;
            if (hTaskRobotControl != NULL) vTaskSuspend(hTaskRobotControl);
            if (hTaskTelemetryOTA != NULL) vTaskResume(hTaskTelemetryOTA);
            Serial.println("[MODE] OTA Mode active (WiFi enabled)");
            break;

        case 2: // IR calibrate
            g_runMode = IR_CALIBRATE;
            WiFi.mode(WIFI_OFF); // Disable WiFi for ADC2 unlock
            if (hTaskRobotControl != NULL) vTaskResume(hTaskRobotControl);
            if (hTaskTelemetryOTA != NULL) vTaskResume(hTaskTelemetryOTA);
            Serial.println("[MODE] IR Calibrate active");
            break;

        case 3: // Command 4
        case 4: // Command 5
        case 5: // Command 6
        case 6: // Command 7
        case 7: // Command 8
            Serial.printf("[MODE] %s activated (User custom slot)\n", COLORS[idx].name);
            break;
    }
}

// ============================================================
// TASK: buttonLogicTask
// ============================================================

void buttonLogicTask(void *pvParameters) {
    (void)pvParameters;

    Serial.println("[DEBUG] buttonLogicTask running on Core 0 (50 Hz debounced)");

    // Startup pulse: 150 ms white flash on RGB and status LED to confirm alive
    ledsSet(40, 40, 40, true);
    vTaskDelay(pdMS_TO_TICKS(150));
    ledsOff();

    ButtonState state        = BTN_IDLE;
    uint8_t     colorIdx     = 0;
    uint32_t    lastColorMs  = 0;
    uint32_t    confirmStartMs = 0;
    uint32_t    lastBlinkMs  = 0;
    bool        blinkState   = false;

    // Debounce tracking
    bool     lastRawState    = HIGH; // Boot button is active LOW (unpressed = HIGH)
    bool     debouncedState  = HIGH;
    uint32_t lastDebounceMs  = 0;

    const TickType_t xPeriod = pdMS_TO_TICKS(20); // 20 ms tick (50 Hz)
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (true) {
        uint32_t now = millis();

        // ----------------------------------------------------
        // 1. Debounced Button Edge Detection
        // ----------------------------------------------------
        bool raw = digitalRead(BOOT_BUTTON_PIN);
        bool clicked = false;

        if (raw != lastRawState) {
            lastDebounceMs = now;
            lastRawState = raw;
        }

        if ((now - lastDebounceMs) >= 40) { // 40 ms stable window
            if (raw != debouncedState) {
                debouncedState = raw;
                // Transition to LOW means button was pressed down
                if (debouncedState == LOW) {
                    clicked = true;
                }
            }
        }

        // ----------------------------------------------------
        // 2. State Machine
        // ----------------------------------------------------
        switch (state) {

            // ------------------------------------------------
            // IDLE: LEDs off, waiting for first click
            // ------------------------------------------------
            case BTN_IDLE:
                if (clicked) {
                    state       = BTN_COLOR_SELECT;
                    colorIdx    = 0;
                    lastColorMs = now;
                    ledsSet(COLORS[colorIdx].r, COLORS[colorIdx].g, COLORS[colorIdx].b, true);
                    Serial.printf("\n[BTN] Mode selector started! (1/%d: %s)\n", NUM_COLORS, COLORS[colorIdx].name);
                }
                break;

            // ------------------------------------------------
            // COLOR_SELECT: Cycle through 8 colors every 600 ms
            // ------------------------------------------------
            case BTN_COLOR_SELECT:
                // Advance color every 600 ms
                if (now - lastColorMs >= 600) {
                    lastColorMs = now;
                    colorIdx = (colorIdx + 1) % NUM_COLORS;
                    ledsSet(COLORS[colorIdx].r, COLORS[colorIdx].g, COLORS[colorIdx].b, true);
                    Serial.printf("[BTN] (%d/%d): %s\n", colorIdx + 1, NUM_COLORS, COLORS[colorIdx].name);
                }

                if (clicked) {
                    state          = BTN_CONFIRM_BLINK;
                    confirmStartMs = now;
                    lastBlinkMs    = now;
                    blinkState     = true;
                    ledsSet(COLORS[colorIdx].r, COLORS[colorIdx].g, COLORS[colorIdx].b, true);
                    Serial.printf("[BTN] Selected [%s] — Click again within 3s to CONFIRM!\n", COLORS[colorIdx].name);
                }
                break;

            // ------------------------------------------------
            // CONFIRM_BLINK: Fast blink (150 ms) for up to 3s
            // ------------------------------------------------
            case BTN_CONFIRM_BLINK:
                // Fast blink toggle every 150 ms
                if (now - lastBlinkMs >= 150) {
                    lastBlinkMs = now;
                    blinkState = !blinkState;
                    if (blinkState) {
                        ledsSet(COLORS[colorIdx].r, COLORS[colorIdx].g, COLORS[colorIdx].b, true);
                    } else {
                        ledsOff();
                    }
                }

                if (clicked) {
                    // Confirmed by user click!
                    setRunMode(colorIdx);
                    ledsOff();
                    state = BTN_IDLE;
                    Serial.printf("[BTN] >>> CONFIRMED: %s <<<\n\n", COLORS[colorIdx].name);
                } else if (now - confirmStartMs >= 3000) {
                    // Timed out (3 seconds without confirm click) -> return to cycle
                    state       = BTN_COLOR_SELECT;
                    lastColorMs = now;
                    ledsSet(COLORS[colorIdx].r, COLORS[colorIdx].g, COLORS[colorIdx].b, true);
                    Serial.println("[BTN] Confirmation timed out. Resuming color cycle...");
                }
                break;
        }

        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

// ============================================================
// buttonTaskInit — call from setup() in robotikka.ino
// ============================================================

void buttonTaskInit() {
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);

    BaseType_t result = xTaskCreatePinnedToCore(
        buttonLogicTask,
        "buttonLogic",
        4096,
        NULL,
        2,              // Priority 2
        &s_buttonTaskHandle,
        0               // Core 0
    );

    if (result != pdPASS || s_buttonTaskHandle == NULL) {
        Serial.println("[ERR] Failed to create buttonLogicTask!");
    } else {
        Serial.println("[BTN] Button task initialized and active on Core 0");
    }
}
