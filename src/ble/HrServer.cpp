#include "BuildFlags.h"

#if HR_RELAY

#include "HrServer.h"
#include "BleCentral.h"
#include "core/NetUtil.h"

static HrServer* g_relay = nullptr;

class RelayServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override {
        if (g_relay && desc) g_relay->onConnect(desc->conn_handle);
        else if (g_relay) g_relay->onConnect(0);
    }
    void onDisconnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override {
        if (g_relay && desc) g_relay->onDisconnect(desc->conn_handle);
        else if (g_relay) g_relay->onDisconnect(0);
        (void)pServer;
    }
};

class RelayHrCallbacks : public NimBLECharacteristicCallbacks {
    void onSubscribe(NimBLECharacteristic*, ble_gap_conn_desc* desc, uint16_t subValue) override {
        if (!g_relay || !desc) return;
        // bit0 notify, bit1 indicate
        bool sub = (subValue & 0x0001) || (subValue & 0x0002);
        g_relay->onHrSubscribe(desc->conn_handle, sub);
    }
};

static RelayServerCallbacks g_srvCb;
static RelayHrCallbacks g_hrCb;

void HrServer::begin(ConfigStore* cfg, BleCentral* ble) {
    cfg_ = cfg;
    ble_ = ble;
    g_relay = this;
    String n = effectiveName();
    strncpy(name_, n.c_str(), sizeof(name_) - 1);
    name_[sizeof(name_) - 1] = 0;
    if (cfg_ && cfg_->relayEnabled) setEnabled(true);
}

String HrServer::effectiveName() const {
    if (cfg_ && cfg_->relayName.length()) return cfg_->relayName;
    return ConfigStore::defaultRelayName();
}

