#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include "face.h"

struct NotificationData {
    String app;
    String title;
    String message;
    String timestamp;
    bool active;
    unsigned long receivedAt;
};

struct PhoneStatusData {
    uint8_t battery;      // 0 - 100%
    bool isCharging;      // true if charging
    uint8_t connection;   // 0: disconnected, 1: connected
    bool updated;
};

enum MediaAction {
    MEDIA_NONE,
    MEDIA_TOGGLE,
    MEDIA_NEXT,
    MEDIA_PREVIOUS
};

// Core BLE Management API
void bleSetup();
void bleLoop();

bool bleIsClientConnected();
void bleSetBuddyBattery(uint8_t percent);
void bleSetDeviceStatus(const char* status);

// Access received telemetry & notifications (Thread-safe)
bool bleHasActiveNotification();
NotificationData bleGetNotification();
void bleDismissNotification();

PhoneStatusData bleGetPhoneStatus();

// Media actions (Thread-safe)
MediaAction bleConsumeMediaAction();

// Thread-safe dispatch of Face animations requested over BLE
bool bleHasPendingFaceAnim();
FaceAnim bleConsumePendingFaceAnim();

// Thread-safe Wi-Fi Provisioning over BLE
bool bleHasWifiConfigUpdate();
bool bleGetWifiConfig(String& ssid, String& password);
void bleSetWifiStatus(const char* status);

#endif // BLE_MANAGER_H
