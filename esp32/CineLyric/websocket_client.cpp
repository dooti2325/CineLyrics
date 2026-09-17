#include "websocket_client.h"
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "animations.h"
#include "visuals.h"
#include "face.h"
#include "display.h"

static WebSocketsClient webSocket;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            if (length > 0 && payload != NULL) {
                Serial.printf("[WSc] Disconnected! Reason: %.*s (Free Heap: %u bytes)\n", (int)length, (char*)payload, (unsigned int)ESP.getFreeHeap());
            } else {
                Serial.printf("[WSc] Disconnected! (Free Heap: %u bytes)\n", (unsigned int)ESP.getFreeHeap());
            }
            break;
        case WStype_CONNECTED:
            Serial.printf("[WSc] Connected to url: %.*s (Free Heap: %u bytes)\n", (int)length, (char*)payload, (unsigned int)ESP.getFreeHeap());
            break;
        case WStype_TEXT:
        {
            // Bounded log output - prevents reading past payload buffer
            Serial.printf("[WSc] get text (len %u): %.*s\n", (unsigned int)length, (int)length, (char*)payload);
            
            // Parse JSON passing explicit length
            StaticJsonDocument<1024> doc;
            DeserializationError error = deserializeJson(doc, payload, length);
            
            if (error) {
                Serial.print(F("[WSc] deserializeJson() failed: "));
                Serial.println(error.f_str());
                return;
            }
            
            AnimPacket pkt;
            memset(&pkt, 0, sizeof(pkt));

            const char* lyricStr = doc["lyric"] | "";
            const char* nextStr = doc["next"] | "";
            const char* animStr = doc["animation"] | "fade";
            const char* titleStr = doc["title"] | "";
            const char* artistStr = doc["artist"] | "";

            strncpy(pkt.lyric, lyricStr, sizeof(pkt.lyric) - 1);
            strncpy(pkt.nextLyric, nextStr, sizeof(pkt.nextLyric) - 1);
            strncpy(pkt.animType, animStr, sizeof(pkt.animType) - 1);
            strncpy(pkt.title, titleStr, sizeof(pkt.title) - 1);
            strncpy(pkt.artist, artistStr, sizeof(pkt.artist) - 1);
            
            // Fallback: If lyric is empty but title is present, display song title & artist!
            if (strlen(pkt.lyric) == 0 && strlen(pkt.title) > 0) {
                strncpy(pkt.lyric, pkt.title, sizeof(pkt.lyric) - 1);
                strncpy(pkt.nextLyric, pkt.artist, sizeof(pkt.nextLyric) - 1);
            }
            
            pkt.bpm = doc["bpm"] | 120.0f;
            pkt.energy = doc["energy"] | 0.5f;
            pkt.duration = doc["duration"] | 5000;
            
            pkt.beatStrength = doc["beatStrength"] | 0.5f;
            pkt.bass = doc["bass"] | 0.5f;
            
            const char* emotionStr = doc["emotion"] | "Calm";
            const char* sceneStr = doc["scene"] | "Verse";
            const char* secondaryStr = doc["secondary"] | "None";
            const char* fontStr = doc["font"] | "Medium";
            const char* particlesStr = doc["particles"] | "None";

            strncpy(pkt.emotion, emotionStr, sizeof(pkt.emotion) - 1);
            strncpy(pkt.scene, sceneStr, sizeof(pkt.scene) - 1);
            strncpy(pkt.secondary, secondaryStr, sizeof(pkt.secondary) - 1);
            strncpy(pkt.font, fontStr, sizeof(pkt.font) - 1);
            strncpy(pkt.particles, particlesStr, sizeof(pkt.particles) - 1);
            
            pkt.x = doc["x"] | 64;
            pkt.y = doc["y"] | 32;
            pkt.shake = doc["shake"] | false;
            pkt.invert = doc["invert"] | false;
            
            bool isPlaying = doc["is_playing"] | true;
            
            setAnimationState(pkt);
            setVisualsData(pkt.bpm, pkt.energy);
            setMusicState(isPlaying, pkt.bpm);
            currentMode = MODE_LYRICS;
            break;
        }
        case WStype_BIN:
            break;
        case WStype_PING:
            break;
        case WStype_PONG:
            break;
    }
}

void websocketSetup() {
    String path = WEBSOCKET_PATH;
    if (strlen(WEBSOCKET_TOKEN) > 0) {
        path += "?token=";
        path += WEBSOCKET_TOKEN;
    }
    
    Serial.printf("[WSc] Free Heap: %u bytes\n", (unsigned int)ESP.getFreeHeap());
    if (WEBSOCKET_PORT == 443) {
        Serial.printf("[WSc] Initializing SSL WebSocket (WSS) to %s:%d%s\n", WEBSOCKET_HOST, WEBSOCKET_PORT, path.c_str());
        webSocket.beginSSL(WEBSOCKET_HOST, WEBSOCKET_PORT, path.c_str());
    } else {
        Serial.printf("[WSc] Initializing plain WebSocket (WS) to %s:%d%s\n", WEBSOCKET_HOST, WEBSOCKET_PORT, path.c_str());
        webSocket.begin(WEBSOCKET_HOST, WEBSOCKET_PORT, path.c_str());
    }
    
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
    // Send ping every 15s, expect pong within 3s, drop if 2 fails (prevents Render/NAT timeouts)
    webSocket.enableHeartbeat(15000, 3000, 2);
}

void websocketLoop() {
    webSocket.loop();
}