void HrServer::ensureServer() {
    if (serverReady_) return;

    server_ = NimBLEDevice::createServer();
    server_->setCallbacks(&g_srvCb, false);

    NimBLEService* hr = server_->createService(NimBLEUUID((uint16_t)0x180D));
    hrMeas_ = hr->createCharacteristic(NimBLEUUID((uint16_t)0x2A37), NIMBLE_PROPERTY::NOTIFY);
    hrMeas_->setCallbacks(&g_hrCb);
    NimBLECharacteristic* loc =
        hr->createCharacteristic(NimBLEUUID((uint16_t)0x2A38), NIMBLE_PROPERTY::READ);
    uint8_t chest = 0x01;
    loc->setValue(&chest, 1);
    hr->start();

    batService_ = !cfg_ || cfg_->relayBattery;
    if (batService_) {
        NimBLEService* bat = server_->createService(NimBLEUUID((uint16_t)0x180F));
        batLevel_ = bat->createCharacteristic(
            NimBLEUUID((uint16_t)0x2A19),
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
        uint8_t zero = 0;
        batLevel_->setValue(&zero, 1);
        bat->start();
    } else {
        batLevel_ = nullptr;
    }

    NimBLEService* dis = server_->createService(NimBLEUUID((uint16_t)0x180A));
    auto setStr = [&](uint16_t uuid, const char* v) {
        NimBLECharacteristic* c =
            dis->createCharacteristic(NimBLEUUID(uuid), NIMBLE_PROPERTY::READ);
        c->setValue(v);
    };
    setStr(0x2A29, "MPunktBPunkt");
    setStr(0x2A24, "esp32.heartrate");
    setStr(0x2A26, FW_VERSION);
    dis->start();

    serverReady_ = true;
    Serial.println("[RELAY] GATT server ready");
}

void HrServer::startAdvertising() {
    if (!serverReady_) return;
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->stop();
    adv->reset();
    adv->addServiceUUID(NimBLEUUID((uint16_t)0x180D));
    adv->setAppearance(0x0341);
    adv->setName(name_);
    adv->setScanResponse(true);
    NimBLEDevice::startAdvertising();
    advertising_ = true;
    Serial.printf("[RELAY] advertising as %s\n", name_);
}

void HrServer::stopAdvertising() {
    if (!serverReady_) {
        advertising_ = false;
        return;
    }
    NimBLEDevice::stopAdvertising();
    advertising_ = false;
    Serial.println("[RELAY] advertising stopped");
}

void HrServer::disconnectAll() {
    if (!server_) return;
    std::vector<uint16_t> peers = server_->getPeerDevices();
    for (uint16_t h : peers) server_->disconnect(h);
}

void HrServer::refreshAdvertising() {
    if (!enabled_) return;
    if (full()) {
        if (advertising_) stopAdvertising();
    } else if (!advertising_) {
        startAdvertising();
    }
}

void HrServer::updateHold() {
    holdingStrap_ = subscribed() > 0;
    if (ble_) ble_->setRelayHold(holdingStrap_);
}

void HrServer::setEnabled(bool on) {
    if (cfg_) cfg_->relayEnabled = on;
    if (on) {
        if (enabled_ && serverReady_) {
            if (!advertising_ && !full()) startAdvertising();
            return;
        }
        enabled_ = true;
        String n = effectiveName();
        strncpy(name_, n.c_str(), sizeof(name_) - 1);
        name_[sizeof(name_) - 1] = 0;
        ensureServer();
        startAdvertising();
    } else {
        if (!enabled_ && !serverReady_) return;
        enabled_ = false;
        disconnectAll();
        stopAdvertising();
        updateHold();
        haveSample_ = false;
        staleSent_ = false;
    }
}

uint8_t HrServer::clients() const {
    if (!server_) return 0;
    return (uint8_t)server_->getConnectedCount();
}

uint8_t HrServer::subscribed() const {
    if (!hrMeas_) return 0;
    return (uint8_t)hrMeas_->getSubscribedCount();
}

bool HrServer::full() const {
    uint8_t maxc = cfg_ ? cfg_->relayMaxClients : HR_RELAY_MAX_CLIENTS;
    if (maxc < 1) maxc = 1;
    if (maxc > HR_RELAY_MAX_CLIENTS) maxc = HR_RELAY_MAX_CLIENTS;
    return clients() >= maxc;
}

void HrServer::onConnect(uint16_t) {
    refreshAdvertising();
}

void HrServer::onDisconnect(uint16_t) {
    updateHold();
    // Delay slightly — NimBLE finishes disconnect housekeeping first
    refreshAdvertising();
}

void HrServer::onHrSubscribe(uint16_t, bool) {
    updateHold();
}

void HrServer::sendEncoded(const uint8_t* data, size_t len, bool truncated) {
    if (!hrMeas_ || !enabled_) return;
    if (truncated) truncated_++;
    if (hrMeas_->getSubscribedCount() == 0) return;
    hrMeas_->setValue(data, len);
    hrMeas_->notify();
    notifySent_++;
    lastNotifyMs_ = millis();
}

void HrServer::sendStaleZero() {
    HrSample z;
    z.heartRate = 0;
    z.contactSupported = false;
    z.contact = 0;
    z.energyPresent = false;
    z.rrCount = 0;
    uint8_t buf[20];
    size_t n = buildHeartRateMeasurement(z, buf, sizeof(buf), nullptr);
    if (n) sendEncoded(buf, n, false);
}

void HrServer::updateBattery(int8_t level) {
    if (!batLevel_ || level < 0) return;
    if (level == lastBat_) return;
    lastBat_ = level;
    uint8_t v = (uint8_t)level;
    batLevel_->setValue(&v, 1);
    if (batLevel_->getSubscribedCount() > 0) batLevel_->notify();
}

void HrServer::onSample(const HrSample& s) {
    if (!enabled_ || !serverReady_) return;
    haveSample_ = true;
    lastSampleMs_ = millis();
    staleSent_ = false;

    updateBattery(s.battery);

    if (hrMeas_->getSubscribedCount() == 0) return;

    uint8_t buf[20];
    bool trunc = false;
    size_t n = buildHeartRateMeasurement(s, buf, sizeof(buf), &trunc);
    if (!n) {
        notifyFailed_++;
        return;
    }
    sendEncoded(buf, n, trunc);
}

void HrServer::loop() {
    if (!enabled_ || !serverReady_) return;
    updateHold();
    refreshAdvertising();

    if (!haveSample_ || staleSent_) return;
    if (hrMeas_->getSubscribedCount() == 0) return;
    unsigned long staleMs = (unsigned long)HR_RELAY_STALE_S * 1000UL;
    if (millis() - lastSampleMs_ >= staleMs) {
        sendStaleZero();
        staleSent_ = true;
    }
}

void HrServer::appendStatusJson(JsonObject obj) const {
    obj["supported"] = true;
    obj["enabled"] = enabled_;
    obj["advertising"] = advertising_;
    obj["name"] = name_;
    obj["clients"] = clients();
    obj["subscribed"] = subscribed();
    obj["full"] = full();
    obj["notifySent"] = notifySent_;
    obj["notifyFailed"] = notifyFailed_;
    obj["truncated"] = truncated_;
    if (lastNotifyMs_) obj["lastNotifyAgoMs"] = (long)(millis() - lastNotifyMs_);
    else obj["lastNotifyAgoMs"] = (long)-1;
    obj["holdingStrap"] = holdingStrap_;
    if (cfg_) obj["maxClients"] = cfg_->relayMaxClients;
}

void HrServer::appendIoValues(JsonObject ios) const {
    {
        JsonObject o = ios["relay_enabled"].to<JsonObject>();
        o["type"] = "sensor";
        o["value"] = enabled_ ? 1 : 0;
        o["unit"] = "";
    }
    {
        JsonObject o = ios["relay_clients"].to<JsonObject>();
        o["type"] = "sensor";
        o["value"] = (float)clients();
        o["unit"] = "";
    }
}

#endif  // HR_RELAY
