#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

/** Compact persisted session snapshot. */
struct SessionSummary {
    uint32_t id = 0;
    uint32_t durationS = 0;
    uint32_t samples = 0;
    uint16_t hrAvg = 0;
    uint16_t hrMin = 0;
    uint16_t hrMax = 0;
    uint16_t restingHr = 0;
    float rmssd = 0;
    float sdnn = 0;
    float pnn50 = 0;
    float calories = 0;
    uint16_t zoneS[5] = {0};  // Z1..Z5 seconds
    int16_t hrr1 = 0;
    int16_t hrr2 = 0;
    int16_t hrr5 = 0;
    uint8_t endMode = 0;
    uint8_t flags = 0;  // bit0: recovery milestones present
    uint16_t reconnectCount = 0;
    uint8_t continuityPct = 0;  // 0–100
    int8_t rssiMin = 0;
    int8_t rssiAvg = 0;
    char peer[20] = {0};
    /** Unix epoch at session end; 0 if NTP not synced */
    uint32_t endedUnix = 0;

    void toJson(JsonObject obj) const;
};

/** Persisted session archive, backed by LittleFS with NVS migration/fallback. */
class SessionArchive {
public:
    static constexpr uint8_t kMax = 64;

    void begin();
    bool push(const SessionSummary& s);
    void clear();
    uint8_t count() const { return count_; }
    uint32_t nextId() const { return nextId_; }
    /** newestIndex 0 = newest */
    bool get(uint8_t newestIndex, SessionSummary& out) const;
    void appendListJson(JsonArray arr, uint8_t limit = kMax) const;

private:
    void beginFs();
    void loadFsMeta();
    bool trimFsIfNeeded();
    bool appendFs(const SessionSummary& s);
    bool loadFsAt(uint32_t index, SessionSummary& out) const;

    void loadLegacyMeta();
    void saveLegacyMeta();
    bool loadLegacySlot(uint8_t slot, SessionSummary& out) const;
    bool saveLegacySlot(uint8_t slot, const SessionSummary& s);
    bool migrateLegacyToFs();

    uint8_t count_ = 0;
    uint8_t head_ = 0;  // next write slot
    uint32_t nextId_ = 1;
    bool fsReady_ = false;
};
