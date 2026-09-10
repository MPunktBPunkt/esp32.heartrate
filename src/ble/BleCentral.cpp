#include "BleCentral.h"
#include <WiFi.h>

static BleCentral* g_ble = nullptr;

/** Keep retrying reconnect for this long after first link loss (strap off / weak RSSI). */
static constexpr uint32_t kReconnectGiveUpMs = 5UL * 60UL * 1000UL;
static constexpr uint32_t kReconnectScanMs = 10000;
static constexpr uint32_t kFreshAdvertiseMs = 8000;
static constexpr uint32_t kStalePacketMs = 12000;
static constexpr uint32_t kStalePacketWeakMs = 22000;
static constexpr int8_t kWeakRssi = -85;

void bleConnectTaskThunk(void* arg) {
    BleCentral* self = static_cast<BleCentral*>(arg);
    const bool isReconnect = self->connectIsReconnect_;
    char mac[18];
    strncpy(mac, self->pendingMac_, sizeof(mac) - 1);
    mac[sizeof(mac) - 1] = 0;
    self->connectInternal(mac, isReconnect);
    self->connectBusy_ = false;
    vTaskDelete(nullptr);
}

static void notifyCb(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool) {
    if (g_ble) g_ble->onNotify(chr, data, len);
}

class HrAdvertisedCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) override {
        if (g_ble) g_ble->onScanResult(advertisedDevice);
    }
};

class HrClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient*) override {}
    void onDisconnect(NimBLEClient*) override {
        if (g_ble) g_ble->onDisconnect();
    }
};

static HrAdvertisedCallbacks g_advCb;
static HrClientCallbacks g_cliCb;

void BleCentral::begin(ConfigStore* cfg, HistoryStore* history) {
    cfg_ = cfg;
    history_ = history;
    g_ble = this;
#if HR_RELAY
    String gapName = (cfg_ && cfg_->relayName.length()) ? cfg_->relayName
                                                       : ConfigStore::defaultRelayName();
    NimBLEDevice::init(gapName.c_str());
#else
    NimBLEDevice::init("");
#endif
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    NimBLEDevice::setSecurityAuth(false, false, false);
    setState(BleState::Idle);
    Serial.println("[BLE] Central ready (manual connect)");
}

void BleCentral::setState(BleState s) {
    if (state_ == s) return;
    state_ = s;
    Serial.printf("[BLE] state %s\n", bleStateName(s));
}

int BleCentral::findScanIndex(const char* addr) const {
    for (uint8_t i = 0; i < scanCount_; i++) {
        if (strcasecmp(scan_[i].addr, addr) == 0) return i;
    }
    return -1;
}

uint8_t BleCentral::resolveAddrType(const char* mac) const {
    int si = findScanIndex(mac);
    if (si >= 0) return scan_[si].addrType;
    if (peerMac_[0] && strcasecmp(peerMac_, mac) == 0) return peerAddrType_;
    return BLE_ADDR_PUBLIC;
}

void BleCentral::applyConnParams(NimBLEClient* client) {
    if (!client) return;
    // denser connect scan; longer supervision helps weak RSSI / WiFi coexistence
    client->setConnectionParams(24, 40, 0, 1000, 80, 60);
    client->setConnectTimeout(10);
}

void BleCentral::onScanResult(NimBLEAdvertisedDevice* dev) {
    if (!dev) return;
    std::string addr = dev->getAddress().toString();
    const char* a = addr.c_str();
    bool hasHr = false;
    if (dev->isAdvertisingService(NimBLEUUID((uint16_t)0x180D))) hasHr = true;
    // Many straps only put service in scan response intermittently — keep name hits too
    std::string name = dev->getName();
    int idx = findScanIndex(a);
    if (idx < 0) {
        if (scanCount_ >= HR_MAX_SCAN) return;
        idx = scanCount_++;
        strncpy(scan_[idx].addr, a, sizeof(scan_[idx].addr) - 1);
        scan_[idx].addr[sizeof(scan_[idx].addr) - 1] = 0;
        scan_[idx].name[0] = 0;
        scan_[idx].hasHrService = false;
        scan_[idx].addrType = BLE_ADDR_PUBLIC;
    }
    scan_[idx].rssi = dev->getRSSI();
    scan_[idx].lastSeen = millis();
    scan_[idx].addrType = dev->getAddress().getType();
    if (hasHr) scan_[idx].hasHrService = true;
    if (!name.empty()) {
        strncpy(scan_[idx].name, name.c_str(), sizeof(scan_[idx].name) - 1);
        scan_[idx].name[sizeof(scan_[idx].name) - 1] = 0;
    }
}

