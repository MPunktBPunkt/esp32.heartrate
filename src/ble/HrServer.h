#pragma once

#include "BuildFlags.h"

#if HR_RELAY

#include "BleTypes.h"
#include "core/ConfigStore.h"
#include <ArduinoJson.h>
#include <NimBLEDevice.h>

class BleCentral;

/** BLE Heart-Rate Peripheral that mirrors strap notifications to consumers. */
class HrServer {
public:
    void begin(ConfigStore* cfg, BleCentral* ble);
    void loop();
    void onSample(const HrSample& s);

    void setEnabled(bool on);
    bool enabled() const { return enabled_; }
    bool advertising() const { return advertising_; }
    uint8_t clients() const;
    uint8_t subscribed() const;
    bool full() const;
    const char* name() const { return name_; }
    bool holdingStrap() const { return holdingStrap_; }

    void appendStatusJson(JsonObject obj) const;
    void appendIoValues(JsonObject ios) const;

    // NimBLE callbacks (no notifies from here)
    void onConnect(uint16_t connId);
    void onDisconnect(uint16_t connId);
    void onHrSubscribe(uint16_t connId, bool subscribed);

private:
    void ensureServer();
    void startAdvertising();
    void stopAdvertising();
    void disconnectAll();
    void refreshAdvertising();
    void updateHold();
    void sendEncoded(const uint8_t* data, size_t len, bool truncated);
    void sendStaleZero();
    void updateBattery(int8_t level);
    String effectiveName() const;

    ConfigStore* cfg_ = nullptr;
    BleCentral* ble_ = nullptr;
    NimBLEServer* server_ = nullptr;
    NimBLECharacteristic* hrMeas_ = nullptr;
    NimBLECharacteristic* batLevel_ = nullptr;

    bool enabled_ = false;
    bool serverReady_ = false;
    bool advertising_ = false;
    bool holdingStrap_ = false;
    bool haveSample_ = false;
    bool staleSent_ = false;
    bool batService_ = true;
    int8_t lastBat_ = -1;
    unsigned long lastSampleMs_ = 0;
    unsigned long lastNotifyMs_ = 0;
    uint32_t notifySent_ = 0;
    uint32_t notifyFailed_ = 0;
    uint32_t truncated_ = 0;
    char name_[32] = {0};
};

#endif  // HR_RELAY
