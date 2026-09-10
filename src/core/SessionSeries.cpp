#include "SessionSeries.h"

void SessionSeries::begin() {
    reset();
}

void SessionSeries::reset() {
    count_ = 0;
    lastPushMs_ = 0;
    intervalMs_ = kBaseIntervalMs;
    active_ = true;
}

void SessionSeries::recomputeInterval(uint32_t sessionDurationMs) {
    uint32_t want = kBaseIntervalMs;
    if (sessionDurationMs > (uint32_t)kMax * kBaseIntervalMs) {
        want = sessionDurationMs / kMax;
        if (want < kBaseIntervalMs) want = kBaseIntervalMs;
        if (want > 60000UL) want = 60000UL;
    }
    intervalMs_ = want;
}

void SessionSeries::pushPoint(uint16_t tS, uint16_t hr) {
    if (count_ >= kMax) {
        // Thin in place: keep every 2nd sample
        uint16_t w = 0;
        for (uint16_t i = 0; i < count_; i += 2) {
            pts_[w++] = pts_[i];
        }
        count_ = w;
        if (intervalMs_ < 60000UL) intervalMs_ *= 2;
    }
    if (count_ >= kMax) return;
    pts_[count_].tS = tS;
    pts_[count_].hr = hr;
    count_++;
}

void SessionSeries::onSample(uint16_t hr, uint32_t sessionDurationMs) {
    if (!active_ || !hr) return;
    recomputeInterval(sessionDurationMs);
    if (lastPushMs_ && (sessionDurationMs - lastPushMs_) < intervalMs_) return;
    lastPushMs_ = sessionDurationMs ? sessionDurationMs : 1;
    uint32_t sec = sessionDurationMs / 1000UL;
    uint16_t tS = sec > 65535UL ? 65535 : (uint16_t)sec;
    pushPoint(tS, hr);
}

void SessionSeries::appendHrJson(JsonArray arr) const {
    for (uint16_t i = 0; i < count_; i++) {
        JsonObject o = arr.add<JsonObject>();
        o["t"] = pts_[i].tS;
        o["hr"] = pts_[i].hr;
    }
}

void SessionSeries::appendCsv(String& out) const {
    out += F("t_s,hr\n");
    for (uint16_t i = 0; i < count_; i++) {
        out += String(pts_[i].tS);
        out += ',';
        out += String(pts_[i].hr);
        out += '\n';
    }
}