void BleCentral::startScan() {
    if (sessionActive_ && (state_ == BleState::Ready || state_ == BleState::Reconnecting)) return;
    stopScan();
    scanCount_ = 0;
    NimBLEScan* scan = NimBLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(&g_advCb, false);
    scan->setActiveScan(true);
    scan->setInterval(160);
    scan->setWindow(80);
    scanning_ = true;
    setState(BleState::Scanning);
    scan->start(0, nullptr, false);  // continuous until stop
    Serial.println("[BLE] scan start");
}

void BleCentral::startReconnectScan() {
    stopScan();
    NimBLEScan* scan = NimBLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(&g_advCb, false);
    scan->setActiveScan(true);
    // denser scan window while hunting the strap after link loss
    scan->setInterval(96);
    scan->setWindow(72);
    scanning_ = true;
    reconnectScanning_ = true;
    reconnectScanUntil_ = millis() + kReconnectScanMs;
    setState(BleState::Reconnecting);
    scan->start(0, nullptr, false);
    Serial.printf("[BLE] reconnect scan for %s\n", pendingMac_);
}

void BleCentral::stopScan() {
    NimBLEScan* scan = NimBLEDevice::getScan();
    if (scan->isScanning()) scan->stop();
    scanning_ = false;
    reconnectScanning_ = false;
    if (state_ == BleState::Scanning) setState(BleState::Idle);
}

const ScanDevice* BleCentral::scanDevice(uint8_t i) const {
    if (i >= scanCount_) return nullptr;
    return &scan_[i];
}

void BleCentral::scanToJson(JsonArray arr) const {
    for (uint8_t i = 0; i < scanCount_; i++) {
        JsonObject o = arr.add<JsonObject>();
        o["mac"] = scan_[i].addr;
        o["name"] = scan_[i].name;
        o["rssi"] = scan_[i].rssi;
        o["hrService"] = scan_[i].hasHrService;
        o["ageMs"] = millis() - scan_[i].lastSeen;
        bool rem = cfg_ && cfg_->rememberedMac.length() &&
                   strcasecmp(scan_[i].addr, cfg_->rememberedMac.c_str()) == 0;
        o["remembered"] = rem;
    }
}

bool BleCentral::connect(const char* mac) {
    if (!mac || !*mac) return false;
    if (connectBusy_) {
        snprintf(lastError_, sizeof(lastError_), "connect in progress");
        return false;
    }
    wantDisconnect_ = false;
    sessionActive_ = true;
    reconnectTries_ = 0;
    reconnectScanning_ = false;
    reconnectWindowStart_ = 0;
    resetLinkStats();
    queueConnect(mac, false);
    return true;
}

void BleCentral::resetLinkStats() {
    sessionLinkStartMs_ = millis();
    linkLossCount_ = 0;
    reconnectOkCount_ = 0;
    rssiMin_ = 0;
    rssiMax_ = 0;
    rssiSum_ = 0;
    rssiSamples_ = 0;
    haveRssiStats_ = false;
}

void BleCentral::noteRssi(int8_t rssi) {
    if (rssi == 0) return;  // unset / invalid typical
    if (!haveRssiStats_) {
        rssiMin_ = rssiMax_ = rssi;
        haveRssiStats_ = true;
    } else {
        if (rssi < rssiMin_) rssiMin_ = rssi;
        if (rssi > rssiMax_) rssiMax_ = rssi;
    }
    rssiSum_ += rssi;
    if (rssiSamples_ < 60000) rssiSamples_++;
}

