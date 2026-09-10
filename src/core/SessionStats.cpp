#include "SessionStats.h"
#include <math.h>

void SessionStats::begin(ConfigStore* cfg) {
    cfg_ = cfg;
    reset();
}

void SessionStats::clearRecovery() {
    recoveryActive_ = false;
    recoveryStartedMs_ = 0;
    recoveryStartHr_ = 0;
    hrAt60_ = 0;
    hrAt120_ = 0;
    hrAt300_ = 0;
    have60_ = false;
    have120_ = false;
    have300_ = false;
}

void SessionStats::resetRestAccum() {
    restSampleCount_ = 0;
    restHrSum_ = 0;
    restHrMin_ = 0;
    liveRestHr_ = 0;
    restWinCount_ = 0;
    restWinIdx_ = 0;
    baselineAutoSaved_ = false;
    lastCaptureAuto_ = false;
    for (uint8_t i = 0; i < kRestWin; i++) restWin_[i] = 0;
}

void SessionStats::clearGuided() {
    guidedActive_ = false;
    guidePhase_ = GuidePhase::Idle;
    guideStartedMs_ = 0;
    guideElevateSinceMs_ = 0;
    guideMsg_ = "";
}

const char* SessionStats::guidePhaseName(GuidePhase p) {
    switch (p) {
        case GuidePhase::Settle: return "SETTLE";
        case GuidePhase::Measure: return "MEASURE";
        case GuidePhase::Done: return "DONE";
        case GuidePhase::Failed: return "FAILED";
        default: return "IDLE";
    }
}

void SessionStats::reset() {
    active_ = false;
    mode_ = SessionMode::Idle;
    startedMs_ = 0;
    endedMs_ = 0;
    lastSampleMs_ = 0;
    modeStartedMs_ = 0;
    sampleCount_ = 0;
    lastHr_ = 0;
    hrMin_ = 0;
    hrMax_ = 0;
    hrSum_ = 0;
    lastRr_ = 0;
    hrFromRr_ = 0;
    rrCount_ = 0;
    rmssd_ = 0;
    sdnn_ = 0;
    pnn50_ = 0;
    sdsd_ = 0;
    hrvValidity_ = "INVALID";
    calories_ = 0;
    zone_ = 0;
    pctMax_ = 0;
    pctHrr_ = 0;
    for (uint8_t i = 0; i < 6; i++) zoneTimeMs_[i] = 0;
    clearRecovery();
    resetRestAccum();
    clearGuided();
    suggestedMode_ = SessionMode::Idle;
    elevatedSinceMs_ = 0;
    calmSinceMs_ = 0;
    autoSwitchedToActivity_ = false;
    autoSwitchedToRest_ = false;
}

void SessionStats::setSessionActive(bool active) {
    if (active && !active_) {
        sampleCount_ = 0;
        lastHr_ = 0;
        hrMin_ = 0;
        hrMax_ = 0;
        hrSum_ = 0;
        rrCount_ = 0;
        rmssd_ = 0;
        sdnn_ = 0;
        pnn50_ = 0;
        sdsd_ = 0;
        hrvValidity_ = "INVALID";
        calories_ = 0;
        zone_ = 0;
        pctMax_ = 0;
        pctHrr_ = 0;
        lastRr_ = 0;
        hrFromRr_ = 0;
        for (uint8_t i = 0; i < 6; i++) zoneTimeMs_[i] = 0;
        clearRecovery();
        resetRestAccum();
        clearGuided();
        suggestedMode_ = SessionMode::Idle;
        elevatedSinceMs_ = 0;
        calmSinceMs_ = 0;
        autoSwitchedToActivity_ = false;
        autoSwitchedToRest_ = false;
        baselineRefined_ = false;
        lastRefineMs_ = 0;
        startedMs_ = millis();
        endedMs_ = 0;
        lastSampleMs_ = 0;
        active_ = true;
        // Known resting HR → Alltag; otherwise Rest to capture baseline first
        mode_ = (cfg_ && cfg_->restingHr > 0) ? SessionMode::Activity : SessionMode::Rest;
        modeStartedMs_ = startedMs_;
        suggestedMode_ = mode_;
    } else if (!active && active_) {
        active_ = false;
        endedMs_ = millis();
        // keep mode/recovery/baseline snapshot for UI until next connect
    }
}

