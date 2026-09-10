#pragma once

#include "ble/BleTypes.h"
#include "ConfigStore.h"
#include "BeatTimeline.h"
#include "SessionArchive.h"
#include <ArduinoJson.h>

enum class SessionMode : uint8_t {
    Idle = 0,
    Rest = 1,
    Training = 2,
    Recovery = 3,
    Activity = 4  // Alltag / Haushalt
};

inline const char* sessionModeName(SessionMode m) {
    switch (m) {
        case SessionMode::Idle: return "IDLE";
        case SessionMode::Rest: return "REST";
        case SessionMode::Training: return "TRAINING";
        case SessionMode::Recovery: return "RECOVERY";
        case SessionMode::Activity: return "ACTIVITY";
        default: return "UNKNOWN";
    }
}

inline SessionMode sessionModeFromName(const char* s) {
    if (!s) return SessionMode::Idle;
    if (strcasecmp(s, "rest") == 0) return SessionMode::Rest;
    if (strcasecmp(s, "training") == 0) return SessionMode::Training;
    if (strcasecmp(s, "recovery") == 0) return SessionMode::Recovery;
    if (strcasecmp(s, "activity") == 0 || strcasecmp(s, "daily") == 0 || strcasecmp(s, "alltag") == 0)
        return SessionMode::Activity;
    if (strcasecmp(s, "idle") == 0) return SessionMode::Idle;
    return SessionMode::Idle;
}

inline bool sessionModeCountsZones(SessionMode m) {
    return m == SessionMode::Training || m == SessionMode::Activity;
}

inline bool sessionModeLimitsHrv(SessionMode m) {
    return m == SessionMode::Training || m == SessionMode::Activity;
}

inline bool sessionModeCountsCalories(SessionMode m) {
    return m == SessionMode::Training || m == SessionMode::Activity || m == SessionMode::Rest;
}

/** Live session aggregates + HRV + modes + recovery HRR + resting baseline. */
class SessionStats {
public:
    void begin(ConfigStore* cfg);
    void reset();
    void setSessionActive(bool active);
    bool setMode(SessionMode mode);
    SessionMode mode() const { return mode_; }
    void onSample(const HrSample& s, const BeatTimeline& beats);

    /** Persist current Rest average as resting HR. Returns false if no candidate. */
    bool captureBaseline(bool autoSave = false);
    void clearBaseline();
    /** Guided 4‑min rest protocol (1 min settle + 3 min measure). Needs active session. */
    bool startGuidedBaseline();
    void cancelGuidedBaseline();
    bool guidedActive() const { return guidedActive_; }

    /** Fill archive snapshot; false if session too short / few samples. */
    bool buildSummary(SessionSummary& out) const;

    bool active() const { return active_; }
    /** Duration as seen from now (if active) or endedMs_ (if ended). */
    uint32_t durationMs() const {
        if (!startedMs_) return 0;
        unsigned long end = active_ ? millis() : (endedMs_ ? endedMs_ : millis());
        return (uint32_t)(end - startedMs_);
    }
    uint32_t durationS() const { return durationMs() / 1000UL; }
    void appendJson(JsonObject obj) const;
    void appendIoValues(JsonObject ios) const;

private:
    static constexpr uint8_t kRrWindow = 64;
    static constexpr uint8_t kRestWin = 32;
    static constexpr unsigned long kRestWarmupMs = 60000UL;
    static constexpr unsigned long kRestMinCaptureMs = 180000UL;
    static constexpr float kRestStableStd = 3.5f;
    static constexpr unsigned long kModeHintElevateMs = 90000UL;
    static constexpr unsigned long kModeHintCalmMs = 120000UL;
    static constexpr uint8_t kElevateAboveRest = 12;
    static constexpr uint8_t kCalmAboveRest = 5;
    static constexpr uint8_t kRefineMinDropBpm = 3;
    static constexpr unsigned long kRefineCooldownMs = 600000UL;  // 10 min
    static constexpr unsigned long kGuideSettleMs = 60000UL;
    static constexpr unsigned long kGuideMeasureMs = 180000UL;
    static constexpr unsigned long kGuideGraceMs = 60000UL;

