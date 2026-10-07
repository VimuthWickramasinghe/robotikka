// Copyright 2024 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <Arduino.h> I have uploaded the JSON file that it was exported from EasyEDA. 
#include <WiFi.h>
#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>

// Wi-Fi credentials for OTA uploads
const char *ssid = "Vimuth_hs";
const char *password = "22345678";

// Pin configuration (adjust to your EasyEDA schematic pinout)
const int BUTTON_PIN = 0;       // Onboard BOOT / push button (GPIO 0 on most ESP32-S3 boards)
const int STATUS_LED_PIN = 2;   // Onboard indicator LED (adjust to your schematic)

// Button triple-press detection state
int buttonPressCount = 0;
unsigned long firstPressTime = 0;
const unsigned long MULTI_CLICK_WINDOW = 1500; // 1.5s window to complete 3 clicks
int lastButtonReading = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// OTA state
bool otaEnabled = false;
uint32_t last_ota_time = 0;

void setupOTA() {
  Serial.println("\n[OTA] Triple-click detected! Activating Wi-Fi & ArduinoOTA...");
  digitalWrite(STATUS_LED_PIN, HIGH);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("[OTA] Connecting to Wi-Fi");
  unsigned long startAttemptTime = millis();
  
  // Timeout after 10 seconds if network is unreachable
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN)); // Flash LED while connecting
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[OTA] Wi-Fi Connected!");
    Serial.print("[OTA] IP Address: ");
    Serial.println(WiFi.localIP());

    ArduinoOTA.setHostname("robotikka-s3");

    ArduinoOTA
      .onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.println("[OTA] Start updating " + type);
      })
      .onEnd([]() {
        Serial.println("\n[OTA] Upload Complete. Rebooting...");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        if (millis() - last_ota_time > 500) {
          Serial.printf("[OTA] Progress: %u%%\n", (progress / (total / 100)));
          last_ota_time = millis();
        }
      })
      .onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
      });

    ArduinoOTA.begin();
    otaEnabled = true;
    digitalWrite(STATUS_LED_PIN, HIGH); // Steady ON when OTA is ready
    Serial.println("[OTA] Ready for wireless firmware upload from Arduino IDE / PlatformIO.");
  } else {
    Serial.println("\n[OTA] Wi-Fi connection timed out. Returning to offline mode.");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    digitalWrite(STATUS_LED_PIN, LOW);
  }
}

void checkTripleClick() {
  int reading = digitalRead(BUTTON_PIN);

  // Debounce check
  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    static int stableState = HIGH;
    if (reading != stableState) {
      stableState = reading;
      // Active LOW (pressed)
      if (stableState == LOW) {
        unsigned long now = millis();
        if (buttonPressCount == 0) {
          firstPressTime = now;
          buttonPressCount = 1;
        } else {
          buttonPressCount++;
        }
        Serial.printf("[BUTTON] Click detected (%d/3)\n", buttonPressCount);

        if (buttonPressCount >= 3) {
          buttonPressCount = 0;
          if (!otaEnabled) {
            setupOTA();
          } else {
            Serial.println("[OTA] OTA is already running.");
          }
        }
      }
    }
  }

  lastButtonReading = reading;

  // Reset count if time window expires before reaching 3 presses
  if (buttonPressCount > 0 && (millis() - firstPressTime > MULTI_CLICK_WINDOW)) {
    Serial.println("[BUTTON] Click timeout. Resetting count.");
    buttonPressCount = 0;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- Robotikka System Starting ---");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  // In accordance with competition rules (Section 4.d / 8.a.vi):
  // All wireless communication MUST be terminated before placing on arena.
  // Wi-Fi starts OFF. Only activates when user presses the button 3 times.
  WiFi.mode(WIFI_OFF);
  Serial.println("[System] Wireless OFF. Press onboard button 3 times within 1.5s to enable OTA.");
}

void loop() {
  checkTripleClick();

  if (otaEnabled) {
    ArduinoOTA.handle();
  }

  // --- Normal robot operation (Line following, TOF wall following, arm servos) runs here ---
}