bool SessionStats::setMode(SessionMode mode) {
    if (mode == mode_) return true;
    SessionMode prev = mode_;
    if (guidedActive_ && mode != SessionMode::Rest &&
        (guidePhase_ == GuidePhase::Settle || guidePhase_ == GuidePhase::Measure)) {
        guidedActive_ = false;
        guidePhase_ = GuidePhase::Failed;
        guideStartedMs_ = 0;
        guideElevateSinceMs_ = 0;
        guideMsg_ = "abgebrochen (Moduswechsel)";
    }
    mode_ = mode;
    modeStartedMs_ = millis();
    elevatedSinceMs_ = 0;
    calmSinceMs_ = 0;
    suggestedMode_ = mode;

    if (mode == SessionMode::Rest) {
        resetRestAccum();
        autoSwitchedToActivity_ = false;
    }

    if (mode == SessionMode::Recovery) {
        recoveryActive_ = true;
        recoveryStartedMs_ = modeStartedMs_;
        recoveryStartHr_ = lastHr_ ? lastHr_ : (sampleCount_ ? hrMax_ : 0);
        hrAt60_ = 0;
        hrAt120_ = 0;
        hrAt300_ = 0;
        have60_ = false;
        have120_ = false;
        have300_ = false;
        (void)prev;
    } else if (prev == SessionMode::Recovery && mode != SessionMode::Recovery) {
        recoveryActive_ = false;
    }
    return true;
}

void SessionStats::updateRecovery(uint16_t hr, unsigned long now) {
    if (!recoveryActive_ || !recoveryStartedMs_ || !hr) return;
    unsigned long elapsed = now - recoveryStartedMs_;
    if (!have60_ && elapsed >= 60000UL) {
        hrAt60_ = hr;
        have60_ = true;
    }
    if (!have120_ && elapsed >= 120000UL) {
        hrAt120_ = hr;
        have120_ = true;
    }
    if (!have300_ && elapsed >= 300000UL) {
        hrAt300_ = hr;
        have300_ = true;
    }
}

bool SessionStats::restStable() const {
    if (restWinCount_ < kRestWin) return false;
    double sum = 0;
    for (uint8_t i = 0; i < kRestWin; i++) sum += restWin_[i];
    double mean = sum / kRestWin;
    double var = 0;
    for (uint8_t i = 0; i < kRestWin; i++) {
        double d = restWin_[i] - mean;
        var += d * d;
    }
    float std = (float)sqrt(var / kRestWin);
    return std <= kRestStableStd;
}

bool SessionStats::restQuietEnough() const {
    if (!restStable() || !liveRestHr_ || !restHrMin_) return false;
    // Reject "stable but elevated" housework baselines
    if (liveRestHr_ > restHrMin_ + 8) return false;
    if (cfg_ && cfg_->restingHr && liveRestHr_ > cfg_->restingHr + 6) return false;
    return true;
}

void SessionStats::updateRestBaseline(uint16_t hr, unsigned long now) {
    if (mode_ != SessionMode::Rest || !hr || !modeStartedMs_) return;
    unsigned long elapsed = now - modeStartedMs_;
    if (elapsed < kRestWarmupMs) return;

    // During guided settle, do not accumulate yet (warmup covers settle when guide starts Rest)
    if (guidedActive_ && guidePhase_ == GuidePhase::Settle) return;

    restHrSum_ += hr;
    restSampleCount_++;
    if (!restHrMin_ || hr < restHrMin_) restHrMin_ = hr;
    liveRestHr_ = (uint16_t)(restHrSum_ / restSampleCount_ + 0.5);

    restWin_[restWinIdx_] = hr;
    restWinIdx_ = (restWinIdx_ + 1) % kRestWin;
    if (restWinCount_ < kRestWin) restWinCount_++;

    // First save if no RHR, else only refine downward when clearly lower
    if (guidedActive_ || elapsed < kRestMinCaptureMs || restSampleCount_ < 60 || !restQuietEnough()) return;
    if (!cfg_) return;
    if (!cfg_->restingHr) {
        if (!baselineAutoSaved_) captureBaseline(true);
        return;
    }
    if (liveRestHr_ + kRefineMinDropBpm <= cfg_->restingHr) {
        if (lastRefineMs_ && (now - lastRefineMs_) < kRefineCooldownMs) return;
        uint8_t before = cfg_->restingHr;
        if (captureBaseline(true)) {
            lastRefineMs_ = now;
            baselineRefined_ = true;
            Serial.printf("[BASE] refined restingHr %u -> %u\n", (unsigned)before,
                          (unsigned)cfg_->restingHr);
        }
    }
}