const char* BleCentral::linkQualityName(int8_t rssi) {
    if (rssi >= -65) return "good";
    if (rssi >= -80) return "fair";
    if (rssi >= -90) return "weak";
    return "poor";
}

bool BleCentral::linkRssiStats(int8_t& minOut, int8_t& maxOut, int8_t& avgOut) const {
    if (!haveRssiStats_ || !rssiSamples_) return false;
    minOut = rssiMin_;
    maxOut = rssiMax_;
    avgOut = (int8_t)(rssiSum_ / (int32_t)rssiSamples_);
    return true;
}

void BleCentral::queueConnect(const char* mac, bool isReconnect) {
    strncpy(pendingMac_, mac, sizeof(pendingMac_) - 1);
    pendingMac_[sizeof(pendingMac_) - 1] = 0;
    connectIsReconnect_ = isReconnect;
    lastError_[0] = 0;
    connectQueued_ = true;
    stopScan();
    setState(isReconnect ? BleState::Reconnecting : BleState::Connecting);
    Serial.printf("[BLE] connect queued %s reconnect=%d\n", pendingMac_, (int)isReconnect);
}

bool BleCentral::connectInternal(const char* mac, bool isReconnect) {
    if (wantDisconnect_) {
        setState(BleState::Idle);
        return false;
    }
    stopScan();
    strncpy(pendingMac_, mac, sizeof(pendingMac_) - 1);
    pendingMac_[sizeof(pendingMac_) - 1] = 0;
    setState(isReconnect ? BleState::Reconnecting : BleState::Connecting);
    lastError_[0] = 0;

    if (client_) {
        if (client_->isConnected()) client_->disconnect();
        cleanupClient();
    }

    client_ = NimBLEDevice::createClient();
    client_->setClientCallbacks(&g_cliCb, false);
    applyConnParams(client_);

    uint8_t addrType = resolveAddrType(mac);
    NimBLEAddress addr(mac, addrType);
    Serial.printf("[BLE] connect %s type=%u reconnect=%d try=%u\n",
                  mac, (unsigned)addrType, (int)isReconnect, (unsigned)reconnectTries_);
    if (!client_->connect(addr)) {
        uint8_t alt = (addrType == BLE_ADDR_PUBLIC) ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;
        NimBLEAddress addrAlt(mac, alt);
        if (!client_->connect(addrAlt)) {
            snprintf(lastError_, sizeof(lastError_),
                     isReconnect ? "reconnect connect failed" : "connect failed");
            cleanupClient();
            if (isReconnect) {
                // keep session alive for further backoff retries
                setState(BleState::Lost);
                lostAt_ = millis();
            } else {
                sessionActive_ = false;
                setState(BleState::Error);
            }
            return false;
        }
        addrType = alt;
    }

    peerAddrType_ = addrType;
    setState(BleState::Connected);
    strncpy(peerMac_, client_->getPeerAddress().toString().c_str(), sizeof(peerMac_) - 1);
    peerMac_[sizeof(peerMac_) - 1] = 0;
    peerName_[0] = 0;
    int si = findScanIndex(peerMac_);
    if (si >= 0 && scan_[si].name[0]) {
        strncpy(peerName_, scan_[si].name, sizeof(peerName_) - 1);
    } else if (cfg_ && cfg_->rememberedMac.length() &&
               strcasecmp(peerMac_, cfg_->rememberedMac.c_str()) == 0 &&
               cfg_->rememberedName.length()) {
        strncpy(peerName_, cfg_->rememberedName.c_str(), sizeof(peerName_) - 1);
    }
    peerName_[sizeof(peerName_) - 1] = 0;

    connectedSince_ = millis();
    lastPacketMs_ = 0;
    hasSample_ = false;
    battery_ = -1;

    if (!subscribeHr(client_)) {
        client_->disconnect();
        cleanupClient();
        if (isReconnect) {
            setState(BleState::Lost);
            lostAt_ = millis();
        } else {
            setState(BleState::Error);
            sessionActive_ = false;
        }
        return false;
    }

    // Prefer longer supervision after link is up (weak RSSI / WiFi coexistence)
    client_->updateConnParams(24, 40, 0, 1200);

    pollBattery(client_);
    pollRssi(client_);
    setState(BleState::Ready);
    reconnectTries_ = 0;
    reconnectWindowStart_ = 0;
    reconnectScanning_ = false;
    if (isReconnect) reconnectOkCount_++;
    return true;
}

