#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "display.h"
#include "animations.h"
#include "websocket_client.h"

unsigned long lastFrameTime = 0;
const int targetFPS = 30;
const int frameDelay = 1000 / targetFPS;

void setup() {
    Serial.begin(115200);
    
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
    setAnimationState("Waiting for", "Spotify...", "fade", "", "", 120.0, 0.5, 5000);
}

void loop() {
    // Handle WebSocket events
    websocketLoop();
    
    // Frame pacing
    unsigned long currentMillis = millis();
    if (currentMillis - lastFrameTime >= frameDelay) {
        lastFrameTime = currentMillis;
        updateAnimation();
    }
}
