#ifndef CONFIG_H
#define CONFIG_H

// WiFi Credentials
const char *const WIFI_SSID = "Dooti_S23";
const char *const WIFI_PASSWORD = "123456789";

// WebSocket Server Configuration
const char *const WEBSOCKET_HOST ="10.143.179.224"; // IP address of your Python server
const int WEBSOCKET_PORT = 8000;
const char *const WEBSOCKET_PATH = "/ws";

// Display Configuration
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RST U8X8_PIN_NONE // Usually none for I2C

#endif // CONFIG_H
