#include "websocket_client.h"
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "animations.h"

WebSocketsClient webSocket;

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
            
            const char* current = doc["current"] | "";
            const char* next = doc["next"] | "";
            const char* anim = doc["animation"] | "fade";
            const char* title = doc["title"] | "";
            const char* artist = doc["artist"] | "";
            float bpm = doc["bpm"] | 120.0;
            float energy = doc["energy"] | 0.5;
            int dur = doc["dur"] | 5000;
            
            setAnimationState(current, next, anim, title, artist, bpm, energy, dur);
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
