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
            Serial.printf("[WSc] Disconnected!\n");
            break;
        case WStype_CONNECTED:
            Serial.printf("[WSc] Connected to url: %s\n", payload);
            break;
        case WStype_TEXT:
        {
            Serial.printf("[WSc] get text: %s\n", payload);
            
            // Parse JSON
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, payload);
            
            if (error) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
                return;
            }
            
            AnimPacket pkt;
            strncpy(pkt.lyric, doc["lyric"] | "", sizeof(pkt.lyric) - 1); pkt.lyric[sizeof(pkt.lyric) - 1] = '\0';
            strncpy(pkt.nextLyric, doc["next"] | "", sizeof(pkt.nextLyric) - 1); pkt.nextLyric[sizeof(pkt.nextLyric) - 1] = '\0';
            strncpy(pkt.animType, doc["animation"] | "fade", sizeof(pkt.animType) - 1); pkt.animType[sizeof(pkt.animType) - 1] = '\0';
            strncpy(pkt.title, doc["title"] | "", sizeof(pkt.title) - 1); pkt.title[sizeof(pkt.title) - 1] = '\0';
            strncpy(pkt.artist, doc["artist"] | "", sizeof(pkt.artist) - 1); pkt.artist[sizeof(pkt.artist) - 1] = '\0';
            
            pkt.bpm = doc["bpm"] | 120.0;
            pkt.energy = doc["energy"] | 0.5;
            pkt.duration = doc["duration"] | 5000;
            
            pkt.beatStrength = doc["beatStrength"] | 0.5;
            pkt.bass = doc["bass"] | 0.5;
            
            strncpy(pkt.emotion, doc["emotion"] | "Calm", sizeof(pkt.emotion) - 1); pkt.emotion[sizeof(pkt.emotion) - 1] = '\0';
            strncpy(pkt.scene, doc["scene"] | "Verse", sizeof(pkt.scene) - 1); pkt.scene[sizeof(pkt.scene) - 1] = '\0';
            strncpy(pkt.secondary, doc["secondary"] | "None", sizeof(pkt.secondary) - 1); pkt.secondary[sizeof(pkt.secondary) - 1] = '\0';
            strncpy(pkt.font, doc["font"] | "Medium", sizeof(pkt.font) - 1); pkt.font[sizeof(pkt.font) - 1] = '\0';
            
            pkt.x = doc["x"] | 64;
            pkt.y = doc["y"] | 32;
            pkt.shake = doc["shake"] | false;
            bool invert = doc["invert"] | false;
            pkt.invert = invert;
            
            strncpy(pkt.particles, doc["particles"] | "None", sizeof(pkt.particles) - 1); pkt.particles[sizeof(pkt.particles) - 1] = '\0';
            
            bool isPlaying = doc["is_playing"] | true;
            
            setAnimationState(pkt);
            setVisualsData(pkt.bpm, pkt.energy);
            setMusicState(isPlaying, pkt.bpm);
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
    webSocket.beginSSL(WEBSOCKET_HOST, WEBSOCKET_PORT, WEBSOCKET_PATH);
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
}

void websocketLoop() {
    webSocket.loop();
}