void SessionStats::updateModeHint(uint16_t hr, unsigned long now) {
    suggestedMode_ = mode_;
    if (!cfg_ || !cfg_->restingHr || !hr || !active_) return;
    if (mode_ == SessionMode::Recovery || mode_ == SessionMode::Idle) return;
    // Never auto-leave Rest during guided baseline
    if (guidedActive_ && (guidePhase_ == GuidePhase::Settle || guidePhase_ == GuidePhase::Measure)) return;

    const uint16_t restRef = restRefForHints();
    if (!restRef) return;
    const uint16_t elevateAt = (uint16_t)restRef + kElevateAboveRest;
    const uint16_t calmAt = (uint16_t)restRef + kCalmAboveRest;

    if (hr >= elevateAt) {
        if (!elevatedSinceMs_) elevatedSinceMs_ = now;
        calmSinceMs_ = 0;
    } else if (hr <= calmAt) {
        if (!calmSinceMs_) calmSinceMs_ = now;
        elevatedSinceMs_ = 0;
    } else {
        elevatedSinceMs_ = 0;
        calmSinceMs_ = 0;
    }

    if (mode_ == SessionMode::Rest && elevatedSinceMs_ && (now - elevatedSinceMs_) >= kModeHintElevateMs) {
        suggestedMode_ = SessionMode::Activity;
        if (!autoSwitchedToActivity_) {
            setMode(SessionMode::Activity);
            autoSwitchedToActivity_ = true;
            suggestedMode_ = SessionMode::Activity;
            Serial.println("[MODE] soft auto → ACTIVITY");
        }
    } else if (mode_ == SessionMode::Activity && calmSinceMs_ &&
               (now - calmSinceMs_) >= kModeHintCalmMs) {
        suggestedMode_ = SessionMode::Rest;
        if (!autoSwitchedToRest_) {
            setMode(SessionMode::Rest);
            autoSwitchedToRest_ = true;
            suggestedMode_ = SessionMode::Rest;
            Serial.println("[MODE] soft auto → REST (baseline refine window)");
        }
    } else if (mode_ == SessionMode::Training && calmSinceMs_ &&
               (now - calmSinceMs_) >= kModeHintCalmMs) {
        suggestedMode_ = SessionMode::Rest;
    }
}

uint16_t SessionStats::restRefForHints() const {
    if (!cfg_ || !cfg_->restingHr) return 0;
    uint16_t rest = cfg_->restingHr;
    // Stale-high RHR: use observed session floor so soft-auto still reacts
    if (sampleCount_ >= 90 && hrMin_ >= 40 && hrMin_ + 10 < rest) {
        uint16_t adapted = (uint16_t)(hrMin_ + 5);
        if (adapted < rest) rest = adapted;
    }
    return rest;
}

bool SessionStats::startGuidedBaseline() {
    if (!active_) return false;
    // Force Rest and fresh rest window
    if (mode_ != SessionMode::Rest) {
        setMode(SessionMode::Rest);
    } else {
        resetRestAccum();
        modeStartedMs_ = millis();
        autoSwitchedToActivity_ = false;
    }
    guidedActive_ = true;
    guidePhase_ = GuidePhase::Settle;
    guideStartedMs_ = millis();
    guideElevateSinceMs_ = 0;
    guideMsg_ = "Warmup — still sitzen/liegen";
    Serial.println("[BASE] guided start (4 min)");
    return true;
}

void SessionStats::cancelGuidedBaseline() {
    if (!guidedActive_ && guidePhase_ != GuidePhase::Settle && guidePhase_ != GuidePhase::Measure) {
        clearGuided();
        return;
    }
    guidedActive_ = false;
    guidePhase_ = GuidePhase::Failed;
    guideElevateSinceMs_ = 0;
    guideMsg_ = "abgebrochen";
    Serial.println("[BASE] guided cancel");
}