bool BleCentral::subscribeHr(NimBLEClient* client) {
    setState(BleState::Subscribing);
    NimBLERemoteService* svc = client->getService(NimBLEUUID((uint16_t)0x180D));
    if (!svc) {
        snprintf(lastError_, sizeof(lastError_), "HR service missing");
        return false;
    }
    NimBLERemoteCharacteristic* chr = svc->getCharacteristic(NimBLEUUID((uint16_t)0x2A37));
    if (!chr) {
        snprintf(lastError_, sizeof(lastError_), "HR measurement missing");
        return false;
    }
    if (chr->canNotify()) {
        if (!chr->subscribe(true, notifyCb)) {
            snprintf(lastError_, sizeof(lastError_), "HR subscribe failed");
            return false;
        }
    } else if (chr->canIndicate()) {
        if (!chr->subscribe(false, notifyCb)) {
            snprintf(lastError_, sizeof(lastError_), "HR indicate failed");
            return false;
        }
    } else {
        snprintf(lastError_, sizeof(lastError_), "HR not notifiable");
        return false;
    }
    return true;
}

void BleCentral::pollBattery(NimBLEClient* client) {
    if (!client || !client->isConnected()) return;
    NimBLERemoteService* svc = client->getService(NimBLEUUID((uint16_t)0x180F));
    if (!svc) return;
    NimBLERemoteCharacteristic* chr = svc->getCharacteristic(NimBLEUUID((uint16_t)0x2A19));
    if (!chr || !chr->canRead()) return;
    std::string v = chr->readValue();
    if (v.size() >= 1) {
        battery_ = (int8_t)(uint8_t)v[0];
        last_.battery = battery_;
        if (history_) history_->push(HistoryStore::BATTERY, (float)battery_);
    }
    lastBatteryPoll_ = millis();
}

void BleCentral::pollRssi(NimBLEClient* client) {
    if (!client || !client->isConnected()) return;
    int rssi = client->getRssi();
    // ESP32 occasionally returns nonsense (< -105) during WiFi coexistence glitches
    if (rssi != 0 && rssi >= -105 && rssi <= 0) {
        last_.rssi = (int8_t)rssi;
        noteRssi((int8_t)rssi);
        if (history_) history_->push(HistoryStore::RSSI, (float)rssi);
    }
    lastRssiPoll_ = millis();
}

void BleCentral::onNotify(NimBLERemoteCharacteristic*, uint8_t* data, size_t len) {
    HrSample s;
    s.battery = battery_;
    s.rssi = last_.rssi;
    if (!parseHeartRateMeasurement(data, len, s)) return;
    last_ = s;
    hasSample_ = true;
    lastPacketMs_ = millis();
    sampleDirty_ = true;
    if (history_) {
        history_->push(HistoryStore::HR_BPM, (float)s.heartRate);
        for (uint8_t i = 0; i < s.rrCount; i++) history_->pushRr((float)s.rrMsAt(i));
    }
}

bool BleCentral::takeSampleDirty() {
    if (!sampleDirty_) return false;
    sampleDirty_ = false;
    return true;
}

void BleCentral::cleanupClient() {
    if (!client_) return;
    NimBLEDevice::deleteClient(client_);
    client_ = nullptr;
}

void BleCentral::onDisconnect() {
    disconnectEvt_ = true;
}