    enum class GuidePhase : uint8_t { Idle = 0, Settle = 1, Measure = 2, Done = 3, Failed = 4 };

    void recomputeHrv(const BeatTimeline& beats);
    void updateRecovery(uint16_t hr, unsigned long now);
    void clearRecovery();
    void resetRestAccum();
    void updateRestBaseline(uint16_t hr, unsigned long now);
    void updateModeHint(uint16_t hr, unsigned long now);
    void updateGuidedBaseline(uint16_t hr, unsigned long now);
    void clearGuided();
    bool restStable() const;
    bool restQuietEnough() const;
    /** Soft-auto reference rest: stored RHR, adapted if session floor is clearly lower. */
    uint16_t restRefForHints() const;
    uint16_t hrMaxBpm() const;
    uint8_t zoneFor(uint16_t hr) const;
    float caloriesDelta(uint16_t hr, float dtS) const;
    float pctHrr(uint16_t hr) const;
    static const char* guidePhaseName(GuidePhase p);

    ConfigStore* cfg_ = nullptr;
    bool active_ = false;
    SessionMode mode_ = SessionMode::Idle;
    unsigned long startedMs_ = 0;
    unsigned long endedMs_ = 0;
    unsigned long lastSampleMs_ = 0;
    unsigned long modeStartedMs_ = 0;

    uint32_t sampleCount_ = 0;
    uint16_t lastHr_ = 0;
    uint16_t hrMin_ = 0;
    uint16_t hrMax_ = 0;
    double hrSum_ = 0;

    uint16_t lastRr_ = 0;
    uint16_t hrFromRr_ = 0;

    uint16_t rrWin_[kRrWindow];
    uint8_t rrCount_ = 0;
    float rmssd_ = 0;
    float sdnn_ = 0;
    float pnn50_ = 0;
    float sdsd_ = 0;
    const char* hrvValidity_ = "INVALID";

    float calories_ = 0;
    uint8_t zone_ = 0;
    float pctMax_ = 0;
    float pctHrr_ = 0;
    uint32_t zoneTimeMs_[6] = {0};

    // Recovery (Training → Recovery)
    bool recoveryActive_ = false;
    unsigned long recoveryStartedMs_ = 0;
    uint16_t recoveryStartHr_ = 0;
    uint16_t hrAt60_ = 0;
    uint16_t hrAt120_ = 0;
    uint16_t hrAt300_ = 0;
    bool have60_ = false;
    bool have120_ = false;
    bool have300_ = false;

    // Resting baseline (Rest mode)
    uint32_t restSampleCount_ = 0;
    double restHrSum_ = 0;
    uint16_t restHrMin_ = 0;
    uint16_t liveRestHr_ = 0;
    uint16_t restWin_[kRestWin];
    uint8_t restWinCount_ = 0;
    uint8_t restWinIdx_ = 0;
    bool baselineAutoSaved_ = false;
    bool lastCaptureAuto_ = false;
    bool baselineRefined_ = false;
    unsigned long lastRefineMs_ = 0;

    // Guided baseline protocol
    bool guidedActive_ = false;
    GuidePhase guidePhase_ = GuidePhase::Idle;
    unsigned long guideStartedMs_ = 0;
    unsigned long guideElevateSinceMs_ = 0;
    const char* guideMsg_ = "";

    // Mode fit hint / soft auto Rest↔Activity
    SessionMode suggestedMode_ = SessionMode::Idle;
    unsigned long elevatedSinceMs_ = 0;
    unsigned long calmSinceMs_ = 0;
    bool autoSwitchedToActivity_ = false;
    bool autoSwitchedToRest_ = false;
};
