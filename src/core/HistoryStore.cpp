#include "HistoryStore.h"

void HistoryStore::begin() {
    for (uint8_t i = 0; i < METRIC_COUNT; i++) {
        series_[i].head = 0;
        series_[i].count = 0;
    }
    rr_.head = 0;
    rr_.count = 0;
}

void HistoryStore::push(Metric metric, float value) {
    if (metric >= METRIC_COUNT) return;
    Series& s = series_[metric];
    s.values[s.head] = value;
    s.ts[s.head] = millis() / 1000UL;
    s.head = (s.head + 1) % HR_HISTORY_SIZE;
    if (s.count < HR_HISTORY_SIZE) s.count++;
}

void HistoryStore::pushRr(float rrMs) {
    rr_.values[rr_.head] = rrMs;
    rr_.ts[rr_.head] = millis() / 1000UL;
    rr_.head = (rr_.head + 1) % HR_RR_HISTORY_SIZE;
    if (rr_.count < HR_RR_HISTORY_SIZE) rr_.count++;
}

size_t HistoryStore::size(Metric metric) const {
    if (metric >= METRIC_COUNT) return 0;
    return series_[metric].count;
}

size_t HistoryStore::rrSize() const {
    return rr_.count;
}

float HistoryStore::last(Metric metric, float fallback) const {
    if (metric >= METRIC_COUNT || series_[metric].count == 0) return fallback;
    size_t idx = (series_[metric].head + HR_HISTORY_SIZE - 1) % HR_HISTORY_SIZE;
    return series_[metric].values[idx];
}

float HistoryStore::lastRr(float fallback) const {
    if (rr_.count == 0) return fallback;
    size_t idx = (rr_.head + HR_RR_HISTORY_SIZE - 1) % HR_RR_HISTORY_SIZE;
    return rr_.values[idx];
}

void HistoryStore::toJson(Metric metric, JsonArray arr, size_t maxPoints) const {
    if (metric >= METRIC_COUNT) return;
    const Series& s = series_[metric];
    size_t n = s.count < maxPoints ? s.count : maxPoints;
    size_t start = s.count > n ? s.count - n : 0;
    for (size_t i = start; i < s.count; i++) {
        size_t idx = (s.head + HR_HISTORY_SIZE - s.count + i) % HR_HISTORY_SIZE;
        JsonObject o = arr.add<JsonObject>();
        o["ts"] = s.ts[idx];
        o["v"] = s.values[idx];
    }
}

void HistoryStore::rrToJson(JsonArray arr, size_t maxPoints) const {
    size_t n = rr_.count < maxPoints ? rr_.count : maxPoints;
    size_t start = rr_.count > n ? rr_.count - n : 0;
    for (size_t i = start; i < rr_.count; i++) {
        size_t idx = (rr_.head + HR_RR_HISTORY_SIZE - rr_.count + i) % HR_RR_HISTORY_SIZE;
        JsonObject o = arr.add<JsonObject>();
        o["ts"] = rr_.ts[idx];
        o["v"] = rr_.values[idx];
    }
}
