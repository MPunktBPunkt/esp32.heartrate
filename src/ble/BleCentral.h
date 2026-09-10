#pragma once

#include "BleTypes.h"
#include "core/ConfigStore.h"
#include "core/HistoryStore.h"
#include <NimBLEDevice.h>

class BleCentral {
public:
    void begin(ConfigStore* cfg, HistoryStore* history);
    void loop();

    BleState state() const { return state_; }
    const char* stateName() const { return bleStateName(state_); }
    bool scanning() const { return scanning_; }
    bool sessionActive() const { return sessionActive_; }
    uint16_t linkLossCount() const { return linkLossCount_; }
    uint16_t reconnectOkCount() const { return reconnectOkCount_; }
    /** Returns false if no RSSI samples yet. */
    bool linkRssiStats(int8_t& minOut, int8_t& maxOut, int8_t& avgOut) const;
    bool connectBusy() const { return connectBusy_; }

    void startScan();
    void stopScan();
    /** Queue a connect; GATT work runs on a FreeRTOS task (HTTP/SSE stay alive). */
    bool connect(const char* mac);
    void disconnect();
    void rememberCurrent();
    void forgetRemembered();

    uint8_t scanCount() const { return scanCount_; }
    const ScanDevice* scanDevice(uint8_t i) const;
    void scanToJson(JsonArray arr) const;

    const HrSample& lastSample() const { return last_; }
    bool hasSample() const { return hasSample_; }
    bool takeSampleDirty();
    const char* peerMac() const { return peerMac_; }
    const char* peerName() const { return peerName_; }
    unsigned long connectedSinceMs() const { return connectedSince_; }
    unsigned long lastPacketMs() const { return lastPacketMs_; }
    const char* lastError() const { return lastError_; }

    void appendStatusJson(JsonObject obj) const;
    void appendIoValues(JsonObject ios) const;

    // called from NimBLE callbacks
    void onScanResult(NimBLEAdvertisedDevice* advertisedDevice);
    void onNotify(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len);
    void onDisconnect();

private:
    friend void bleConnectTaskThunk(void* arg);

    void setState(BleState s);
    void queueConnect(const char* mac, bool isReconnect);
    bool connectInternal(const char* mac, bool isReconnect);
    bool subscribeHr(NimBLEClient* client);
    void pollBattery(NimBLEClient* client);
    void pollRssi(NimBLEClient* client);
    void handleIdleDisconnect();
    void handleSessionReconnect();
    void startReconnectScan();
    void applyConnParams(NimBLEClient* client);
    void cleanupClient();
    int findScanIndex(const char* addr) const;
    uint8_t resolveAddrType(const char* mac) const;
    unsigned long reconnectBackoffMs() const;

    ConfigStore* cfg_ = nullptr;
    HistoryStore* history_ = nullptr;
    NimBLEClient* client_ = nullptr;

    BleState state_ = BleState::Idle;
    bool scanning_ = false;
    bool sessionActive_ = false;
    bool wantDisconnect_ = false;
    volatile bool disconnectEvt_ = false;
    bool reconnectScanning_ = false;
    volatile bool connectQueued_ = false;
    volatile bool connectBusy_ = false;
    bool connectIsReconnect_ = false;
    volatile bool sampleDirty_ = false;
    uint8_t reconnectTries_ = 0;
    uint8_t peerAddrType_ = BLE_ADDR_PUBLIC;

    ScanDevice scan_[HR_MAX_SCAN];
    uint8_t scanCount_ = 0;

    char peerMac_[18] = {0};
    char peerName_[32] = {0};
    char pendingMac_[18] = {0};
    char lastError_[64] = {0};

    HrSample last_;
    bool hasSample_ = false;
    unsigned long connectedSince_ = 0;
    unsigned long lastPacketMs_ = 0;
    unsigned long lastBatteryPoll_ = 0;
    unsigned long lastRssiPoll_ = 0;
    unsigned long lostAt_ = 0;
    unsigned long reconnectScanUntil_ = 0;
    unsigned long reconnectWindowStart_ = 0;
    int8_t battery_ = -1;

    // Per-session link continuity (reset on fresh connect)
    unsigned long sessionLinkStartMs_ = 0;
    uint16_t linkLossCount_ = 0;
    uint16_t reconnectOkCount_ = 0;
    int8_t rssiMin_ = 0;
    int8_t rssiMax_ = 0;
    int32_t rssiSum_ = 0;
    uint16_t rssiSamples_ = 0;
    bool haveRssiStats_ = false;

    void resetLinkStats();
    void noteRssi(int8_t rssi);
    static const char* linkQualityName(int8_t rssi);
};
