#include "ble_manager.h"
#include "config.h"
#include "face.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// FreeRTOS Synchronization Mutex
static SemaphoreHandle_t bleMutex = nullptr;
#define BLE_LOCK()   do { if (bleMutex) xSemaphoreTake(bleMutex, portMAX_DELAY); } while(0)
#define BLE_UNLOCK() do { if (bleMutex) xSemaphoreGive(bleMutex); } while(0)

// Global State
static bool deviceConnected = false;
static bool oldDeviceConnected = false;
static BLEServer* pServer = nullptr;

// Characteristics pointers for notifications/read updates
static BLECharacteristic* pCharBuddyBattery = nullptr;
static BLECharacteristic* pCharBleConn = nullptr;
static BLECharacteristic* pCharDeviceStatus = nullptr;
static BLECharacteristic* pCharWifiStatus = nullptr;

static NotificationData currentNotification = {"", "", "", "", false, 0};
static PhoneStatusData currentPhoneStatus = {100, false, 0, false};
static MediaAction pendingMediaAction = MEDIA_NONE;

// Thread-safe Face Animation Queue
static bool hasPendingFaceAnimState = false;
static FaceAnim pendingFaceAnimValue = ANIM_IDLE;

// Wi-Fi Provisioning Buffer
static String pendingWifiSSID = "";
static String pendingWifiPass = "";
static bool wifiConfigUpdatedFlag = false;

// Forward declaration
extern bool isAsleep;

static void queueFaceAnim(FaceAnim anim) {
    BLE_LOCK();
    pendingFaceAnimValue = anim;
    hasPendingFaceAnimState = true;
    BLE_UNLOCK();
}

// Server connection callbacks
class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        BLE_LOCK();
        deviceConnected = true;
        BLE_UNLOCK();
        Serial.println("[BLE] Client connected");
        if (pCharBleConn) {
            uint8_t connState = 1;
            pCharBleConn->setValue(&connState, 1);
            pCharBleConn->notify();
        }
    }

    void onDisconnect(BLEServer* pServer) override {
        BLE_LOCK();
        deviceConnected = false;
        BLE_UNLOCK();
        Serial.println("[BLE] Client disconnected");
        if (pCharBleConn) {
            uint8_t connState = 0;
            pCharBleConn->setValue(&connState, 1);
        }
    }
};

// Command Characteristic Callback
class CommandCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) override {
        String value = pCharacteristic->getValue();
        String uuid = pCharacteristic->getUUID().toString();

        if (uuid.equalsIgnoreCase(CHAR_CMD_WAKE_UUID)) {
            Serial.println("[BLE] Command: WAKE");
            queueFaceAnim(ANIM_WAKE);
            bleSetDeviceStatus("ACTIVE");
        } else if (uuid.equalsIgnoreCase(CHAR_CMD_SLEEP_UUID)) {
            Serial.println("[BLE] Command: SLEEP");
            queueFaceAnim(ANIM_SLEEP);
            bleSetDeviceStatus("SLEEPING");
        } else if (uuid.equalsIgnoreCase(CHAR_CMD_CUSTOM_UUID)) {
            String emotion = value;
            emotion.toLowerCase();
            emotion.trim();
            if (emotion.length() > 32) emotion = emotion.substring(0, 32);
            Serial.printf("[BLE] Command: CUSTOM EMOTION '%s'\n", emotion.c_str());

            if (emotion == "happy") queueFaceAnim(ANIM_HAPPY);
            else if (emotion == "laugh") queueFaceAnim(ANIM_LAUGH);
            else if (emotion == "sad") queueFaceAnim(ANIM_SAD);
            else if (emotion == "love") queueFaceAnim(ANIM_LOVE);
            else if (emotion == "shock") queueFaceAnim(ANIM_SHOCK);
            else if (emotion == "sleep") queueFaceAnim(ANIM_SLEEP);
            else if (emotion == "wake") queueFaceAnim(ANIM_WAKE);
            else if (emotion == "angry") queueFaceAnim(ANIM_ANGRY);
            else if (emotion == "curious") queueFaceAnim(ANIM_CURIOUS);
            else if (emotion == "thinking") queueFaceAnim(ANIM_THINKING);
            else if (emotion == "vibe") queueFaceAnim(ANIM_VIBE);
            else {
                queueFaceAnim(ANIM_CURIOUS);
            }
        }
    }
};

