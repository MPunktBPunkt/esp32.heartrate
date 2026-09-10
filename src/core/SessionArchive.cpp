#include "SessionArchive.h"
#include "NetUtil.h"
#include <LittleFS.h>
#include <Preferences.h>

static Preferences sessPrefs;
static const char* kSessNs = "hr_sess";
static const char* kSessPath = "/sessions.bin";
static const char* kSessTmpPath = "/sessions.tmp";

void SessionSummary::toJson(JsonObject obj) const {
    obj["id"] = id;
    obj["durationS"] = durationS;
    obj["samples"] = samples;
    obj["hrAvg"] = hrAvg;
    obj["hrMin"] = hrMin;
    obj["hrMax"] = hrMax;
    obj["restingHr"] = restingHr;
    obj["rmssd"] = (double)((int)(rmssd * 10 + 0.5) / 10.0);
    obj["sdnn"] = (double)((int)(sdnn * 10 + 0.5) / 10.0);
    obj["pnn50"] = (double)((int)(pnn50 * 10 + 0.5) / 10.0);
    obj["calories"] = (double)((int)(calories * 10 + 0.5) / 10.0);
    JsonObject zt = obj["zoneTimeS"].to<JsonObject>();
    for (uint8_t z = 0; z < 5; z++) {
        char key[4];
        snprintf(key, sizeof(key), "z%u", (unsigned)(z + 1));
        zt[key] = zoneS[z];
    }
    obj["hrr1"] = hrr1;
    obj["hrr2"] = hrr2;
    obj["hrr5"] = hrr5;
    obj["endMode"] = endMode;
    obj["flags"] = flags;
    obj["reconnectCount"] = reconnectCount;
    obj["continuityPct"] = continuityPct;
    obj["rssiMin"] = rssiMin;
    obj["rssiAvg"] = rssiAvg;
    obj["peer"] = peer;
    obj["endedUnix"] = endedUnix;
    if (endedUnix) {
        obj["endedAt"] = NetUtil::formatUnixLocal(endedUnix);
        if (durationS && endedUnix > durationS) {
            obj["startedUnix"] = endedUnix - durationS;
            obj["startedAt"] = NetUtil::formatUnixLocal(endedUnix - durationS);
        }
    }
}

void SessionArchive::begin() {
    beginFs();
    if (fsReady_) {
        if (!LittleFS.exists(kSessPath)) migrateLegacyToFs();
        loadFsMeta();
    } else {
        loadLegacyMeta();
    }
}

void SessionArchive::beginFs() {
    // Try without formatting first to avoid extra write/flash churn during boot.
    fsReady_ = LittleFS.begin(false);
    if (!fsReady_) fsReady_ = LittleFS.begin(true);  // last resort: format once
    if (!fsReady_) {
        Serial.println("[SESS] LittleFS mount failed, fallback to NVS");
        return;
    }
    Serial.println("[SESS] LittleFS ready");
}

void SessionArchive::loadLegacyMeta() {
    sessPrefs.begin(kSessNs, true);
    count_ = sessPrefs.getUChar("cnt", 0);
    head_ = sessPrefs.getUChar("head", 0);
    nextId_ = sessPrefs.getUInt("nid", 1);
    sessPrefs.end();
    if (count_ > kMax) count_ = 0;
    if (head_ >= kMax) head_ = 0;
    if (nextId_ == 0) nextId_ = 1;
}

void SessionArchive::saveLegacyMeta() {
    sessPrefs.begin(kSessNs, false);
    sessPrefs.putUChar("cnt", count_);
    sessPrefs.putUChar("head", head_);
    sessPrefs.putUInt("nid", nextId_);
    sessPrefs.end();
}

bool SessionArchive::loadLegacySlot(uint8_t slot, SessionSummary& out) const {
    if (slot >= kMax) return false;
    char key[6];
    snprintf(key, sizeof(key), "s%u", (unsigned)slot);
    Preferences p;
    p.begin(kSessNs, true);
    size_t n = p.getBytesLength(key);
    if (n != sizeof(SessionSummary)) {
        p.end();
        return false;
    }
    size_t got = p.getBytes(key, &out, sizeof(SessionSummary));
    p.end();
    return got == sizeof(SessionSummary);
}

bool SessionArchive::saveLegacySlot(uint8_t slot, const SessionSummary& s) {
    if (slot >= kMax) return false;
    char key[6];
    snprintf(key, sizeof(key), "s%u", (unsigned)slot);
    sessPrefs.begin(kSessNs, false);
    size_t n = sessPrefs.putBytes(key, &s, sizeof(SessionSummary));
    sessPrefs.end();
    return n == sizeof(SessionSummary);
}

void SessionArchive::loadFsMeta() {
    count_ = 0;
    head_ = 0;
    nextId_ = 1;
    if (!fsReady_ || !LittleFS.exists(kSessPath)) return;

    File f = LittleFS.open(kSessPath, "r");
    if (!f) return;
    size_t sz = f.size();
    if (sz == 0) {
        f.close();
        return;
    }
    if (sz % sizeof(SessionSummary) != 0) {
        f.close();
        LittleFS.remove(kSessPath);
        Serial.println("[SESS] invalid archive file removed");
        return;
    }
    uint32_t records = sz / sizeof(SessionSummary);
    f.seek((records - 1) * sizeof(SessionSummary), SeekSet);
    SessionSummary last;
    if (f.read((uint8_t*)&last, sizeof(SessionSummary)) == sizeof(SessionSummary)) {
        nextId_ = last.id ? last.id + 1 : 1;
    }
    f.close();
    if (records > kMax) trimFsIfNeeded();
    count_ = records > kMax ? kMax : (uint8_t)records;
}

