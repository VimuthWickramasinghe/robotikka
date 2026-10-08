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
//   1. On boot, buttonTaskInit() attaches an ISR to the BOOT
//      button (GPIO 0) and creates the task in SUSPENDED state.
//   2. When the user presses the BOOT button at any time (even
//      during maze run), the ISR resumes the task.
//   3. The task cycles through 8 colors on the onboard RGB LED.
//      Each color = one command.
//   4. User presses button during the desired color → LED blinks
//      that color for ~3 seconds.
//   5. User presses button again within 3 s → command confirmed,
//      corresponding flag is set, task suspends itself.
//   6. If no press within 3 s → selection cancelled, resumes
//      cycling through all 8 colors.
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
    {255,   0,   0, "linefollow"},
    {  0, 255,   0, "OTA mode"},
    {  0,   0, 255, "IR calibrate"},
    {255, 255,   0, "command4"},
    {255,   0, 255, "command5"},
    {  0, 255, 255, "command6"},
    {128,   0, 255, "command7"},
    {255, 128,   0, "command8"}
};

// --- ISR flag (set by GPIO interrupt, cleared by task) ---
static volatile bool s_isrButtonPress = false;

// --- Task handle (created suspended, resumed by ISR) ---
static TaskHandle_t s_buttonTaskHandle = NULL;

// --- ISR: set flag and resume button task ---
static void IRAM_ATTR onBootButtonISR() {
    s_isrButtonPress = true;

    if (s_buttonTaskHandle != NULL) {
        xTaskResumeFromISR(s_buttonTaskHandle);
    }
}

// --- Helper: turn RGB LED off ---
static void rgbOff() {
    neopixelWrite(RGB_LED_PIN, 0, 0, 0);
}

// --- Helper: set RGB LED to a color ---
static void rgbSet(uint8_t r, uint8_t g, uint8_t b) {
    neopixelWrite(RGB_LED_PIN, r, g, b);
}

// --- Helper: map color index to RunMode ---
static void setRunMode(uint8_t idx) {
    switch (idx) {
        case 0: // linefollow
            g_runMode = LINE_FOLLOW;
            WiFi.mode(WIFI_OFF); // Disable WiFi for ADC2 unlock
            if (hTaskRobotControl != NULL) vTaskResume(hTaskRobotControl);
            if (hTaskTelemetryOTA != NULL) vTaskResume(hTaskTelemetryOTA);
            break;
        case 1: // OTA mode
            g_runMode = OTA_MODE;
            if (hTaskRobotControl != NULL) vTaskSuspend(hTaskRobotControl);
            if (hTaskTelemetryOTA != NULL) vTaskResume(hTaskTelemetryOTA);
            break;
        case 2: // IR calibrate
            g_runMode = IR_CALIBRATE;
            WiFi.mode(WIFI_OFF); // Disable WiFi for ADC2 unlock
            if (hTaskRobotControl != NULL) vTaskResume(hTaskRobotControl);
            if (hTaskTelemetryOTA != NULL) vTaskResume(hTaskTelemetryOTA);
            break;
        case 3: // command4
        case 4: // command5
        case 5: // command6
        case 6: // command7
        case 7: // command8
            break;
    }
}

// ============================================================
// TASK: buttonLogicTask
// ============================================================

