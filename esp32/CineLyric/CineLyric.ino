#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "config.h"
#include "display.h"
#include "animations.h"
#include "websocket_client.h"
#include "face.h"
#include "visuals.h"
#include "ble_manager.h"

// Frame timing
unsigned long lastFrameTime = 0;
const int targetFPS = 30;
const int frameDelay = 1000 / targetFPS;

DisplayMode currentMode = MODE_LYRICS;

// Touch sensor debouncing
bool lastDebouncedState = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

unsigned long touchStartTime = 0;
unsigned long lastTapReleaseTime = 0;
int touchCount = 0;
bool isLongPressHandled = false;
bool isAsleep = false;

// Persistent Wi-Fi storage
static Preferences preferences;
static bool wifiConnected = false;
static unsigned long lastWifiRetryTime = 0;
static const unsigned long wifiRetryInterval = 30000; // Retry Wi-Fi every 30s if disconnected
static bool wsInitialized = false;

bool attemptWifiConnection(const char* ssid, const char* password, unsigned long timeoutMs = 8000) {
    if (!ssid || strlen(ssid) == 0) {
        Serial.println("[WiFi] No SSID configured");
        return false;
    }

    Serial.printf("[WiFi] Connecting to: %s\n", ssid);
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
        delay(250);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WiFi] Connected! IP: " + WiFi.localIP().toString());
        wifiConnected = true;
        bleSetWifiStatus("CONNECTED");
        return true;
    } else {
        Serial.println("\n[WiFi] Connection failed or timed out.");
        wifiConnected = false;
        bleSetWifiStatus("FAILED");
        return false;
    }
}

void handleSingleTap() {
    if (bleHasActiveNotification()) {
        bleDismissNotification();
        return;
    }
    if (currentMode == MODE_LYRICS) {
        cycleAnimation();
    } else if (currentMode == MODE_VISUALS) {
        cycleVisuals();
    } else {
        if (isAsleep) {
            playFaceAnimation(ANIM_WAKE);
            isAsleep = false;
        } else {
            FaceAnim anims[] = {ANIM_HAPPY, ANIM_CURIOUS, ANIM_THINKING, ANIM_SHOCK, ANIM_LOVE};
            playFaceAnimation(anims[random(0, 5)]);
        }
    }
}

void handleDoubleTap() {
    if (currentMode == MODE_LYRICS) {
        currentMode = MODE_VISUALS;
    } else if (currentMode == MODE_VISUALS) {
        currentMode = MODE_FACE;
        playFaceAnimation(ANIM_WAKE);
    } else {
        if (!isAsleep) {
            playFaceAnimation(ANIM_LAUGH);
        }
    }
}

void handleLongPress() {
    if (currentMode == MODE_LYRICS) {
        currentMode = MODE_VISUALS;
    } else if (currentMode == MODE_VISUALS) {
        currentMode = MODE_FACE;
        playFaceAnimation(ANIM_WAKE);
    } else {
        if (isAsleep) {
            currentMode = MODE_LYRICS;
            isAsleep = false;
        } else {
            playFaceAnimation(ANIM_SLEEP);
            isAsleep = true;
        }
    }
}

void setup() {
    Serial.begin(115200);

    // Initialize Touch Sensor
    pinMode(TOUCH_PIN, INPUT);

    // Initialize Display
    displaySetup();
    displayClear();
    drawCenteredText("CineLyric", 26, u8g2_font_helvB10_te);
    drawCenteredText("Starting DeskBuddy...", 48, u8g2_font_helvB08_tr);
    displayUpdate();

    // Initialize DeskBuddy BLE early so provisioning is available immediately
    bleSetup();

    // Initialize Face
    setupFace();

    // Load Wi-Fi credentials from Preferences NVS (or fall back to config.h / secrets.h)
    preferences.begin("cinelyric", false);
    String activeSSID = preferences.getString("ssid", DEFAULT_WIFI_SSID);
    String activePass = preferences.getString("pass", DEFAULT_WIFI_PASSWORD);

    displayClear();
    drawCenteredText("CineLyric", 26, u8g2_font_helvB10_te);
    drawCenteredText("Connecting WiFi...", 48, u8g2_font_helvB08_tr);
    displayUpdate();

    // Attempt connection with bounded 8-second timeout
    if (activeSSID.length() > 0 && attemptWifiConnection(activeSSID.c_str(), activePass.c_str(), 8000)) {
        displayClear();
        drawCenteredText("WiFi Connected", 32, u8g2_font_helvB10_te);
        displayUpdate();
        delay(800);

        websocketSetup();
        wsInitialized = true;
        currentMode = MODE_LYRICS;
    } else {
        displayClear();
        drawCenteredText("WiFi Offline", 28, u8g2_font_helvB10_te);
        drawCenteredText("BLE Standalone Mode", 48, u8g2_font_6x12_tf);
        displayUpdate();
        delay(1200);

        // Graceful standalone face mode
        currentMode = MODE_FACE;
        isAsleep = false;
        playFaceAnimation(ANIM_WAKE);
    }

    AnimPacket initPkt = {"Waiting for", "Spotify...", "fade", "", "", 120.0, 0.5, 5000, 0.5, 0.5, "Calm", "Verse", "None", "Medium", 64, 32, false, false, "None"};
    setAnimationState(initPkt);
}

