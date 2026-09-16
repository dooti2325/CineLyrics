#include "ble_manager.h"
#include "config.h"
#include "face.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Global State
static bool deviceConnected = false;
static bool oldDeviceConnected = false;
static BLEServer* pServer = nullptr;

// Characteristics pointers for notifications/read updates
static BLECharacteristic* pCharBuddyBattery = nullptr;
static BLECharacteristic* pCharBleConn = nullptr;
static BLECharacteristic* pCharDeviceStatus = nullptr;

static NotificationData currentNotification = {"", "", "", "", false, 0};
static PhoneStatusData currentPhoneStatus = {100, false, 0, false};
static MediaAction pendingMediaAction = MEDIA_NONE;

// Forward declaration
extern bool isAsleep;

// Server connection callbacks
class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        deviceConnected = true;
        Serial.println("[BLE] Client connected");
        if (pCharBleConn) {
            uint8_t connState = 1;
            pCharBleConn->setValue(&connState, 1);
            pCharBleConn->notify();
        }
    }

    void onDisconnect(BLEServer* pServer) override {
        deviceConnected = false;
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
            isAsleep = false;
            playFaceAnimation(ANIM_WAKE);
            bleSetDeviceStatus("ACTIVE");
        } else if (uuid.equalsIgnoreCase(CHAR_CMD_SLEEP_UUID)) {
            Serial.println("[BLE] Command: SLEEP");
            isAsleep = true;
            playFaceAnimation(ANIM_SLEEP);
            bleSetDeviceStatus("SLEEPING");
        } else if (uuid.equalsIgnoreCase(CHAR_CMD_CUSTOM_UUID)) {
            String emotion = value;
            emotion.toLowerCase();
            emotion.trim();
            Serial.printf("[BLE] Command: CUSTOM EMOTION '%s'\n", emotion.c_str());

            currentMode = MODE_FACE;
            isAsleep = false;

            if (emotion == "happy") playFaceAnimation(ANIM_HAPPY);
            else if (emotion == "laugh") playFaceAnimation(ANIM_LAUGH);
            else if (emotion == "sad") playFaceAnimation(ANIM_SAD);
            else if (emotion == "love") playFaceAnimation(ANIM_LOVE);
            else if (emotion == "shock") playFaceAnimation(ANIM_SHOCK);
            else if (emotion == "sleep") { isAsleep = true; playFaceAnimation(ANIM_SLEEP); }
            else if (emotion == "wake") { isAsleep = false; playFaceAnimation(ANIM_WAKE); }
            else if (emotion == "angry") playFaceAnimation(ANIM_ANGRY);
            else if (emotion == "curious") playFaceAnimation(ANIM_CURIOUS);
            else if (emotion == "thinking") playFaceAnimation(ANIM_THINKING);
            else if (emotion == "vibe") playFaceAnimation(ANIM_VIBE);
            else {
                playFaceAnimation(ANIM_CURIOUS);
            }
        }
    }
};

// Notification Characteristics Callback
class NotificationCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) override {
        String value = pCharacteristic->getValue();
        String uuid = pCharacteristic->getUUID().toString();

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
            
            // Trigger curiosity/shock animation on notification
            playFaceAnimation(ANIM_SHOCK);
        } else if (uuid.equalsIgnoreCase(CHAR_NOTIF_TIME_UUID)) {
            currentNotification.timestamp = value;
        }
    }
};

// Phone Telemetry Callback
class PhoneCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) override {
        String value = pCharacteristic->getValue();
        String uuid = pCharacteristic->getUUID().toString();

        if (uuid.equalsIgnoreCase(CHAR_PHONE_BATTERY_UUID)) {
            if (value.length() > 0) {
                currentPhoneStatus.battery = (uint8_t)value[0];
                if (value.length() > 1 && value[0] >= '0' && value[0] <= '9') {
                    // Passed as string "85"
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
    }
};

// Media Characteristic Callback
class MediaCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) override {
        String uuid = pCharacteristic->getUUID().toString();
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
    }
};

static CommandCallbacks cmdCallbacks;
static NotificationCallbacks notifCallbacks;
static PhoneCallbacks phoneCallbacks;
static MediaCallbacks mediaCallbacks;

void bleSetup() {
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

    // Setup Advertising
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_STATUS_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    BLEDevice::startAdvertising();
    Serial.println("[BLE] Advertising started as: " BLE_DEVICE_NAME);
}

void bleLoop() {
    // Restart advertising if disconnected
    if (!deviceConnected && oldDeviceConnected) {
        delay(100);
        BLEDevice::startAdvertising();
        Serial.println("[BLE] Restarted advertising after disconnect");
        oldDeviceConnected = deviceConnected;
    }
    // Update connected state
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    // Auto-dismiss notification after timeout
    if (currentNotification.active) {
        if (millis() - currentNotification.receivedAt > NOTIFICATION_POPUP_MS) {
            bleDismissNotification();
        }
    }
}

bool bleIsClientConnected() {
    return deviceConnected;
}

void bleSetBuddyBattery(uint8_t percent) {
    if (pCharBuddyBattery) {
        pCharBuddyBattery->setValue(&percent, 1);
        if (deviceConnected) {
            pCharBuddyBattery->notify();
        }
    }
}

void bleSetDeviceStatus(const char* status) {
    if (pCharDeviceStatus) {
        pCharDeviceStatus->setValue(status);
        if (deviceConnected) {
            pCharDeviceStatus->notify();
        }
    }
}

bool bleHasActiveNotification() {
    return currentNotification.active;
}

NotificationData bleGetNotification() {
    return currentNotification;
}

void bleDismissNotification() {
    currentNotification.active = false;
}

PhoneStatusData bleGetPhoneStatus() {
    return currentPhoneStatus;
}

MediaAction bleConsumeMediaAction() {
    MediaAction action = pendingMediaAction;
    pendingMediaAction = MEDIA_NONE;
    return action;
}
