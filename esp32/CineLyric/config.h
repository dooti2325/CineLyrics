#ifndef CONFIG_H
#define CONFIG_H

// WiFi Credentials
const char *const WIFI_SSID = "Dooti_S23";
const char *const WIFI_PASSWORD = "123456789";

// WebSocket Server Configuration
const char *const WEBSOCKET_HOST ="cinelyrics.onrender.com"; // Your Render URL (without https://)
const int WEBSOCKET_PORT = 443; // 443 for secure websockets (WSS)
const char *const WEBSOCKET_PATH = "/ws";

// Display Configuration
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RST U8X8_PIN_NONE // Usually none for I2C

// Touch Sensor Configuration
#define TOUCH_PIN 4        // Change to the pin connected to your capacitive touch sensor (e.g., T0 is GPIO 4)
#define TOUCH_THRESHOLD 40 // Adjust this if using internal touchRead. If using a digital module, this is ignored.

// DeskBuddy BLE Configuration
#define BLE_DEVICE_NAME "DeskBuddy-BLE"
#define NOTIFICATION_POPUP_MS 5000 // Display notification popup for 5 seconds

// DeskBuddy BLE UUIDs
#define SERVICE_STATUS_UUID        "4f711000-2a91-4c10-9b8c-b03a1a1f0001"
#define CHAR_BUDDY_BATTERY_UUID    "4f711001-2a91-4c10-9b8c-b03a1a1f0001"
#define CHAR_BLE_CONNECTION_UUID   "4f711002-2a91-4c10-9b8c-b03a1a1f0001"
#define CHAR_DEVICE_STATUS_UUID    "4f711003-2a91-4c10-9b8c-b03a1a1f0001"

#define SERVICE_NOTIFICATION_UUID  "4f712000-2a91-4c10-9b8c-b03a1a1f0001"
#define CHAR_NOTIF_APP_UUID        "4f712001-2a91-4c10-9b8c-b03a1a1f0001"
#define CHAR_NOTIF_TITLE_UUID      "4f712002-2a91-4c10-9b8c-b03a1a1f0001"
#define CHAR_NOTIF_MSG_UUID        "4f712003-2a91-4c10-9b8c-b03a1a1f0001"
#define CHAR_NOTIF_TIME_UUID       "4f712004-2a91-4c10-9b8c-b03a1a1f0001"

#define SERVICE_PHONE_UUID         "4f713000-2a91-4c10-9b8c-b03a1a1f0001"
#define CHAR_PHONE_BATTERY_UUID    "4f713001-2a91-4c10-9b8c-b03a1a1f0001"
#define CHAR_PHONE_CHARGING_UUID   "4f713002-2a91-4c10-9b8c-b03a1a1f0001"
#define CHAR_PHONE_CONN_UUID       "4f713003-2a91-4c10-9b8c-b03a1a1f0001"

#define SERVICE_MEDIA_UUID         "4f714000-2a91-4c10-9b8c-b03a1a1f0001"
#define CHAR_MEDIA_PLAYPAUSE_UUID  "4f714001-2a91-4c10-9b8c-b03a1a1f0001"
#define CHAR_MEDIA_NEXT_UUID       "4f714002-2a91-4c10-9b8c-b03a1a1f0001"
#define CHAR_MEDIA_PREV_UUID       "4f714003-2a91-4c10-9b8c-b03a1a1f0001"

#define SERVICE_COMMAND_UUID       "4f715000-2a91-4c10-9b8c-b03a1a1f0001"
#define CHAR_CMD_WAKE_UUID         "4f715001-2a91-4c10-9b8c-b03a1a1f0001"
#define CHAR_CMD_SLEEP_UUID        "4f715002-2a91-4c10-9b8c-b03a1a1f0001"
#define CHAR_CMD_CUSTOM_UUID       "4f715003-2a91-4c10-9b8c-b03a1a1f0001"

#endif // CONFIG_H
