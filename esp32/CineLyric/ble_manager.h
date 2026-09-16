#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>

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

// Access received telemetry & notifications
bool bleHasActiveNotification();
NotificationData bleGetNotification();
void bleDismissNotification();

PhoneStatusData bleGetPhoneStatus();

// Media actions
MediaAction bleConsumeMediaAction();

#endif // BLE_MANAGER_H