// Notification Characteristics Callback
class NotificationCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) override {
        String value = pCharacteristic->getValue();
        // Bound length to prevent heap exhaustion
        if (value.length() > 128) {
            value = value.substring(0, 128);
        }
        String uuid = pCharacteristic->getUUID().toString();

        BLE_LOCK();
        if (uuid.equalsIgnoreCase(CHAR_NOTIF_APP_UUID)) {
            currentNotification.app = value;
        } else if (uuid.equalsIgnoreCase(CHAR_NOTIF_TITLE_UUID)) {
            currentNotification.title = value;
        } else if (uuid.equalsIgnoreCase(CHAR_NOTIF_MSG_UUID)) {
            currentNotification.message = value;
            currentNotification.active = true;
            currentNotification.receivedAt = millis();
            Serial.printf("[BLE] Notification from %s: %s - %s\n", 
                currentNotification.app.c_str(), 
                currentNotification.title.c_str(), 
                currentNotification.message.c_str());
        } else if (uuid.equalsIgnoreCase(CHAR_NOTIF_TIME_UUID)) {
            currentNotification.timestamp = value;
        }
        BLE_UNLOCK();

        if (uuid.equalsIgnoreCase(CHAR_NOTIF_MSG_UUID)) {
            queueFaceAnim(ANIM_SHOCK);
        }
    }
};

// Phone Telemetry Callback
class PhoneCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) override {
        String value = pCharacteristic->getValue();
        String uuid = pCharacteristic->getUUID().toString();

        BLE_LOCK();
        if (uuid.equalsIgnoreCase(CHAR_PHONE_BATTERY_UUID)) {
            if (value.length() > 0) {
                currentPhoneStatus.battery = (uint8_t)value[0];
                if (value.length() > 1 && value[0] >= '0' && value[0] <= '9') {
                    currentPhoneStatus.battery = (uint8_t)value.toInt();
                }
                currentPhoneStatus.updated = true;
                Serial.printf("[BLE] Phone Battery: %d%%\n", currentPhoneStatus.battery);
            }
        } else if (uuid.equalsIgnoreCase(CHAR_PHONE_CHARGING_UUID)) {
            if (value.length() > 0) {
                currentPhoneStatus.isCharging = (value[0] == 1 || value[0] == '1' || value[0] == 't');
                currentPhoneStatus.updated = true;
                Serial.printf("[BLE] Phone Charging: %s\n", currentPhoneStatus.isCharging ? "YES" : "NO");
            }
        } else if (uuid.equalsIgnoreCase(CHAR_PHONE_CONN_UUID)) {
            if (value.length() > 0) {
                currentPhoneStatus.connection = (uint8_t)value[0];
                currentPhoneStatus.updated = true;
            }
        }
        BLE_UNLOCK();
    }
};

// Media Characteristic Callback
class MediaCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) override {
        String uuid = pCharacteristic->getUUID().toString();
        BLE_LOCK();
        if (uuid.equalsIgnoreCase(CHAR_MEDIA_PLAYPAUSE_UUID)) {
            Serial.println("[BLE] Media: PLAY/PAUSE");
            pendingMediaAction = MEDIA_TOGGLE;
        } else if (uuid.equalsIgnoreCase(CHAR_MEDIA_NEXT_UUID)) {
            Serial.println("[BLE] Media: NEXT");
            pendingMediaAction = MEDIA_NEXT;
        } else if (uuid.equalsIgnoreCase(CHAR_MEDIA_PREV_UUID)) {
            Serial.println("[BLE] Media: PREVIOUS");
            pendingMediaAction = MEDIA_PREVIOUS;
        }
        BLE_UNLOCK();
    }
};