void BleCentral::disconnect() {
    wantDisconnect_ = true;
    sessionActive_ = false;
    connectQueued_ = false;
    reconnectTries_ = 0;
    reconnectWindowStart_ = 0;
    reconnectScanning_ = false;
    lastError_[0] = 0;
    setState(BleState::Disconnecting);
    stopScan();
    // If connect task still running, it will see wantDisconnect_ / fail and clean up;
    // also request disconnect if already linked.
    if (client_ && client_->isConnected()) {
        client_->disconnect();
    } else if (!connectBusy_) {
        wantDisconnect_ = false;
        cleanupClient();
        setState(BleState::Idle);
        peerMac_[0] = 0;
    }
}

void BleCentral::rememberCurrent() {
    if (!cfg_ || !peerMac_[0]) return;
    cfg_->rememberedMac = peerMac_;
    cfg_->rememberedName = peerName_[0] ? peerName_ : "HR Sensor";
    cfg_->save();
}

void BleCentral::forgetRemembered() {
    if (!cfg_) return;
    cfg_->rememberedMac = "";
    cfg_->rememberedName = "";
    cfg_->save();
}

unsigned long BleCentral::reconnectBackoffMs() const {
    // Cap at 15s — keep hunting while give-up window is open
    static const uint16_t kBackoffS[] = {2, 3, 4, 5, 6, 8, 10, 12, 15};
    uint8_t i = reconnectTries_;
    if (i >= sizeof(kBackoffS) / sizeof(kBackoffS[0])) i = sizeof(kBackoffS) / sizeof(kBackoffS[0]) - 1;
    return (unsigned long)kBackoffS[i] * 1000UL;
}

void BleCentral::handleSessionReconnect() {
    if (!sessionActive_ || !cfg_ || !cfg_->sessionReconnect) return;
    if (state_ != BleState::Lost && state_ != BleState::Error && state_ != BleState::Reconnecting) return;
    if (pendingMac_[0] == 0) return;
    if (wantDisconnect_) return;

    if (!reconnectWindowStart_) reconnectWindowStart_ = lostAt_ ? lostAt_ : millis();
    if (millis() - reconnectWindowStart_ >= kReconnectGiveUpMs) {
        sessionActive_ = false;
        reconnectScanning_ = false;
        stopScan();
        snprintf(lastError_, sizeof(lastError_), "reconnect timeout (5 min)");
        setState(BleState::Error);
        Serial.println("[BLE] reconnect give up after 5 min");
        return;
    }

    // Phase 1: wait backoff after loss / failed attempt
    if (!reconnectScanning_ && state_ != BleState::Reconnecting) {
        if (millis() - lostAt_ < reconnectBackoffMs()) return;
        startReconnectScan();
        return;
    }

    // Phase 2: hunt advertise, then connect
    if (reconnectScanning_) {
        int si = findScanIndex(pendingMac_);
        bool fresh = si >= 0 && (millis() - scan_[si].lastSeen) < kFreshAdvertiseMs;
        bool timedOut = millis() >= reconnectScanUntil_;
        if (!fresh && !timedOut) return;

        stopScan();
        reconnectScanning_ = false;
        if (!fresh) {
            // strap not advertising yet — count as try, back to Lost (keep hunting)
            reconnectTries_++;
            snprintf(lastError_, sizeof(lastError_), "reconnect: not advertising (try %u)",
                     (unsigned)reconnectTries_);
            setState(BleState::Lost);
            lostAt_ = millis();
            return;
        }
        reconnectTries_++;
        queueConnect(pendingMac_, true);
        return;
    }

    // Reconnecting without scan flag: resume scan phase after failed connectInternal
    if (state_ == BleState::Reconnecting && !client_ && !connectBusy_ && !connectQueued_) {
        if (millis() - lostAt_ < reconnectBackoffMs()) return;
        startReconnectScan();
    }
}

void BleCentral::handleIdleDisconnect() {
    if (relayHold_) return;
    if (!cfg_ || cfg_->idleDisconnectS == 0) return;
    if (!sessionActive_ || state_ != BleState::Ready) return;
    if (!hasSample_) return;
    unsigned long idleMs = (unsigned long)cfg_->idleDisconnectS * 1000UL;
    if (millis() - lastPacketMs_ > idleMs) {
        Serial.println("[BLE] idle disconnect");
        disconnect();
    }
}