bool SessionArchive::trimFsIfNeeded() {
    if (!fsReady_ || !LittleFS.exists(kSessPath)) return true;

    File in = LittleFS.open(kSessPath, "r");
    if (!in) return false;
    uint32_t records = in.size() / sizeof(SessionSummary);
    if (records <= kMax) {
        count_ = (uint8_t)records;
        in.close();
        return true;
    }

    uint32_t keep = kMax;
    uint32_t skip = records - keep;
    File out = LittleFS.open(kSessTmpPath, "w");
    if (!out) {
        in.close();
        return false;
    }
    SessionSummary s;
    in.seek(skip * sizeof(SessionSummary), SeekSet);
    while (in.available() >= (int)sizeof(SessionSummary)) {
        if (in.read((uint8_t*)&s, sizeof(SessionSummary)) != sizeof(SessionSummary)) break;
        if (out.write((const uint8_t*)&s, sizeof(SessionSummary)) != sizeof(SessionSummary)) {
            in.close();
            out.close();
            return false;
        }
    }
    in.close();
    out.close();
    LittleFS.remove(kSessPath);
    LittleFS.rename(kSessTmpPath, kSessPath);
    count_ = kMax;
    return true;
}

bool SessionArchive::appendFs(const SessionSummary& s) {
    if (!fsReady_) return false;
    File f = LittleFS.open(kSessPath, "a");
    if (!f) return false;
    size_t n = f.write((const uint8_t*)&s, sizeof(SessionSummary));
    f.close();
    if (n != sizeof(SessionSummary)) return false;
    return trimFsIfNeeded();
}

bool SessionArchive::loadFsAt(uint32_t index, SessionSummary& out) const {
    if (!fsReady_ || !LittleFS.exists(kSessPath)) return false;
    File f = LittleFS.open(kSessPath, "r");
    if (!f) return false;
    uint32_t records = f.size() / sizeof(SessionSummary);
    if (index >= records) {
        f.close();
        return false;
    }
    f.seek(index * sizeof(SessionSummary), SeekSet);
    size_t got = f.read((uint8_t*)&out, sizeof(SessionSummary));
    f.close();
    return got == sizeof(SessionSummary);
}

bool SessionArchive::migrateLegacyToFs() {
    loadLegacyMeta();
    if (!count_) return true;

    File f = LittleFS.open(kSessPath, "w");
    if (!f) return false;
    for (int i = (int)count_ - 1; i >= 0; i--) {
        int slot = (int)head_ - 1 - i;
        while (slot < 0) slot += kMax;
        SessionSummary s;
        if (!loadLegacySlot((uint8_t)slot, s)) continue;
        if (f.write((const uint8_t*)&s, sizeof(SessionSummary)) != sizeof(SessionSummary)) {
            f.close();
            return false;
        }
    }
    f.close();
    Serial.printf("[SESS] migrated %u legacy sessions to LittleFS\n", (unsigned)count_);
    return true;
}

bool SessionArchive::push(const SessionSummary& in) {
    SessionSummary s = in;
    s.id = nextId_++;
    if (fsReady_) {
        if (!appendFs(s)) return false;
        loadFsMeta();
    } else {
        if (!saveLegacySlot(head_, s)) return false;
        head_ = (uint8_t)((head_ + 1) % kMax);
        if (count_ < kMax) count_++;
        saveLegacyMeta();
    }
    Serial.printf("[SESS] archived id=%lu dur=%lus avg=%u\n",
                  (unsigned long)s.id, (unsigned long)s.durationS, (unsigned)s.hrAvg);
    return true;
}

void SessionArchive::clear() {
    sessPrefs.begin(kSessNs, false);
    sessPrefs.clear();
    sessPrefs.end();
    if (fsReady_) {
        LittleFS.remove(kSessPath);
        LittleFS.remove(kSessTmpPath);
    }
    count_ = 0;
    head_ = 0;
    nextId_ = 1;
}

bool SessionArchive::get(uint8_t newestIndex, SessionSummary& out) const {
    if (newestIndex >= count_) return false;
    if (fsReady_) {
        uint32_t records = count_;
        uint32_t idx = records - 1 - newestIndex;
        return loadFsAt(idx, out);
    }
    int slot = (int)head_ - 1 - (int)newestIndex;
    while (slot < 0) slot += kMax;
    return loadLegacySlot((uint8_t)slot, out);
}

void SessionArchive::appendListJson(JsonArray arr, uint8_t limit) const {
    if (limit > count_) limit = count_;
    for (uint8_t i = 0; i < limit; i++) {
        SessionSummary s;
        if (!get(i, s)) continue;
        s.toJson(arr.add<JsonObject>());
    }
}