// Wi-Fi Configuration Callback
class WifiConfigCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) override {
        String value = pCharacteristic->getValue();
        String uuid = pCharacteristic->getUUID().toString();

        BLE_LOCK();
        if (uuid.equalsIgnoreCase(CHAR_WIFI_SSID_UUID)) {
            if (value.length() > 64) value = value.substring(0, 64);
            pendingWifiSSID = value;
            Serial.printf("[BLE] Wi-Fi SSID received: %s\n", pendingWifiSSID.c_str());
        } else if (uuid.equalsIgnoreCase(CHAR_WIFI_PASS_UUID)) {
            if (value.length() > 64) value = value.substring(0, 64);
            pendingWifiPass = value;
            Serial.println("[BLE] Wi-Fi Password received");
        } else if (uuid.equalsIgnoreCase(CHAR_WIFI_APPLY_UUID)) {
            wifiConfigUpdatedFlag = true;
            Serial.println("[BLE] Wi-Fi Apply command received");
        }
        BLE_UNLOCK();
    }
};

static CommandCallbacks cmdCallbacks;
static NotificationCallbacks notifCallbacks;
static PhoneCallbacks phoneCallbacks;
static MediaCallbacks mediaCallbacks;
static WifiConfigCallbacks wifiConfigCallbacks;

