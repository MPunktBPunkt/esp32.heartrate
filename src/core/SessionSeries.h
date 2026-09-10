#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * Downsampled HR series for long sessions (bike 1–2h).
 * Base interval 10s → up to ~2h in 720 points; longer rides auto-thin.
 */
class SessionSeries {
public:
    static constexpr uint16_t kMax = 720;
    static constexpr uint32_t kBaseIntervalMs = 10000UL;

    void begin();
    void reset();
    void onSample(uint16_t hr, uint32_t sessionDurationMs);
    uint16_t count() const { return count_; }
    uint32_t intervalMs() const { return intervalMs_; }

    void appendHrJson(JsonArray arr) const;
    void appendCsv(String& out) const;

private:
    struct Point {
        uint16_t tS = 0;   // seconds from session start
        uint16_t hr = 0;
    };

    Point pts_[kMax];
    uint16_t count_ = 0;
    uint32_t lastPushMs_ = 0;
    uint32_t intervalMs_ = kBaseIntervalMs;
    bool active_ = false;

    void pushPoint(uint16_t tS, uint16_t hr);
    void recomputeInterval(uint32_t sessionDurationMs);
};
