#include "BeatTimeline.h"

void BeatTimeline::begin() {
    reset();
}

void BeatTimeline::reset() {
    head_ = 0;
    count_ = 0;
    haveBeat_ = false;
    lastBeatTs_ = 0;
    lastRrMs_ = 0;
    lastPacketAt_ = 0;
    totalBeats_ = 0;
    currentGapMs_ = 0;
    longestGapMs_ = 0;
    notifGaps_ = 0;
    totalGapMs_ = 0;
    nValid_ = 0;
    nSuspect_ = 0;
    nArtifact_ = 0;
    nInvalid_ = 0;
    lastHr_ = 0;
    lastRrCount_ = 0;
}

RrQuality BeatTimeline::classify(uint16_t rrMs, uint16_t prevMs, bool havePrev) const {
    if (rrMs < 300 || rrMs > 2000) return RrQuality::Invalid;
    if (!havePrev || prevMs == 0) return RrQuality::Valid;

    int32_t d = (int32_t)rrMs - (int32_t)prevMs;
    if (d < 0) d = -d;
    float rel = (float)d / (float)prevMs;
    if (rel > 0.35f) return RrQuality::Artifact;
    if (rel > 0.20f || d > 120) return RrQuality::Suspect;
    return RrQuality::Valid;
}

void BeatTimeline::pushBeat(uint32_t beatTs, uint16_t rrRaw, RrQuality q) {
    BeatSample& b = beats_[head_];
    b.beatTsMs = beatTs;
    b.rrRaw = rrRaw;
    b.quality = q;
    b.usedForHrv = (q == RrQuality::Valid);
    head_ = (head_ + 1) % HR_BEAT_HISTORY;
    if (count_ < HR_BEAT_HISTORY) count_++;
    totalBeats_++;
    switch (q) {
        case RrQuality::Valid: nValid_++; break;
        case RrQuality::Suspect: nSuspect_++; break;
        case RrQuality::Artifact: nArtifact_++; break;
        default: nInvalid_++; break;
    }
}

void BeatTimeline::onPacket(const HrSample& packet) {
    const uint32_t T = packet.tMs ? packet.tMs : millis();
    lastHr_ = packet.heartRate;
    lastRrCount_ = packet.rrCount;
    for (uint8_t i = 0; i < packet.rrCount && i < 8; i++) {
        lastRrRaw_[i] = packet.rrRaw[i];
        lastQ_[i] = RrQuality::Invalid;
    }

    if (lastPacketAt_ > 0 && T >= lastPacketAt_) {
        currentGapMs_ = T - lastPacketAt_;
        if (currentGapMs_ > longestGapMs_) longestGapMs_ = currentGapMs_;
        if (currentGapMs_ > HR_MAX_ZONE_GAP_MS) {
            notifGaps_++;
            totalGapMs_ += currentGapMs_;
            haveBeat_ = false;  // re-anchor after large notification gap
        }
    } else {
        currentGapMs_ = 0;
    }
    lastPacketAt_ = T;

    if (packet.rrCount == 0) return;

    // Anchor so the last RR in this packet ends near packet receive time
    if (!haveBeat_) {
        uint32_t sumMs = 0;
        for (uint8_t i = 0; i < packet.rrCount; i++) sumMs += packet.rrMsAt(i);
        lastBeatTs_ = (T > sumMs) ? (T - sumMs) : 0;
        haveBeat_ = true;
        lastRrMs_ = 0;
    }

    for (uint8_t i = 0; i < packet.rrCount; i++) {
        uint16_t raw = packet.rrRaw[i];
        uint16_t ms = rrRawToMs(raw);
        bool havePrev = (lastRrMs_ > 0);
        RrQuality q = classify(ms, lastRrMs_, havePrev);
        lastQ_[i] = q;
        lastBeatTs_ += ms;
        pushBeat(lastBeatTs_, raw, q);
        if (q != RrQuality::Invalid) lastRrMs_ = ms;
    }
}

const BeatSample* BeatTimeline::lastBeat() const {
    if (count_ == 0) return nullptr;
    uint16_t idx = (head_ + HR_BEAT_HISTORY - 1) % HR_BEAT_HISTORY;
    return &beats_[idx];
}