void bleSetup() {
    if (!bleMutex) {
        bleMutex = xSemaphoreCreateMutex();
    }

    Serial.println("[BLE] Initializing DeskBuddy BLE...");
    BLEDevice::init(BLE_DEVICE_NAME);

    // Create Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    // 1. STATUS SERVICE
    BLEService* pStatusService = pServer->createService(SERVICE_STATUS_UUID);
    pCharBuddyBattery = pStatusService->createCharacteristic(
        CHAR_BUDDY_BATTERY_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharBuddyBattery->addDescriptor(new BLE2902());
    uint8_t initBatt = 100;
    pCharBuddyBattery->setValue(&initBatt, 1);

    pCharBleConn = pStatusService->createCharacteristic(
        CHAR_BLE_CONNECTION_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharBleConn->addDescriptor(new BLE2902());
    uint8_t initConn = 0;
    pCharBleConn->setValue(&initConn, 1);

    pCharDeviceStatus = pStatusService->createCharacteristic(
        CHAR_DEVICE_STATUS_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharDeviceStatus->addDescriptor(new BLE2902());
    pCharDeviceStatus->setValue("IDLE");
    pStatusService->start();

    // 2. NOTIFICATION SERVICE
    BLEService* pNotifService = pServer->createService(SERVICE_NOTIFICATION_UUID);
    BLECharacteristic* pCharApp = pNotifService->createCharacteristic(
        CHAR_NOTIF_APP_UUID, BLECharacteristic::PROPERTY_WRITE
    );
    pCharApp->setCallbacks(&notifCallbacks);

    BLECharacteristic* pCharTitle = pNotifService->createCharacteristic(
        CHAR_NOTIF_TITLE_UUID, BLECharacteristic::PROPERTY_WRITE
    );
    pCharTitle->setCallbacks(&notifCallbacks);

    BLECharacteristic* pCharMsg = pNotifService->createCharacteristic(
        CHAR_NOTIF_MSG_UUID, BLECharacteristic::PROPERTY_WRITE
    );
    pCharMsg->setCallbacks(&notifCallbacks);

    BLECharacteristic* pCharTime = pNotifService->createCharacteristic(
        CHAR_NOTIF_TIME_UUID, BLECharacteristic::PROPERTY_WRITE
    );
    pCharTime->setCallbacks(&notifCallbacks);
    pNotifService->start();

    // 3. PHONE SERVICE
    BLEService* pPhoneService = pServer->createService(SERVICE_PHONE_UUID);
    BLECharacteristic* pCharPhoneBatt = pPhoneService->createCharacteristic(
        CHAR_PHONE_BATTERY_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ
    );
    pCharPhoneBatt->setCallbacks(&phoneCallbacks);

    BLECharacteristic* pCharPhoneCharging = pPhoneService->createCharacteristic(
        CHAR_PHONE_CHARGING_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ
    );
    pCharPhoneCharging->setCallbacks(&phoneCallbacks);

    BLECharacteristic* pCharPhoneConn = pPhoneService->createCharacteristic(
        CHAR_PHONE_CONN_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ
    );
    pCharPhoneConn->setCallbacks(&phoneCallbacks);
    pPhoneService->start();

    // 4. MEDIA SERVICE
    BLEService* pMediaService = pServer->createService(SERVICE_MEDIA_UUID);
    BLECharacteristic* pCharMediaPlay = pMediaService->createCharacteristic(
        CHAR_MEDIA_PLAYPAUSE_UUID, BLECharacteristic::PROPERTY_WRITE
    );
    pCharMediaPlay->setCallbacks(&mediaCallbacks);

    BLECharacteristic* pCharMediaNext = pMediaService->createCharacteristic(
        CHAR_MEDIA_NEXT_UUID, BLECharacteristic::PROPERTY_WRITE
    );
    pCharMediaNext->setCallbacks(&mediaCallbacks);

    BLECharacteristic* pCharMediaPrev = pMediaService->createCharacteristic(
        CHAR_MEDIA_PREV_UUID, BLECharacteristic::PROPERTY_WRITE
    );
    pCharMediaPrev->setCallbacks(&mediaCallbacks);
    pMediaService->start();

    // 5. COMMAND SERVICE
    BLEService* pCmdService = pServer->createService(SERVICE_COMMAND_UUID);
    BLECharacteristic* pCharWake = pCmdService->createCharacteristic(
        CHAR_CMD_WAKE_UUID, BLECharacteristic::PROPERTY_WRITE
    );
    pCharWake->setCallbacks(&cmdCallbacks);

    BLECharacteristic* pCharSleep = pCmdService->createCharacteristic(
        CHAR_CMD_SLEEP_UUID, BLECharacteristic::PROPERTY_WRITE
    );
    pCharSleep->setCallbacks(&cmdCallbacks);

    BLECharacteristic* pCharCustom = pCmdService->createCharacteristic(
        CHAR_CMD_CUSTOM_UUID, BLECharacteristic::PROPERTY_WRITE
    );
    pCharCustom->setCallbacks(&cmdCallbacks);
    pCmdService->start();

    // 6. WI-FI PROVISIONING SERVICE
    BLEService* pWifiService = pServer->createService(SERVICE_WIFI_UUID);
    BLECharacteristic* pCharWifiSSID = pWifiService->createCharacteristic(
        CHAR_WIFI_SSID_UUID, BLECharacteristic::PROPERTY_WRITE
    );
    pCharWifiSSID->setCallbacks(&wifiConfigCallbacks);

    BLECharacteristic* pCharWifiPass = pWifiService->createCharacteristic(
        CHAR_WIFI_PASS_UUID, BLECharacteristic::PROPERTY_WRITE
    );
    pCharWifiPass->setCallbacks(&wifiConfigCallbacks);

    BLECharacteristic* pCharWifiApply = pWifiService->createCharacteristic(
        CHAR_WIFI_APPLY_UUID, BLECharacteristic::PROPERTY_WRITE
    );
    pCharWifiApply->setCallbacks(&wifiConfigCallbacks);

    pCharWifiStatus = pWifiService->createCharacteristic(
        CHAR_WIFI_STATUS_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharWifiStatus->addDescriptor(new BLE2902());
    pCharWifiStatus->setValue("IDLE");
    pWifiService->start();

    // Setup Advertising
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_STATUS_UUID);
    pAdvertising->addServiceUUID(SERVICE_WIFI_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    BLEDevice::startAdvertising();
    Serial.println("[BLE] Advertising started as: " BLE_DEVICE_NAME);
}

void bleLoop() {
    bool isConn = false;
    BLE_LOCK();
    isConn = deviceConnected;
    BLE_UNLOCK();

    // Restart advertising if disconnected
    if (!isConn && oldDeviceConnected) {
        delay(100);
        BLEDevice::startAdvertising();
        Serial.println("[BLE] Restarted advertising after disconnect");
        oldDeviceConnected = false;
    }
    // Update connected state
    if (isConn && !oldDeviceConnected) {
        oldDeviceConnected = true;
    }

    // Auto-dismiss notification after timeout
    BLE_LOCK();
    if (currentNotification.active) {
        if (millis() - currentNotification.receivedAt > NOTIFICATION_POPUP_MS) {
            currentNotification.active = false;
        }
    }
    BLE_UNLOCK();
}

bool bleIsClientConnected() {
    BLE_LOCK();
    bool conn = deviceConnected;
    BLE_UNLOCK();
    return conn;
}

void bleSetBuddyBattery(uint8_t percent) {
    if (pCharBuddyBattery) {
        pCharBuddyBattery->setValue(&percent, 1);
        if (bleIsClientConnected()) {
            pCharBuddyBattery->notify();
        }
    }
}

void bleSetDeviceStatus(const char* status) {
    if (pCharDeviceStatus) {
        pCharDeviceStatus->setValue(status);
        if (bleIsClientConnected()) {
            pCharDeviceStatus->notify();
        }
    }
}

bool bleHasActiveNotification() {
    BLE_LOCK();
    bool active = currentNotification.active;
    BLE_UNLOCK();
    return active;
}

NotificationData bleGetNotification() {
    BLE_LOCK();
    NotificationData copy = currentNotification;
    BLE_UNLOCK();
    return copy;
}

void bleDismissNotification() {
    BLE_LOCK();
    currentNotification.active = false;
    BLE_UNLOCK();
}

PhoneStatusData bleGetPhoneStatus() {
    BLE_LOCK();
    PhoneStatusData copy = currentPhoneStatus;
    BLE_UNLOCK();
    return copy;
}

MediaAction bleConsumeMediaAction() {
    BLE_LOCK();
    MediaAction action = pendingMediaAction;
    pendingMediaAction = MEDIA_NONE;
    BLE_UNLOCK();
    return action;
}

bool bleHasPendingFaceAnim() {
    BLE_LOCK();
    bool has = hasPendingFaceAnimState;
    BLE_UNLOCK();
    return has;
}

FaceAnim bleConsumePendingFaceAnim() {
    BLE_LOCK();
    FaceAnim anim = pendingFaceAnimValue;
    hasPendingFaceAnimState = false;
    BLE_UNLOCK();
    return anim;
}

bool bleHasWifiConfigUpdate() {
    BLE_LOCK();
    bool updated = wifiConfigUpdatedFlag;
    BLE_UNLOCK();
    return updated;
}

bool bleGetWifiConfig(String& ssid, String& password) {
    BLE_LOCK();
    if (!wifiConfigUpdatedFlag) {
        BLE_UNLOCK();
        return false;
    }
    ssid = pendingWifiSSID;
    password = pendingWifiPass;
    wifiConfigUpdatedFlag = false;
    BLE_UNLOCK();
    return true;
}

void bleSetWifiStatus(const char* status) {
    if (pCharWifiStatus) {
        pCharWifiStatus->setValue(status);
        if (bleIsClientConnected()) {
            pCharWifiStatus->notify();
        }
    }
}