void buttonLogicTask(void *pvParameters) {
    (void)pvParameters;

    Serial.println("[DEBUG] buttonLogicTask created, priority 4, stack 4096");

    ButtonState state       = BTN_IDLE;
    uint8_t     colorIdx    = 0;
    uint8_t     tickCount   = 0;
    uint32_t    lastColorMs = 0;

    // Show "system alive" pulse at startup: quick white flash
    rgbSet(20, 20, 20);
    vTaskDelay(pdMS_TO_TICKS(100));
    rgbOff();

    while (true) {
        // Poll the ISR flag (safe to read volatile from task)
        bool pressed = s_isrButtonPress;
        if (pressed) {
            s_isrButtonPress = false;

            // Simple debounce: ignore if pressed < 250 ms ago
            static uint32_t lastPressMs = 0;
            uint32_t now = millis();
            if (now - lastPressMs < 250) {
                pressed = false;
            } else {
                lastPressMs = now;
            }
        }

        switch (state) {

            // ------------------------------------------------
            // IDLE — task is running but LED is off
            // Button press starts color cycling
            // ------------------------------------------------
            case BTN_IDLE:
                if (pressed) {
                    state       = BTN_COLOR_SELECT;
                    colorIdx    = 0;
                    tickCount   = 0;
                    lastColorMs = millis();
                    rgbSet(COLORS[colorIdx].r, COLORS[colorIdx].g, COLORS[colorIdx].b);
                    Serial.println("[BTN] Color select started");
                }
                break;

            // ------------------------------------------------
            // COLOR_SELECT — cycle through 8 colors
            // Button press selects current color → confirm
            // ------------------------------------------------
            case BTN_COLOR_SELECT:
                // Advance color every 500 ms
                if (millis() - lastColorMs >= 500) {
                    lastColorMs = millis();
                    colorIdx = (colorIdx + 1) % NUM_COLORS;
                    rgbSet(COLORS[colorIdx].r, COLORS[colorIdx].g, COLORS[colorIdx].b);
                }

                if (pressed) {
                    state     = BTN_CONFIRM_BLINK;
                    tickCount = 0;
                    Serial.printf("[BTN] Selected: %s — confirm?\n", COLORS[colorIdx].name);
                }
                break;

            // ------------------------------------------------
            // CONFIRM_BLINK — blink chosen color for 3 s
            // Button press → confirmed (set flag, go IDLE)
            // Timeout     → cancelled (resume COLOR_SELECT)
            // ------------------------------------------------
            case BTN_CONFIRM_BLINK:
                // Blink: ON for 200 ms, then OFF next tick (total ~400 ms cycle)
                tickCount++;
                if (tickCount % 2 == 1) {
                    rgbSet(COLORS[colorIdx].r, COLORS[colorIdx].g, COLORS[colorIdx].b);
                } else {
                    rgbOff();
                }

                if (pressed) {
                    // Confirmed
                    setRunMode(colorIdx);
                    rgbOff();
                    state = BTN_IDLE;
                    Serial.printf("[BTN] CONFIRMED: %s\n", COLORS[colorIdx].name);
                } else if (tickCount >= 15) {
                    // Timeout (~3 s): no confirmation
                    rgbOff();
                    state       = BTN_COLOR_SELECT;
                    tickCount   = 0;
                    lastColorMs = millis();
                    rgbSet(COLORS[colorIdx].r, COLORS[colorIdx].g, COLORS[colorIdx].b);
                    Serial.println("[BTN] Confirm timeout, resuming cycle");
                }
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(200));  // 5 Hz
    }
}

// ============================================================
// buttonTaskInit — call from setup() AFTER createTasks()
// ============================================================
//
// 1. Configure BOOT button GPIO with internal pull-up
// 2. Attach falling-edge ISR
// 3. Create the button task in SUSPENDED state
//
// The task only runs when the ISR calls xTaskResumeFromISR().
// This means the button task uses ZERO CPU until the user
// actually presses the boot button.
// ============================================================

void buttonTaskInit() {
    // Configure boot button pin
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

    // Attach interrupt on falling edge (button press)
    attachInterrupt(digitalPinToInterrupt(BOOT_BUTTON_PIN),
                    onBootButtonISR, FALLING);

    // Create the task SUSPENDED — won't run until resumed by ISR
    BaseType_t result = xTaskCreatePinnedToCore(
        buttonLogicTask,
        "buttonLogic",
        4096,
        NULL,
        4,              // priority 4
        &s_buttonTaskHandle,
        0               // Core 0
    );

    if (result != pdPASS || s_buttonTaskHandle == NULL) {
        Serial.println("[ERR] Failed to create buttonLogicTask!");
        return;
    }

    vTaskSuspend(s_buttonTaskHandle);

    Serial.println("[BTN] Boot button ISR attached, task suspended (idle until press)");
    Serial.flush();
}