void BleCentral::loop() {
    if (disconnectEvt_) {
        disconnectEvt_ = false;
        if (connectBusy_) {
            // connect task still owns the client — defer cleanup
            disconnectEvt_ = true;
        } else {
            cleanupClient();
            reconnectScanning_ = false;
            if (wantDisconnect_) {
                wantDisconnect_ = false;
                sessionActive_ = false;
                connectQueued_ = false;
                setState(BleState::Idle);
                peerMac_[0] = 0;
            } else if (sessionActive_) {
                setState(BleState::Lost);
                lostAt_ = millis();
                if (!reconnectWindowStart_) reconnectWindowStart_ = lostAt_;
                linkLossCount_++;
                snprintf(lastError_, sizeof(lastError_), "link lost (rssi %d)", (int)last_.rssi);
                Serial.printf("[BLE] link lost (rssi was %d) loss#%u\n", (int)last_.rssi,
                              (unsigned)linkLossCount_);
            } else {
                setState(BleState::Idle);
            }
        }
    }

    if (connectQueued_ && !connectBusy_ && !wantDisconnect_) {
        connectQueued_ = false;
        connectBusy_ = true;
        // Separate task so HTTP/SSE keep running during the blocking connect()
        BaseType_t ok = xTaskCreatePinnedToCore(
            bleConnectTaskThunk, "hr_conn", 8192, this, 1, nullptr, 0);
        if (ok != pdPASS) {
            connectBusy_ = false;
            snprintf(lastError_, sizeof(lastError_), "connect task failed");
            setState(BleState::Error);
            if (!connectIsReconnect_) sessionActive_ = false;
        }
    }

    if (scanning_ && state_ == BleState::Idle) setState(BleState::Scanning);

    if (state_ == BleState::Ready && client_ && client_->isConnected() && !connectBusy_) {
        // Battery rarely changes — avoid frequent GATT reads that stall notifies
        if (millis() - lastBatteryPoll_ > 300000UL) pollBattery(client_);
        if (millis() - lastRssiPoll_ > 5000UL) pollRssi(client_);
        // Half-dead link: still "connected" but no HR notifies → force reconnect path
        // Weak RSSI: wait longer before reset (coexistence jitter)
        uint32_t staleMs = (last_.rssi <= kWeakRssi) ? kStalePacketWeakMs : kStalePacketMs;
        if (hasSample_ && millis() - lastPacketMs_ > staleMs) {
            Serial.printf("[BLE] stale HR packet (%lums, rssi %d) — forcing link reset\n",
                          (unsigned long)(millis() - lastPacketMs_), (int)last_.rssi);
            snprintf(lastError_, sizeof(lastError_), "stale packets (rssi %d)", (int)last_.rssi);
            client_->disconnect();
        }
    }

    handleIdleDisconnect();
    if (!connectBusy_) handleSessionReconnect();
}