uint8_t BeatTimeline::copyHrvRrMs(uint16_t* out, uint8_t maxCount) const {
    if (!out || maxCount == 0 || count_ == 0) return 0;
    // Collect valid RR from oldest→newest, keep only last maxCount
    uint16_t tmp[HR_BEAT_HISTORY];
    uint16_t n = 0;
    uint16_t start = (count_ == HR_BEAT_HISTORY) ? head_ : 0;
    for (uint16_t i = 0; i < count_; i++) {
        const BeatSample& b = beats_[(start + i) % HR_BEAT_HISTORY];
        if (!b.usedForHrv) continue;
        if (n < HR_BEAT_HISTORY) tmp[n++] = rrRawToMs(b.rrRaw);
    }
    if (n == 0) return 0;
    uint16_t take = n > maxCount ? maxCount : n;
    uint16_t src = n - take;
    for (uint16_t i = 0; i < take; i++) out[i] = tmp[src + i];
    return (uint8_t)take;
}

void BeatTimeline::appendStatusJson(JsonObject obj) const {
    obj["count"] = totalBeats_;
    obj["buffered"] = count_;
    obj["gapMs"] = currentGapMs_;
    obj["longestGapMs"] = longestGapMs_;
    obj["notifGaps"] = notifGaps_;
    obj["totalGapMs"] = totalGapMs_;
    obj["totalGapS"] = totalGapMs_ / 1000UL;
    uint32_t openGap = gapBlocksAccumulation() ? currentGapMs_ : 0;
    obj["openGapMs"] = openGap;
    obj["effectiveGapMs"] = totalGapMs_ + openGap;
    obj["gapBlocks"] = gapBlocksAccumulation();
    obj["valid"] = nValid_;
    obj["suspect"] = nSuspect_;
    obj["artifact"] = nArtifact_;
    obj["invalid"] = nInvalid_;
    uint32_t labeled = nValid_ + nSuspect_ + nArtifact_ + nInvalid_;
    obj["validPct"] = labeled ? (double)((int)(1000.0 * nValid_ / labeled + 0.5) / 10.0) : 0;
    obj["artifactPct"] = labeled ? (double)((int)(1000.0 * nArtifact_ / labeled + 0.5) / 10.0) : 0;
    const BeatSample* last = lastBeat();
    if (last) {
        obj["lastRrRaw"] = last->rrRaw;
        obj["lastRrMs"] = rrRawToMs(last->rrRaw);
        obj["lastQuality"] = rrQualityName(last->quality);
        obj["lastBeatTs"] = last->beatTsMs;
    }
}

void BeatTimeline::appendLastPacketJson(JsonObject obj) const {
    obj["hr"] = lastHr_;
    obj["rrCount"] = lastRrCount_;
    JsonArray raw = obj["rrRaw"].to<JsonArray>();
    JsonArray ms = obj["rrMs"].to<JsonArray>();
    JsonArray q = obj["quality"].to<JsonArray>();
    for (uint8_t i = 0; i < lastRrCount_; i++) {
        raw.add(lastRrRaw_[i]);
        ms.add(rrRawToMs(lastRrRaw_[i]));
        q.add(rrQualityName(lastQ_[i]));
    }
}

void BeatTimeline::appendIoValues(JsonObject ios) const {
    auto addN = [&](const char* key, float value, const char* unit) {
        JsonObject o = ios[key].to<JsonObject>();
        o["type"] = "sensor";
        o["value"] = value;
        o["unit"] = unit;
    };
    auto addS = [&](const char* key, const char* value) {
        JsonObject o = ios[key].to<JsonObject>();
        o["type"] = "sensor";
        o["value"] = value;
        o["unit"] = "";
    };
    uint32_t labeled = nValid_ + nSuspect_ + nArtifact_ + nInvalid_;
    addN("rr_valid_pct", labeled ? (100.0f * nValid_ / labeled) : 0, "%");
    addN("rr_artifact_pct", labeled ? (100.0f * nArtifact_ / labeled) : 0, "%");
    addN("ble_gap_ms", (float)currentGapMs_, "ms");
    addN("ble_longest_gap_ms", (float)longestGapMs_, "ms");
    addN("ble_notif_gaps", (float)notifGaps_, "");
    const BeatSample* last = lastBeat();
    addS("rr_quality", last ? rrQualityName(last->quality) : "-");
}