void SessionStats::updateGuidedBaseline(uint16_t hr, unsigned long now) {
    if (!guidedActive_) return;
    if (guidePhase_ != GuidePhase::Settle && guidePhase_ != GuidePhase::Measure) return;
    if (!guideStartedMs_) return;

    unsigned long elapsed = now - guideStartedMs_;
    const unsigned long totalTarget = kGuideSettleMs + kGuideMeasureMs;

    if (guidePhase_ == GuidePhase::Settle) {
        guideMsg_ = "Warmup — still sitzen/liegen";
        if (elapsed >= kGuideSettleMs) {
            guidePhase_ = GuidePhase::Measure;
            guideMsg_ = "Messung — ruhig bleiben";
            guideElevateSinceMs_ = 0;
            // modeStartedMs already aligned; rest accum starts via warmup gate
        }
        return;
    }

    // Measure phase: detect sustained elevation → fail
    uint16_t elevateRef = restHrMin_ ? restHrMin_ : (cfg_ && cfg_->restingHr ? cfg_->restingHr : 0);
    if (elevateRef && hr >= elevateRef + 15) {
        if (!guideElevateSinceMs_) guideElevateSinceMs_ = now;
        else if (now - guideElevateSinceMs_ >= 20000UL) {
            guidedActive_ = false;
            guidePhase_ = GuidePhase::Failed;
            guideMsg_ = "zu unruhig — bitte erneut starten";
            Serial.println("[BASE] guided failed: elevated");
            return;
        }
    } else {
        guideElevateSinceMs_ = 0;
    }

    guideMsg_ = "Messung — ruhig bleiben";

    if (elapsed >= totalTarget) {
        if (restQuietEnough() && restSampleCount_ >= 40) {
            if (captureBaseline(true)) {
                guidedActive_ = false;
                guidePhase_ = GuidePhase::Done;
                guideMsg_ = "Baseline gespeichert";
                Serial.printf("[BASE] guided done restingHr=%u\n", cfg_ ? cfg_->restingHr : 0);
                return;
            }
        }
        // Grace: wait up to +60s for quiet window
        if (elapsed < totalTarget + kGuideGraceMs) {
            guideMsg_ = "warte auf stabile Ruhe…";
            return;
        }
        guidedActive_ = false;
        guidePhase_ = GuidePhase::Failed;
        guideMsg_ = "nicht ruhig genug — erneut versuchen";
        Serial.println("[BASE] guided failed: not quiet");
    }
}

bool SessionStats::captureBaseline(bool autoSave) {
    if (!cfg_) return false;
    uint16_t candidate = liveRestHr_;
    if (!candidate && restSampleCount_) candidate = (uint16_t)(restHrSum_ / restSampleCount_ + 0.5);
    if (!candidate && lastHr_) candidate = lastHr_;
    if (!candidate) return false;
    if (candidate < 30) candidate = 30;
    if (candidate > 120) candidate = 120;

    // Prefer slightly lower of avg vs recent min if min is close (quiet floor)
    if (restHrMin_ >= 30 && restHrMin_ <= candidate && (candidate - restHrMin_) <= 6) {
        candidate = (uint16_t)((candidate + restHrMin_) / 2);
    }

    cfg_->restingHr = (uint8_t)candidate;
    cfg_->save();
    baselineAutoSaved_ = true;
    lastCaptureAuto_ = autoSave;
    return true;
}

void SessionStats::clearBaseline() {
    if (!cfg_) return;
    cfg_->restingHr = 0;
    cfg_->save();
    baselineAutoSaved_ = false;
    lastCaptureAuto_ = false;
}