void BleCentral::appendStatusJson(JsonObject obj) const {
    obj["state"] = bleStateName(state_);
    obj["scanning"] = scanning_;
    obj["session"] = sessionActive_;
    obj["mac"] = peerMac_;
    obj["name"] = peerName_;
    obj["error"] = lastError_;
    obj["reconnectTries"] = reconnectTries_;
    obj["linkLossCount"] = linkLossCount_;
    obj["reconnectOkCount"] = reconnectOkCount_;
    obj["addrType"] = peerAddrType_;
    obj["connectedSinceMs"] = connectedSince_ ? (millis() - connectedSince_) : 0;
    obj["sessionLinkMs"] = sessionLinkStartMs_ ? (millis() - sessionLinkStartMs_) : 0;
    if (hasSample_) obj["lastPacketAgeMs"] = (long)(millis() - lastPacketMs_);
    else obj["lastPacketAgeMs"] = (long)-1;
    obj["battery"] = battery_;
    {
        JsonObject link = obj["link"].to<JsonObject>();
        link["lossCount"] = linkLossCount_;
        link["reconnectOk"] = reconnectOkCount_;
        if (haveRssiStats_ && rssiSamples_) {
            int8_t avg = (int8_t)(rssiSum_ / (int32_t)rssiSamples_);
            link["rssiMin"] = rssiMin_;
            link["rssiMax"] = rssiMax_;
            link["rssiAvg"] = avg;
            link["rssiSamples"] = rssiSamples_;
            link["quality"] = linkQualityName(hasSample_ ? last_.rssi : avg);
        } else {
            link["rssiMin"] = 0;
            link["rssiMax"] = 0;
            link["rssiAvg"] = 0;
            link["rssiSamples"] = 0;
            link["quality"] = hasSample_ ? linkQualityName(last_.rssi) : "n/a";
        }
        if (hasSample_) link["rssiNow"] = last_.rssi;
    }
    bool recon = sessionActive_ && (state_ == BleState::Lost || state_ == BleState::Reconnecting ||
                                    (state_ == BleState::Error && reconnectWindowStart_ &&
                                     millis() - reconnectWindowStart_ < kReconnectGiveUpMs));
    obj["reconnecting"] = recon;
    if (sessionActive_ && reconnectWindowStart_) {
        unsigned long elapsed = millis() - reconnectWindowStart_;
        obj["reconnectElapsedS"] = elapsed / 1000UL;
        obj["reconnectRemainS"] =
            elapsed < kReconnectGiveUpMs ? (kReconnectGiveUpMs - elapsed) / 1000UL : 0;
    } else {
        obj["reconnectElapsedS"] = 0;
        obj["reconnectRemainS"] = 0;
    }
    if (hasSample_) {
        JsonObject s = obj["sample"].to<JsonObject>();
        s["t"] = last_.tMs;
        s["hr"] = last_.heartRate;
        s["rssi"] = last_.rssi;
        s["bat"] = last_.battery;
        s["contact"] = last_.contact;
        s["contactSupported"] = last_.contactSupported;
        s["flags"] = last_.flags;
        s["energy"] = last_.energyPresent ? last_.energyExpended : (int)-1;
        JsonArray rr = s["rr"].to<JsonArray>();
        JsonArray rrRaw = s["rrRaw"].to<JsonArray>();
        for (uint8_t i = 0; i < last_.rrCount; i++) {
            rr.add(last_.rrMsAt(i));
            rrRaw.add(last_.rrRaw[i]);
        }
    }
}

void BleCentral::appendIoValues(JsonObject ios) const {
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

    addS("ble_state", bleStateName(state_));
    addS("device_name", peerName_[0] ? peerName_ : "-");
    addS("device_mac", peerMac_[0] ? peerMac_ : "-");
    addN("connected", (state_ == BleState::Ready) ? 1 : 0, "");
    addN("heart_rate", hasSample_ ? last_.heartRate : 0, "BPM");
    addN("rr_interval", hasSample_ && last_.rrCount ? last_.rrMsAt(last_.rrCount - 1) : 0, "ms");
    addN("rr_count", hasSample_ ? last_.rrCount : 0, "");
    addN("battery", battery_ >= 0 ? battery_ : 0, "%");
    addN("rssi_ble", hasSample_ ? last_.rssi : 0, "dBm");
    addN("ble_link_loss", (float)linkLossCount_, "");
    addN("ble_reconnect_ok", (float)reconnectOkCount_, "");
    if (haveRssiStats_ && rssiSamples_) {
        addN("ble_rssi_min", (float)rssiMin_, "dBm");
        addN("ble_rssi_avg", (float)(rssiSum_ / (int32_t)rssiSamples_), "dBm");
    }
    const char* contact = "n/a";
    if (hasSample_) {
        if (!last_.contactSupported) contact = "n/a";
        else if (last_.contact == 1) contact = "detected";
        else if (last_.contact == 2) contact = "not_detected";
        else contact = "unknown";
    }
    addS("sensor_contact", contact);
    addN("sensor_contact_supported", hasSample_ && last_.contactSupported ? 1 : 0, "");
    addN("last_packet_age", hasSample_ ? (float)(millis() - lastPacketMs_) : -1.0f, "ms");
}
