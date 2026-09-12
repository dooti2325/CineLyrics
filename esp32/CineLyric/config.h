#ifndef CONFIG_H
#define CONFIG_H

// WiFi Credentials
const char *const WIFI_SSID = "Dooti_S23";
const char *const WIFI_PASSWORD = "123456789";

// WebSocket Server Configuration
const char *const WEBSOCKET_HOST ="cinelyrics.onrender.com"; // Your Render URL (without https://)
const int WEBSOCKET_PORT = 443; // 443 for secure websockets (WSS)
const char *const WEBSOCKET_PATH = "/ws";

// Display Configuration (ESP32-C3)
// Default hardware I2C pins for ESP32-C3 are SDA=8, SCL=9 (or SDA=4, SCL=5 on many mini boards)
// For ESP32-C3 SuperMini / DevKit: SDA=GPIO 8 (or 4), SCL=GPIO 9 (or 5)
// For Seeed Studio XIAO ESP32-C3: SDA=GPIO 6, SCL=GPIO 7
#define OLED_SDA 8
#define OLED_SCL 9
#define OLED_RST U8X8_PIN_NONE // Usually none for I2C

// Touch / Button Configuration (ESP32-C3)
// Note: ESP32-C3 does not have built-in capacitive touch pins (touchRead).
// Use a digital touch module (e.g. TTP223) or a momentary push button.
#define TOUCH_PIN 3            // GPIO connected to touch sensor or button (e.g., GPIO 3, 2, or 4)
#define TOUCH_ACTIVE_HIGH true // Set true for TTP223 module (active HIGH), or false for push button (active LOW with internal pull-up)

#endif // CONFIG_H