bool SessionStats::buildSummary(SessionSummary& out) const {
    if (sampleCount_ < 30) return false;
    unsigned long end = endedMs_ ? endedMs_ : millis();
    if (!startedMs_ || end <= startedMs_) return false;
    uint32_t durS = (uint32_t)((end - startedMs_) / 1000UL);
    if (durS < 60) return false;

    out = SessionSummary{};
    out.durationS = durS;
    out.samples = sampleCount_;
    out.hrAvg = (uint16_t)(hrSum_ / sampleCount_ + 0.5);
    out.hrMin = hrMin_;
    out.hrMax = hrMax_;
    out.restingHr = cfg_ ? cfg_->restingHr : 0;
    out.rmssd = rmssd_;
    out.sdnn = sdnn_;
    out.pnn50 = pnn50_;
    out.calories = calories_;
    for (uint8_t z = 0; z < 5; z++) {
        out.zoneS[z] = (uint16_t)(zoneTimeMs_[z + 1] / 1000UL);
        if (out.zoneS[z] > 65535) out.zoneS[z] = 65535;
    }
    out.hrr1 = have60_ ? (int16_t)((int)recoveryStartHr_ - (int)hrAt60_) : 0;
    out.hrr2 = have120_ ? (int16_t)((int)recoveryStartHr_ - (int)hrAt120_) : 0;
    out.hrr5 = have300_ ? (int16_t)((int)recoveryStartHr_ - (int)hrAt300_) : 0;
    out.endMode = (uint8_t)mode_;
    out.flags = 0;
    if (have60_ || have120_ || have300_) out.flags |= 0x01;
    return true;
}

void SessionStats::recomputeHrv(const BeatTimeline& beats) {
    rrCount_ = beats.copyHrvRrMs(rrWin_, kRrWindow);
    if (rrCount_ < 2) {
        rmssd_ = 0;
        sdnn_ = 0;
        pnn50_ = 0;
        sdsd_ = 0;
        hrvValidity_ = rrCount_ ? "LIMITED" : "INVALID";
        return;
    }

    lastRr_ = rrWin_[rrCount_ - 1];
    hrFromRr_ = (uint16_t)((60000UL + lastRr_ / 2) / lastRr_);

    double sum = 0;
    for (uint8_t i = 0; i < rrCount_; i++) sum += rrWin_[i];
    double mean = sum / rrCount_;

    double var = 0;
    for (uint8_t i = 0; i < rrCount_; i++) {
        double d = rrWin_[i] - mean;
        var += d * d;
    }
    sdnn_ = (float)sqrt(var / rrCount_);

    double sumSqDiff = 0;
    double sumDiff = 0;
    double sumDiffSqDev = 0;
    uint16_t nn50 = 0;
    uint16_t diffs = rrCount_ - 1;
    for (uint8_t i = 1; i < rrCount_; i++) {
        double d = (double)rrWin_[i] - (double)rrWin_[i - 1];
        sumSqDiff += d * d;
        sumDiff += d;
        double ad = d < 0 ? -d : d;
        if (ad > 50) nn50++;
    }
    rmssd_ = (float)sqrt(sumSqDiff / diffs);
    double meanDiff = sumDiff / diffs;
    for (uint8_t i = 1; i < rrCount_; i++) {
        double d = (double)rrWin_[i] - (double)rrWin_[i - 1] - meanDiff;
        sumDiffSqDev += d * d;
    }
    sdsd_ = (float)sqrt(sumDiffSqDev / diffs);
    pnn50_ = 100.0f * nn50 / diffs;

    // Activity/Training: HRV limited even with enough samples
    if (sessionModeLimitsHrv(mode_)) {
        hrvValidity_ = rrCount_ >= 10 ? "LIMITED" : "INVALID";
    } else if (rrCount_ >= 40) {
        hrvValidity_ = "VALID";
    } else if (rrCount_ >= 10) {
        hrvValidity_ = "LIMITED";
    } else {
        hrvValidity_ = "INVALID";
    }

    if (beats.gapBlocksAccumulation() && rrCount_ < 40) hrvValidity_ = "LIMITED";
}

uint16_t SessionStats::hrMaxBpm() const {
    if (!cfg_ || cfg_->userAge == 0) return 0;
    int m = (int)(208.0f - 0.7f * (float)cfg_->userAge + 0.5f);
    if (m < 100) m = 100;
    if (m > 220) m = 220;
    return (uint16_t)m;
}

float SessionStats::pctHrr(uint16_t hr) const {
    if (!cfg_ || !cfg_->restingHr || !hr) return 0;
    uint16_t mx = hrMaxBpm();
    if (!mx || mx <= cfg_->restingHr) return 0;
    float n = (float)hr - (float)cfg_->restingHr;
    float d = (float)mx - (float)cfg_->restingHr;
    float pct = 100.0f * n / d;
    if (pct < 0) pct = 0;
    if (pct > 150) pct = 150;
    return pct;
}

