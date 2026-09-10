#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

enum class BleState : uint8_t {
    Idle = 0,
    Scanning,
    Connecting,
    Connected,
    Subscribing,
    Ready,
    Lost,
    Reconnecting,
    Disconnecting,
    Error
};

inline const char* bleStateName(BleState s) {
    switch (s) {
        case BleState::Idle: return "IDLE";
        case BleState::Scanning: return "SCANNING";
        case BleState::Connecting: return "CONNECTING";
        case BleState::Connected: return "CONNECTED";
        case BleState::Subscribing: return "SUBSCRIBING";
        case BleState::Ready: return "READY";
        case BleState::Lost: return "LOST";
        case BleState::Reconnecting: return "RECONNECTING";
        case BleState::Disconnecting: return "DISCONNECTING";
        case BleState::Error: return "ERROR";
        default: return "UNKNOWN";
    }
}

enum class RrQuality : uint8_t {
    Valid = 0,
    Suspect = 1,
    Artifact = 2,
    Invalid = 3
};

inline const char* rrQualityName(RrQuality q) {
    switch (q) {
        case RrQuality::Valid: return "VALID";
        case RrQuality::Suspect: return "SUSPECT";
        case RrQuality::Artifact: return "ARTIFACT";
        case RrQuality::Invalid: return "INVALID";
        default: return "UNKNOWN";
    }
}

/** Convert BLE RR unit (1/1024 s) to milliseconds. */
inline uint16_t rrRawToMs(uint16_t rrRaw) {
    return (uint16_t)((rrRaw * 1000UL) / 1024UL);
}

struct ScanDevice {
    char addr[18];
    char name[32];
    int8_t rssi;
    uint8_t addrType;
    bool hasHrService;
    uint32_t lastSeen;
};

struct HrSample {
    uint32_t tMs = 0;                 // packet receive time (monotonic)
    uint16_t heartRate = 0;
    uint8_t rrCount = 0;
    uint16_t rrRaw[8] = {0};          // original BLE units 1/1024 s
    int8_t battery = -1;
    int8_t rssi = 0;
    uint8_t contact = 0;              // 0 unknown/unsupported, 1 detected, 2 not_detected
    bool contactSupported = false;    // BLE flag bit2 (Polar H9 typically false)
    bool energyPresent = false;
    uint16_t energyExpended = 0;
    uint8_t flags = 0;

    uint16_t rrMsAt(uint8_t i) const { return i < rrCount ? rrRawToMs(rrRaw[i]) : 0; }
};

bool parseHeartRateMeasurement(const uint8_t* data, size_t len, HrSample& out);

/** Encode sample into HRM wire format. Returns bytes written, 0 on failure.
 *  Optional truncatedOut is set true when RR values were dropped for MTU budget. */
size_t buildHeartRateMeasurement(const HrSample& s, uint8_t* out, size_t cap,
                                 bool* truncatedOut = nullptr);
