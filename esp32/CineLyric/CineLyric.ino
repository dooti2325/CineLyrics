#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "display.h"
#include "animations.h"
#include "websocket_client.h"
#include "face.h"
#include "visuals.h"

unsigned long lastFrameTime = 0;
const int targetFPS = 30;
const int frameDelay = 1000 / targetFPS;

enum DisplayMode { MODE_LYRICS, MODE_VISUALS, MODE_FACE };
DisplayMode currentMode = MODE_LYRICS;

bool lastDebouncedState = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

unsigned long touchStartTime = 0;
unsigned long lastTapReleaseTime = 0;
int touchCount = 0;
bool isLongPressHandled = false;
bool isAsleep = false;

void handleSingleTap() {
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
    drawCenteredText("CineLyric", 32, u8g2_font_helvB10_te);
    drawCenteredText("Connecting WiFi...", 50, u8g2_font_helvB08_tr);
    displayUpdate();

    // Connect WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");
    
    displayClear();
    drawCenteredText("WiFi Connected", 32, u8g2_font_helvB10_te);
    displayUpdate();
    delay(1000);

    // Initialize WebSocket
    websocketSetup();
    
    // Initial state
    setupFace();
    AnimPacket initPkt = {"Waiting for", "Spotify...", "fade", "", "", 120.0, 0.5, 5000, 0.5, 0.5, "Calm", "Verse", "None", "Medium", 64, 32, false, false, "None"};
    setAnimationState(initPkt);
}

void loop() {
    // Handle WebSocket events
    websocketLoop();
    
    // Handle Touch Sensor Toggle
    // Using digitalRead for standard capacitive touch modules (like TTP223).
    // If you are using a bare wire directly to an ESP32 touch pin, use: bool currentTouchState = (touchRead(TOUCH_PIN) < TOUCH_THRESHOLD);
    bool rawTouchState = (digitalRead(TOUCH_PIN) == HIGH); 
    
    if (rawTouchState != lastDebouncedState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
        bool currentTouchState = rawTouchState;
        
        if (currentTouchState && !isLongPressHandled) {
            if (touchStartTime == 0) {
                touchStartTime = millis(); // Just started touching
            } else if (millis() - touchStartTime > 600) {
                // Trigger long press once
                handleLongPress();
                isLongPressHandled = true;
                touchCount = 0; // Cancel any taps
            }
        } 
        else if (!currentTouchState) {
            if (touchStartTime != 0) {
                unsigned long duration = millis() - touchStartTime;
                if (!isLongPressHandled && duration < 600) {
                    // It was a short tap
                    touchCount++;
                    lastTapReleaseTime = millis();
                }
                // Reset touch trackers
                touchStartTime = 0;
                isLongPressHandled = false;
            }
        }
    }
    lastDebouncedState = rawTouchState;
    
    // Check if tap sequence is complete (no new taps within 400ms)
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
    
    // Frame pacing
    unsigned long currentMillis = millis();
    if (currentMillis - lastFrameTime >= frameDelay) {
        lastFrameTime = currentMillis;
        if (currentMode == MODE_LYRICS) {
            updateAnimation();
        } else if (currentMode == MODE_VISUALS) {
            updateVisuals();
        } else {
            updateFace();
        }
    }
}