uint8_t SessionStats::zoneFor(uint16_t hr) const {
    uint16_t mx = hrMaxBpm();
    if (!mx || !hr) return 0;

    float pct;
    if (cfg_ && cfg_->zonesUseHrr()) {
        // Karvonen: %HRR = (HR − RHR) / (HRmax − RHR)
        pct = pctHrr(hr);
        if (pct <= 0 && hr <= cfg_->restingHr) return 1;
    } else {
        pct = 100.0f * hr / mx;
    }

    if (pct < 60) return 1;
    if (pct < 70) return 2;
    if (pct < 80) return 3;
    if (pct < 90) return 4;
    return 5;
}

float SessionStats::caloriesDelta(uint16_t hr, float dtS) const {
    if (!cfg_ || cfg_->userWeightKg == 0 || cfg_->userAge == 0 || hr < 40 || dtS <= 0) return 0;
    float age = (float)cfg_->userAge;
    float w = (float)cfg_->userWeightKg;
    float h = (float)hr;
    float kcalPerMin;
    if (cfg_->userFemale) {
        kcalPerMin = (-20.4022f + 0.4472f * h - 0.1263f * w + 0.074f * age) / 4.184f;
    } else {
        kcalPerMin = (-55.0969f + 0.6309f * h + 0.1988f * w + 0.2017f * age) / 4.184f;
    }
    if (kcalPerMin < 0) kcalPerMin = 0;
    return kcalPerMin * (dtS / 60.0f);
}

void SessionStats::onSample(const HrSample& s, const BeatTimeline& beats) {
    if (!active_) return;
    unsigned long now = s.tMs ? s.tMs : millis();
    uint8_t prevZone = zone_;
    lastHr_ = s.heartRate;

    if (sampleCount_ == 0) {
        hrMin_ = s.heartRate;
        hrMax_ = s.heartRate;
        startedMs_ = now;
    } else {
        if (s.heartRate < hrMin_) hrMin_ = s.heartRate;
        if (s.heartRate > hrMax_) hrMax_ = s.heartRate;
        if (lastSampleMs_ && !beats.gapBlocksAccumulation()) {
            float dt = (now - lastSampleMs_) / 1000.0f;
            if (dt > 0 && dt < 10.0f) {
                if (sessionModeCountsCalories(mode_)) {
                    calories_ += caloriesDelta(s.heartRate, dt);
                }
                if (sessionModeCountsZones(mode_) && prevZone >= 1 && prevZone <= 5) {
                    zoneTimeMs_[prevZone] += (uint32_t)(now - lastSampleMs_);
                }
            }
        }
    }
    hrSum_ += s.heartRate;
    sampleCount_++;
    lastSampleMs_ = now;

    recomputeHrv(beats);
    updateRecovery(s.heartRate, now);
    updateRestBaseline(s.heartRate, now);
    updateGuidedBaseline(s.heartRate, now);
    updateModeHint(s.heartRate, now);

    uint16_t mx = hrMaxBpm();
    if (mx) {
        pctMax_ = 100.0f * s.heartRate / mx;
        zone_ = zoneFor(s.heartRate);
    } else {
        pctMax_ = 0;
        zone_ = 0;
    }
    pctHrr_ = pctHrr(s.heartRate);
}

