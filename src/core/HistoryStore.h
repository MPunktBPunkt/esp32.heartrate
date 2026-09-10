#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "BuildFlags.h"

class HistoryStore {
public:
    enum Metric : uint8_t {
        HR_BPM = 0,
        BATTERY = 1,
        RSSI = 2,
        METRIC_COUNT = 3
    };

    void begin();
    void push(Metric metric, float value);
    void pushRr(float rrMs);
    size_t size(Metric metric) const;
    size_t rrSize() const;
    void toJson(Metric metric, JsonArray arr, size_t maxPoints = HR_HISTORY_SIZE) const;
    void rrToJson(JsonArray arr, size_t maxPoints = HR_RR_HISTORY_SIZE) const;
    float last(Metric metric, float fallback = 0) const;
    float lastRr(float fallback = 0) const;

private:
    struct Series {
        float values[HR_HISTORY_SIZE];
        uint32_t ts[HR_HISTORY_SIZE];
        size_t head = 0;
        size_t count = 0;
    };
    struct RrSeries {
        float values[HR_RR_HISTORY_SIZE];
        uint32_t ts[HR_RR_HISTORY_SIZE];
        size_t head = 0;
        size_t count = 0;
    };

    Series series_[METRIC_COUNT];
    RrSeries rr_;
};
