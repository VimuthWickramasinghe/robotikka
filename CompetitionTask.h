#pragma once
#include <Arduino.h>
#include "Config.h"

// ==============================================================================
// WAYA 2.0 COMPETITION STAGES
// ==============================================================================

enum CompetitionStage : uint8_t {
    STAGE_START_PAD_EXIT,          // 1. Blind forward pulse off black start pad
    STAGE_LINE_TO_WALLS,           // 2. High-speed line follow up to dual walls
    STAGE_WALL_NAVIGATION,         // 3. TOF PD centering between the 10cm black walls
    STAGE_RAMP_CROSSING,           // 4. Follow continuous line up 15-deg slope and descent
    STAGE_POST_RAMP_CURVE,         // 5. Detect wavy S-curve pattern from IR array & adapt speed
    STAGE_FORWARD_TO_COLOR,        // 6. Pass junction straight forward to Color Selector
    STAGE_CREEPING_COLOR_DETECT,   // 7. Crawl forward reading TCS34725 until Red/Blue found
    STAGE_TURNAROUND_AND_BRANCH,   // 8. Spin 180 degrees, drive forward, turn left to branch
    STAGE_LINE_TO_PICK,            // 9. Follow branch line to the Pick Point
    STAGE_COLLECT_OBJECT,          // 10. Stop at Pick Point, grip & lift 5x5x5cm cube
    STAGE_CARRY_TO_DROP,           // 11. Follow line carrying cube to 20x20cm Drop Area
    STAGE_PLACE_OBJECT,            // 12. Lower arm and release cube into Drop Area
    STAGE_DRIVE_TO_END,            // 13. Drive to End Point marked area and active brake
    STAGE_COMPLETED                // 14. Finished run (celebrate / idle)
};

// Target color detected on floor
enum DetectedColor : uint8_t {
    COLOR_UNKNOWN = 0,
    COLOR_TARGET_RED,
    COLOR_TARGET_BLUE
};

// Global stage tracking
extern volatile CompetitionStage g_compStage;
extern volatile DetectedColor g_detectedFloorColor;

// Returns human-readable stage name for serial/OLED telemetry
const char* getStageName(CompetitionStage stage);

// Main autonomous runner (executes on Core 1 in TaskRobotControl)
void runCompetitionMasterSequence();

// Reset state between runs
void resetCompetitionState();

// --- Dedicated Start & Instant Emergency Stop (GPIO 14) ---
void initCompetitionButton();
bool isEmergencyStopTriggered();
void triggerEmergencyStop();

// Helper maneuver primitives
void executeTurn180();
void executeTurnLeft90();
void executeTurnRight90();
void executeForwardPulse(int ms, int pwm);