void SessionStats::appendJson(JsonObject obj) const {
    obj["active"] = active_;
    obj["mode"] = sessionModeName(mode_);
    obj["suggestedMode"] = sessionModeName(suggestedMode_);
    obj["modeFit"] = (suggestedMode_ == mode_);
    obj["autoSwitched"] = autoSwitchedToActivity_;
    obj["autoSwitchedRest"] = autoSwitchedToRest_;
    obj["restRefHint"] = restRefForHints();
    unsigned long durMs = 0;
    if (startedMs_) {
        unsigned long end = active_ ? millis() : (endedMs_ ? endedMs_ : millis());
        durMs = end - startedMs_;
    }
    obj["durationS"] = durMs / 1000UL;
    obj["modeDurationS"] = modeStartedMs_ ? ((active_ ? millis() : (endedMs_ ? endedMs_ : millis())) - modeStartedMs_) / 1000UL : 0;
    obj["samples"] = sampleCount_;
    obj["hrMin"] = hrMin_;
    obj["hrMax"] = hrMax_;
    obj["hrAvg"] = sampleCount_ ? (uint16_t)(hrSum_ / sampleCount_ + 0.5) : 0;
    obj["hrFromRr"] = hrFromRr_;
    obj["lastRr"] = lastRr_;
    obj["lastHr"] = lastHr_;
    obj["rmssd"] = (double)((int)(rmssd_ * 10 + 0.5) / 10.0);
    obj["sdnn"] = (double)((int)(sdnn_ * 10 + 0.5) / 10.0);
    obj["sdsd"] = (double)((int)(sdsd_ * 10 + 0.5) / 10.0);
    obj["pnn50"] = (double)((int)(pnn50_ * 10 + 0.5) / 10.0);
    obj["rrWindow"] = rrCount_;
    obj["hrvValidity"] = hrvValidity_;
    obj["hrvWindow"] = "last-valid-NN≤64";
    obj["calories"] = (double)((int)(calories_ * 10 + 0.5) / 10.0);
    obj["pctMax"] = (double)((int)(pctMax_ * 10 + 0.5) / 10.0);
    obj["pctHrr"] = (double)((int)(pctHrr_ * 10 + 0.5) / 10.0);
    obj["zone"] = zone_;
    obj["hrMaxEst"] = hrMaxBpm();
    obj["zoneMode"] = cfg_ ? cfg_->zoneMode.c_str() : "auto";
    obj["zoneBy"] = (cfg_ && cfg_->zonesUseHrr()) ? "hrr" : "hrmax";
    JsonObject zt = obj["zoneTimeS"].to<JsonObject>();
    for (uint8_t z = 1; z <= 5; z++) {
        char key[4];
        snprintf(key, sizeof(key), "z%u", (unsigned)z);
        zt[key] = zoneTimeMs_[z] / 1000UL;
    }

    JsonObject rec = obj["recovery"].to<JsonObject>();
    rec["active"] = recoveryActive_;
    rec["startHr"] = recoveryStartHr_;
    rec["elapsedS"] = (recoveryStartedMs_ && (recoveryActive_ || have60_))
                          ? (millis() - recoveryStartedMs_) / 1000UL
                          : 0;
    rec["hr60"] = have60_ ? hrAt60_ : -1;
    rec["hr120"] = have120_ ? hrAt120_ : -1;
    rec["hr300"] = have300_ ? hrAt300_ : -1;
    rec["hrr1"] = have60_ ? (int)recoveryStartHr_ - (int)hrAt60_ : 0;
    rec["hrr2"] = have120_ ? (int)recoveryStartHr_ - (int)hrAt120_ : 0;
    rec["hrr5"] = have300_ ? (int)recoveryStartHr_ - (int)hrAt300_ : 0;

    JsonObject bl = obj["baseline"].to<JsonObject>();
    uint8_t stored = cfg_ ? cfg_->restingHr : 0;
    bl["restingHr"] = stored;
    bl["liveRestHr"] = liveRestHr_;
    bl["restMin"] = restHrMin_;
    bl["restSamples"] = restSampleCount_;
    bl["restDurationS"] = (mode_ == SessionMode::Rest && modeStartedMs_)
                              ? (millis() - modeStartedMs_) / 1000UL
                              : 0;
    bl["warmupS"] = (unsigned)(kRestWarmupMs / 1000UL);
    bl["minCaptureS"] = (unsigned)(kRestMinCaptureMs / 1000UL);
    bl["stable"] = restStable();
    bl["quiet"] = restQuietEnough();
    bl["ready"] = (liveRestHr_ > 0 && restSampleCount_ >= 30);
    bl["autoSaved"] = baselineAutoSaved_;
    bl["refined"] = baselineRefined_;
    bl["lastCaptureAuto"] = lastCaptureAuto_;

    JsonObject g = bl["guide"].to<JsonObject>();
    g["active"] = guidedActive_ && (guidePhase_ == GuidePhase::Settle || guidePhase_ == GuidePhase::Measure);
    g["phase"] = guidePhaseName(guidePhase_);
    g["message"] = guideMsg_ ? guideMsg_ : "";
    g["settleS"] = (unsigned)(kGuideSettleMs / 1000UL);
    g["measureS"] = (unsigned)(kGuideMeasureMs / 1000UL);
    g["totalS"] = (unsigned)((kGuideSettleMs + kGuideMeasureMs) / 1000UL);
    unsigned long gelapsed = 0;
    if (guideStartedMs_ && (guidePhase_ == GuidePhase::Settle || guidePhase_ == GuidePhase::Measure ||
                            guidePhase_ == GuidePhase::Done || guidePhase_ == GuidePhase::Failed)) {
        gelapsed = (millis() - guideStartedMs_) / 1000UL;
    }
    g["elapsedS"] = gelapsed;
    unsigned long total = (kGuideSettleMs + kGuideMeasureMs) / 1000UL;
    g["remainS"] = (guidePhase_ == GuidePhase::Settle || guidePhase_ == GuidePhase::Measure)
                       ? (gelapsed < total ? total - gelapsed : 0)
                       : 0;
    g["progressPct"] = total ? (unsigned)((gelapsed > total ? 100 : (gelapsed * 100UL) / total)) : 0;
}