void loop() {
    // 1. Process BLE Events
    bleLoop();

    // 2. Handle BLE Wi-Fi Provisioning Updates
    if (bleHasWifiConfigUpdate()) {
        String newSSID, newPass;
        if (bleGetWifiConfig(newSSID, newPass)) {
            Serial.printf("[Provisioning] Applying new Wi-Fi credentials: %s\n", newSSID.c_str());
            bleSetWifiStatus("CONNECTING");

            displayClear();
            drawCenteredText("WiFi Setup", 26, u8g2_font_helvB10_te);
            drawCenteredText("Connecting...", 48, u8g2_font_helvB08_tr);
            displayUpdate();

            if (attemptWifiConnection(newSSID.c_str(), newPass.c_str(), 10000)) {
                // Save to persistent flash
                preferences.putString("ssid", newSSID);
                preferences.putString("pass", newPass);

                displayClear();
                drawCenteredText("WiFi Connected!", 32, u8g2_font_helvB10_te);
                displayUpdate();
                delay(1000);

                if (!wsInitialized) {
                    websocketSetup();
                    wsInitialized = true;
                }
                currentMode = MODE_LYRICS;
            } else {
                displayClear();
                drawCenteredText("WiFi Failed", 28, u8g2_font_helvB10_te);
                drawCenteredText("Check credentials", 48, u8g2_font_6x12_tf);
                displayUpdate();
                delay(1500);
            }
        }
    }

    // 3. Handle Pending Face Animation Requests from BLE safely on Core 1
    if (bleHasPendingFaceAnim()) {
        FaceAnim anim = bleConsumePendingFaceAnim();
        currentMode = MODE_FACE;
        isAsleep = (anim == ANIM_SLEEP);
        playFaceAnimation(anim);
    }

    // 4. Handle incoming BLE Media Actions
    MediaAction mediaAction = bleConsumeMediaAction();
    if (mediaAction != MEDIA_NONE) {
        if (mediaAction == MEDIA_TOGGLE) {
            Serial.println("[DeskBuddy] BLE Media Play/Pause triggered");
        } else if (mediaAction == MEDIA_NEXT) {
            Serial.println("[DeskBuddy] BLE Media Next triggered");
        } else if (mediaAction == MEDIA_PREVIOUS) {
            Serial.println("[DeskBuddy] BLE Media Previous triggered");
        }
    }

    // 5. Handle WebSocket events if Wi-Fi is connected
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        if (!wsInitialized) {
            websocketSetup();
            wsInitialized = true;
        }
        websocketLoop();
    } else {
        wifiConnected = false;
        // Non-blocking periodic reconnect attempt
        if (millis() - lastWifiRetryTime > wifiRetryInterval) {
            lastWifiRetryTime = millis();
            String savedSSID = preferences.getString("ssid", DEFAULT_WIFI_SSID);
            if (savedSSID.length() > 0) {
                Serial.println("[WiFi] Reconnecting in background...");
                WiFi.reconnect();
            }
        }
    }

    // 6. Handle Touch Sensor
    bool rawTouchState = (digitalRead(TOUCH_PIN) == HIGH);

    if (rawTouchState != lastDebouncedState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
        bool currentTouchState = rawTouchState;

        if (currentTouchState && !isLongPressHandled) {
            if (touchStartTime == 0) {
                touchStartTime = millis();
            } else if (millis() - touchStartTime > 600) {
                handleLongPress();
                isLongPressHandled = true;
                touchCount = 0;
            }
        } else if (!currentTouchState) {
            if (touchStartTime != 0) {
                unsigned long duration = millis() - touchStartTime;
                if (!isLongPressHandled && duration < 600) {
                    touchCount++;
                    lastTapReleaseTime = millis();
                }
                touchStartTime = 0;
                isLongPressHandled = false;
            }
        }
    }
    lastDebouncedState = rawTouchState;

    // Check tap sequence
    if (touchCount > 0 && touchStartTime == 0) {
        if (millis() - lastTapReleaseTime > 400) {
            if (touchCount == 1) {
                handleSingleTap();
            } else if (touchCount >= 2) {
                handleDoubleTap();
            }
            touchCount = 0;
        }
    }

    // 7. Frame pacing & Rendering
    unsigned long currentMillis = millis();
    if (currentMillis - lastFrameTime >= frameDelay) {
        lastFrameTime = currentMillis;

        // Highest priority: Active notification popup
        if (bleHasActiveNotification()) {
            NotificationData notif = bleGetNotification();
            displayClear();
            drawNotificationPopup(notif.app.c_str(), notif.title.c_str(), notif.message.c_str());
            displayUpdate();
        } else if (currentMode == MODE_LYRICS) {
            updateAnimation();
        } else if (currentMode == MODE_VISUALS) {
            updateVisuals();
        } else {
            updateFace();
        }
    }
}
