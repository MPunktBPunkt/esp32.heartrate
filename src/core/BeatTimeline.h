#pragma once

#include "ble/BleTypes.h"
#include "BuildFlags.h"
#include <ArduinoJson.h>

struct BeatSample {
    uint32_t beatTsMs = 0;
    uint16_t rrRaw = 0;
    RrQuality quality = RrQuality::Invalid;
    bool usedForHrv = false;
};

/**
 * Packet-spanning beat timeline from H9 RR intervals.
 * Keeps original rrRaw; quality is labeled separately.
 */
class BeatTimeline {
public:
    void begin();
    void reset();
    void onPacket(const HrSample& packet);

    uint32_t beatCount() const { return totalBeats_; }
    uint32_t currentGapMs() const { return currentGapMs_; }
    uint32_t longestGapMs() const { return longestGapMs_; }
    uint32_t notifGaps() const { return notifGaps_; }
    uint32_t totalGapMs() const { return totalGapMs_; }
    uint32_t openGapMs() const { return gapBlocksAccumulation() ? currentGapMs_ : 0; }
    uint32_t effectiveGapMs() const { return totalGapMs_ + openGapMs(); }
    bool gapBlocksAccumulation() const { return currentGapMs_ > HR_MAX_ZONE_GAP_MS; }
    float validPct() const {
        uint32_t labeled = nValid_ + nSuspect_ + nArtifact_ + nInvalid_;
        return labeled ? (100.0f * nValid_ / labeled) : 0.0f;
    }

    const BeatSample* lastBeat() const;
    /** Copy up to maxCount most-recent HRV-eligible RR intervals (ms), oldest→newest. */
    uint8_t copyHrvRrMs(uint16_t* out, uint8_t maxCount) const;

    void appendStatusJson(JsonObject obj) const;
    void appendLastPacketJson(JsonObject obj) const;
    void appendIoValues(JsonObject ios) const;

private:
    RrQuality classify(uint16_t rrMs, uint16_t prevMs, bool havePrev) const;
    void pushBeat(uint32_t beatTs, uint16_t rrRaw, RrQuality q);

    BeatSample beats_[HR_BEAT_HISTORY];
    uint16_t head_ = 0;
    uint16_t count_ = 0;

    bool haveBeat_ = false;
    uint32_t lastBeatTs_ = 0;
    uint16_t lastRrMs_ = 0;
    uint32_t lastPacketAt_ = 0;

    uint32_t totalBeats_ = 0;
    uint32_t currentGapMs_ = 0;
    uint32_t longestGapMs_ = 0;
    uint32_t notifGaps_ = 0;
    uint32_t totalGapMs_ = 0;  // sum of notification gaps > maxZoneGap

    uint32_t nValid_ = 0;
    uint32_t nSuspect_ = 0;
    uint32_t nArtifact_ = 0;
    uint32_t nInvalid_ = 0;

    // last packet debug snapshot
    uint16_t lastHr_ = 0;
    uint8_t lastRrCount_ = 0;
    uint16_t lastRrRaw_[8] = {0};
    RrQuality lastQ_[8] = {};
};