void SessionStats::appendIoValues(JsonObject ios) const {
    auto addS = [&](const char* key, const char* value) {
        JsonObject o = ios[key].to<JsonObject>();
        o["type"] = "sensor";
        o["value"] = value;
        o["unit"] = "";
    };
    auto addN = [&](const char* key, float value, const char* unit) {
        JsonObject o = ios[key].to<JsonObject>();
        o["type"] = "sensor";
        o["value"] = value;
        o["unit"] = unit;
    };

    unsigned long durMs = 0;
    if (startedMs_) {
        unsigned long end = active_ ? millis() : (endedMs_ ? endedMs_ : millis());
        durMs = end - startedMs_;
    }
    addN("session_active", active_ ? 1 : 0, "");
    addS("session_mode", sessionModeName(mode_));
    addS("suggested_mode", sessionModeName(suggestedMode_));
    addN("session_duration", (float)(durMs / 1000UL), "s");
    addN("hr_avg", sampleCount_ ? (float)(hrSum_ / sampleCount_) : 0, "BPM");
    addN("hr_min", hrMin_, "BPM");
    addN("hr_max", hrMax_, "BPM");
    addN("hr_from_rr", hrFromRr_, "BPM");
    addN("hrv_rmssd", rmssd_, "ms");
    addN("hrv_sdnn", sdnn_, "ms");
    addN("hrv_sdsd", sdsd_, "ms");
    addN("hrv_pnn50", pnn50_, "%");
    addS("hrv_validity", hrvValidity_);
    addN("hr_pct_max", pctMax_, "%");
    addN("hr_pct_hrr", pctHrr_, "%");
    addN("hr_zone", zone_, "");
    addS("hr_zone_by", (cfg_ && cfg_->zonesUseHrr()) ? "hrr" : "hrmax");
    addN("calories", calories_, "kcal");
    addN("rr_valid_window", rrCount_, "");
    addN("resting_hr", cfg_ ? cfg_->restingHr : 0, "BPM");
    addN("live_rest_hr", liveRestHr_, "BPM");
    addN("recovery_start_hr", recoveryStartHr_, "BPM");
    addN("recovery_hr_60", have60_ ? hrAt60_ : 0, "BPM");
    addN("recovery_hr_120", have120_ ? hrAt120_ : 0, "BPM");
    addN("recovery_hr_300", have300_ ? hrAt300_ : 0, "BPM");
    addN("hrr_1min", have60_ ? (float)((int)recoveryStartHr_ - (int)hrAt60_) : 0, "BPM");
    addN("hrr_2min", have120_ ? (float)((int)recoveryStartHr_ - (int)hrAt120_) : 0, "BPM");
    addN("hrr_5min", have300_ ? (float)((int)recoveryStartHr_ - (int)hrAt300_) : 0, "BPM");
    char zbuf[8];
    if (zone_ >= 1 && zone_ <= 5) snprintf(zbuf, sizeof(zbuf), "Z%u", (unsigned)zone_);
    else snprintf(zbuf, sizeof(zbuf), "-");
    addS("hr_zone_label", zbuf);
}
