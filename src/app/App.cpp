#include "App.h"
#include "core/NetUtil.h"
#include "web/UiPages.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <esp_system.h>

App& App::instance() {
    static App app;
    return app;
}

void App::begin() {
    Serial.begin(115200);
    delay(300);
    Serial.printf("\n=== esp32.heartrate v%s (%s) ===\n", FW_VERSION, HR_BOARD_LABEL);
    Serial.printf("[BOOT] reset reason %d\n", (int)esp_reset_reason());

    config.begin();
    history.begin();
    beats.begin();
    session.begin(&config);
    series.begin();
    checkResetButton();
    setupWifi();

    ble.begin(&config, &history);
#if HR_RELAY
    relay.begin(&config, &ble);
#endif

    // Mount/prepare persisted archives after BLE init (reduces RAM pressure on connect)
    archive.begin();
    // Prefill last-session banner from NVS if any
    if (archive.count() > 0 && archive.get(0, lastEnded_)) haveLastEnded_ = true;

    if (config.enableMdns) {
        String mdns = "hr-" + NetUtil::macNoColon().substring(6);
        if (MDNS.begin(mdns.c_str())) Serial.printf("[mDNS] %s.local\n", mdns.c_str());
    }

    setupWeb();
    hub.begin(&config);
    hub.setPayloadBuilder([](JsonDocument& doc) { App::instance().buildHeartbeat(doc); });
    if (config.enableHub) hub.sendNow();

    // Auto-connect only if explicitly enabled (default OFF)
    if (config.autoConnect && config.rememberedMac.length() > 0) {
        Serial.println("[BLE] autoConnect ON — connecting remembered device");
        ble.connect(config.rememberedMac.c_str());
    }
}

void App::checkResetButton() {
    pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
    if (digitalRead(RESET_BUTTON_PIN) == HIGH) return;
    Serial.printf("[BOOT] Reset-Taste, halte %ds...\n", RESET_HOLD_SEC);
    unsigned long t = millis();
    while (digitalRead(RESET_BUTTON_PIN) == LOW) {
        if (millis() - t > (unsigned long)RESET_HOLD_SEC * 1000UL) {
            WiFiManager wm;
            wm.resetSettings();
            config.factoryReset();
            archive.clear();
            delay(400);
            ESP.restart();
        }
        delay(50);
    }
}

void App::setupWifi() {
    WiFi.mode(WIFI_STA);
    WiFiManager wm;
    WiFiManagerParameter pName("name", "Geraetename", config.deviceName.c_str(), 32);
    WiFiManagerParameter pHost("hub_host", "ESP-Hub IP", config.hubHost.c_str(), 40);
    WiFiManagerParameter pPort("hub_port", "Port", String(config.hubPort).c_str(), 6);
    wm.addParameter(&pName);
    wm.addParameter(&pHost);
    wm.addParameter(&pPort);
    wm.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT_S);
    wm.setAPCallback([](WiFiManager*) {
        Serial.println("[WiFi] Portal: " WIFI_AP_NAME);
    });
    if (!wm.autoConnect(WIFI_AP_NAME)) {
        delay(800);
        ESP.restart();
    }
    config.deviceName = String(pName.getValue());
    config.hubHost = String(pHost.getValue());
    config.hubPort = String(pPort.getValue()).toInt();
    config.save();
    // Modem sleep frees airtime for BLE (classic ESP32 WiFi+BLE coexistence)
    WiFi.setSleep(true);
    Serial.printf("[WiFi] IP %s  Hub %s:%d (sleep on)\n", NetUtil::localIp().c_str(), config.hubHost.c_str(), config.hubPort);
    NetUtil::configureNtp(config);
}

void App::setupWeb() {
    registerRoutes();
    server.onNotFound([this]() {
        server.sendHeader(F("Location"), F("/"), true);
        server.send(302, F("text/plain"), F(""));
    });
    server.begin();
    Serial.printf("[WEB] http://%s/\n", NetUtil::localIp().c_str());
}

void App::buildStatusJson(JsonDocument& doc) {
    doc["name"] = config.deviceName;
    doc["chip"] = NetUtil::chipModel();
    doc["board"] = HR_BOARD_ID;
    doc["boardLabel"] = HR_BOARD_LABEL;
    doc["mac"] = NetUtil::macNoColon();
    doc["ip"] = NetUtil::localIp();
    doc["rssi"] = WiFi.RSSI();
    doc["ssid"] = WiFi.SSID();
    doc["uptime"] = NetUtil::fmtUptime(millis() / 1000UL);
    doc["uptimeS"] = millis() / 1000UL;
    doc["heap"] = ESP.getFreeHeap();
    doc["hub"] = config.hubHost + ":" + String(config.hubPort);
    doc["interval"] = config.heartbeatIntervalS;
    doc["version"] = FW_VERSION;
    doc["hubOk"] = hub.lastOk();
    doc["hubEnabled"] = config.enableHub;
    doc["rememberedMac"] = config.rememberedMac;
    doc["rememberedName"] = config.rememberedName;
    doc["idleDisconnectS"] = config.idleDisconnectS;
    doc["sessionReconnect"] = config.sessionReconnect;
    doc["autoConnect"] = config.autoConnect;
    doc["userAge"] = config.userAge;
    doc["userWeightKg"] = config.userWeightKg;
    doc["userFemale"] = config.userFemale;
    doc["restingHr"] = config.restingHr;
    doc["zoneMode"] = config.zoneMode;
    doc["zonesUseHrr"] = config.zonesUseHrr();
    doc["sessionsStored"] = archive.count();
    doc["seriesPoints"] = series.count();
    doc["seriesIntervalS"] = series.intervalMs() / 1000UL;
    doc["ntpOk"] = NetUtil::timeSynced();
    doc["enableNtp"] = config.enableNtp;
    doc["unixtime"] = NetUtil::unixNow();
    doc["time"] = NetUtil::localNowStr();
    ble.appendStatusJson(doc["ble"].to<JsonObject>());
    beats.appendStatusJson(doc["beats"].to<JsonObject>());
    beats.appendLastPacketJson(doc["lastPacket"].to<JsonObject>());
    session.appendJson(doc["session"].to<JsonObject>());
    // Continuity: share of session time without large RR/notification gaps
    {
        uint32_t durS = doc["session"]["durationS"] | 0;
        uint32_t gapMs = doc["beats"]["effectiveGapMs"] | 0;
        if (durS > 0) {
            uint32_t durMs = durS * 1000UL;
            float pct = 100.0f;
            if (gapMs < durMs) pct = 100.0f * (1.0f - (float)gapMs / (float)durMs);
            else pct = 0;
            doc["beats"]["continuityPct"] = (double)((int)(pct * 10 + 0.5) / 10.0);
        }
    }
    if (haveLastEnded_) {
        lastEnded_.toJson(doc["lastSession"].to<JsonObject>());
    }
    JsonObject ios = doc["ios"].to<JsonObject>();
    ble.appendIoValues(ios);
    beats.appendIoValues(ios);
    session.appendIoValues(ios);

    // Extra continuity metrics for hub logging
    uint32_t durMs = session.durationMs();
    uint32_t effGapMs = beats.effectiveGapMs();
    float contPct = 0.0f;
    if (durMs > 0) {
        if (effGapMs < durMs) contPct = 100.0f * (1.0f - ((float)effGapMs / (float)durMs));
        else contPct = 0.0f;
    }
    {
        JsonObject c = ios["session_continuity_pct"].to<JsonObject>();
        c["type"] = "sensor";
        c["value"] = contPct;
        c["unit"] = "%";
    }
    {
        JsonObject g = ios["session_effective_gap_s"].to<JsonObject>();
        g["type"] = "sensor";
        g["value"] = (float)(effGapMs / 1000UL);
        g["unit"] = "s";
    }

    // Strap/signal fit: Polar H9 has no contact flag — derive from RR quality + gaps + RSSI
    {
        const char* fit = "n/a";
        if (ble.hasSample() || ble.sessionActive()) {
            float validPct = beats.validPct();
            uint32_t longest = beats.longestGapMs();
            int8_t rssi = ble.lastSample().rssi;
            bool contactSup = ble.lastSample().contactSupported;
            uint8_t contact = ble.lastSample().contact;
            if (contactSup && contact == 2) fit = "poor";
            else if (validPct >= 95.0f && longest < 5000 && rssi > -80) fit = "good";
            else if (validPct >= 85.0f && longest < 20000 && rssi > -90) fit = "fair";
            else if (validPct >= 70.0f || rssi > -95) fit = "weak";
            else fit = "poor";
        }
        doc["strapFit"] = fit;
        JsonObject f = ios["strap_fit"].to<JsonObject>();
        f["type"] = "sensor";
        f["value"] = fit;
        f["unit"] = "";
    }

#if HR_RELAY
    relay.appendStatusJson(doc["relay"].to<JsonObject>());
    relay.appendIoValues(ios);
#else
    {
        JsonObject r = doc["relay"].to<JsonObject>();
        r["supported"] = false;
    }
#endif

}

void App::buildHeartbeat(JsonDocument& doc) {
    doc["mac"] = NetUtil::macNoColon();
    doc["name"] = config.deviceName;
    doc["hwType"] = HR_HW_TYPE;
    doc["chipModel"] = NetUtil::chipModel();
    doc["version"] = FW_VERSION;
    doc["ip"] = NetUtil::localIp();
    doc["rssi"] = WiFi.RSSI();
    doc["uptime"] = millis() / 1000UL;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["freeSketch"] = ESP.getFreeSketchSpace();
    doc["fwType"] = "heartrate";
    doc["board"] = HR_BOARD_ID;
    JsonObject ios = doc["ios"].to<JsonObject>();
    ble.appendIoValues(ios);
    beats.appendIoValues(ios);
    session.appendIoValues(ios);

    // Extra continuity metrics (also exposed via /api/status ios)
    uint32_t durMs = session.durationMs();
    uint32_t effGapMs = beats.effectiveGapMs();
    float contPct = 0.0f;
    if (durMs > 0) {
        if (effGapMs < durMs) contPct = 100.0f * (1.0f - ((float)effGapMs / (float)durMs));
        else contPct = 0.0f;
    }
    {
        JsonObject c = ios["session_continuity_pct"].to<JsonObject>();
        c["type"] = "sensor";
        c["value"] = contPct;
        c["unit"] = "%";
    }
    {
        JsonObject g = ios["session_effective_gap_s"].to<JsonObject>();
        g["type"] = "sensor";
        g["value"] = (float)(effGapMs / 1000UL);
        g["unit"] = "s";
    }

    // Strap/signal fit: Polar H9 has no contact flag — derive from RR quality + gaps + RSSI
    {
        const char* fit = "n/a";
        if (ble.hasSample() || ble.sessionActive()) {
            float validPct = beats.validPct();
            uint32_t longest = beats.longestGapMs();
            int8_t rssi = ble.lastSample().rssi;
            bool contactSup = ble.lastSample().contactSupported;
            uint8_t contact = ble.lastSample().contact;
            if (contactSup && contact == 2) fit = "poor";
            else if (validPct >= 95.0f && longest < 5000 && rssi > -80) fit = "good";
            else if (validPct >= 85.0f && longest < 20000 && rssi > -90) fit = "fair";
            else if (validPct >= 70.0f || rssi > -95) fit = "weak";
            else fit = "poor";
        }
        doc["strapFit"] = fit;
        JsonObject f = ios["strap_fit"].to<JsonObject>();
        f["type"] = "sensor";
        f["value"] = fit;
        f["unit"] = "";
    }

#if HR_RELAY
    relay.appendIoValues(ios);
#endif

}

String App::statusString() {
    JsonDocument doc;
    buildStatusJson(doc);
    return NetUtil::jsonToString(doc);
}

bool App::buildExportDoc(JsonDocument& doc, bool includeSeries) {
    doc.clear();
    doc["type"] = "heartrate-session-export";
    doc["version"] = FW_VERSION;
    doc["board"] = HR_BOARD_ID;
    doc["mac"] = NetUtil::macNoColon();
    doc["name"] = config.deviceName;
    doc["exportedUnix"] = NetUtil::unixNow();
    doc["exportedAt"] = NetUtil::localNowStr();
    doc["hubEnabled"] = config.enableHub;

    bool haveAnything = false;

    if (session.active()) {
        session.appendJson(doc["session"].to<JsonObject>());
        SessionSummary sum;
        if (session.buildSummary(sum)) {
            const char* peer = ble.peerName();
            if (peer && peer[0]) {
                strncpy(sum.peer, peer, sizeof(sum.peer) - 1);
                sum.peer[sizeof(sum.peer) - 1] = 0;
            }
            sum.endedUnix = NetUtil::unixNow();
            sum.reconnectCount = ble.reconnectOkCount();
            uint32_t gapMs = beats.totalGapMs();
            if (sum.durationS > 0) {
                uint32_t durMs = sum.durationS * 1000UL;
                float pct = (gapMs < durMs) ? (100.0f * (1.0f - (float)gapMs / (float)durMs)) : 0;
                if (pct < 0) pct = 0;
                if (pct > 100) pct = 100;
                sum.continuityPct = (uint8_t)(pct + 0.5f);
            }
            int8_t rMin = 0, rMax = 0, rAvg = 0;
            if (ble.linkRssiStats(rMin, rMax, rAvg)) {
                sum.rssiMin = rMin;
                sum.rssiAvg = rAvg;
            }
            sum.toJson(doc["summary"].to<JsonObject>());
        }
        haveAnything = true;
    } else if (haveLastEnded_) {
        lastEnded_.toJson(doc["summary"].to<JsonObject>());
        haveAnything = true;
    }

    doc["seriesIntervalS"] = series.intervalMs() / 1000UL;
    doc["seriesPoints"] = series.count();
    if (includeSeries) series.appendHrJson(doc["hrSeries"].to<JsonArray>());
    if (series.count()) haveAnything = true;
    return haveAnything;
}

String App::buildExportCsv() {
    String out;
    out.reserve(series.count() * 12 + 64);
    out += F("# heartrate session export\n");
    out += F("# version=");
    out += FW_VERSION;
    out += '\n';
    series.appendCsv(out);
    return out;
}

void App::sseSend(const String& data) {
    if (!sseClient_ || !sseClient_.connected()) return;
    sseClient_.print("data: ");
    sseClient_.print(data);
    sseClient_.print("\n\n");
    sseClient_.flush();
}

void App::handleEvents() {
    if (sseClient_ && sseClient_.connected()) sseClient_.stop();
    sseClient_ = server.client();
    sseClient_.print(F("HTTP/1.1 200 OK\r\n"
                       "Content-Type: text/event-stream\r\n"
                       "Cache-Control: no-cache\r\n"
                       "Connection: keep-alive\r\n"
                       "Access-Control-Allow-Origin: *\r\n\r\n"));
    sseClient_.flush();
    sseSend(statusString());
}

void App::handleMain() {
    server.sendHeader(F("Cache-Control"), F("no-store, no-cache, must-revalidate, max-age=0"));
    server.sendHeader(F("Pragma"), F("no-cache"));
    server.sendHeader(F("Expires"), F("0"));
    server.send_P(200, "text/html", PAGE_MAIN);
}

void App::handleOtaUpload() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("[OTA] Start %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) Serial.printf("[OTA] OK %u Bytes\n", upload.totalSize);
        else Update.printError(Serial);
    }
}

void App::handleOtaUploadFinish() {
    if (Update.hasError()) server.send(500, F("text/plain"), F("OTA fehlgeschlagen!"));
    else server.send(200, F("text/plain"), F("OK - Neustart..."));
    delay(400);
    ESP.restart();
}

void App::registerRoutes() {
    server.on("/", HTTP_GET, [this]() { handleMain(); });
    server.on("/live", HTTP_GET, [this]() { handleMain(); });
    server.on("/devices", HTTP_GET, [this]() { handleMain(); });
    server.on("/history", HTTP_GET, [this]() { handleMain(); });
    server.on("/config", HTTP_GET, [this]() { handleMain(); });
    server.on("/ota", HTTP_GET, [this]() { handleMain(); });
    server.on("/events", HTTP_GET, [this]() { handleEvents(); });
    server.on("/ota-upload", HTTP_POST, [this]() { handleOtaUploadFinish(); }, [this]() { handleOtaUpload(); });

    server.on("/api/status", HTTP_GET, [this]() {
        JsonDocument doc;
        buildStatusJson(doc);
        NetUtil::sendJson(server, 200, doc);
    });

    server.on("/api/history", HTTP_GET, [this]() {
        // Cap payload size — full ring is large; UI only needs the live window
        JsonDocument doc;
        history.toJson(HistoryStore::HR_BPM, doc["hr"].to<JsonArray>(), 360);
        history.rrToJson(doc["rr"].to<JsonArray>(), 720);
        history.toJson(HistoryStore::BATTERY, doc["battery"].to<JsonArray>(), 120);
        history.toJson(HistoryStore::RSSI, doc["rssi"].to<JsonArray>(), 360);
        NetUtil::sendJson(server, 200, doc);
    });

    server.on("/api/ble/devices", HTTP_GET, [this]() {
        JsonDocument doc;
        doc["state"] = ble.stateName();
        doc["scanning"] = ble.scanning();
        doc["rememberedMac"] = config.rememberedMac;
        doc["rememberedName"] = config.rememberedName;
        ble.scanToJson(doc["devices"].to<JsonArray>());
        NetUtil::sendJson(server, 200, doc);
    });

    server.on("/api/ble/scan/start", HTTP_POST, [this]() {
        ble.startScan();
        JsonDocument doc;
        doc["ok"] = true;
        doc["state"] = ble.stateName();
        NetUtil::sendJson(server, 200, doc);
    });

    server.on("/api/ble/scan/stop", HTTP_POST, [this]() {
        ble.stopScan();
        JsonDocument doc;
        doc["ok"] = true;
        doc["state"] = ble.stateName();
        NetUtil::sendJson(server, 200, doc);
    });

    server.on("/api/ble/connect", HTTP_POST, [this]() {
        JsonDocument body;
        if (!NetUtil::readJsonBody(server, body)) return;
        const char* mac = body["mac"] | "";
        if (!mac[0] && config.rememberedMac.length()) mac = config.rememberedMac.c_str();
        if (!mac[0]) {
            NetUtil::sendError(server, 400, "mac fehlt");
            return;
        }
        bool ok = ble.connect(mac);
        JsonDocument doc;
        doc["ok"] = ok;
        doc["queued"] = ok;
        doc["state"] = ble.stateName();
        doc["error"] = ble.lastError();
        // Connect runs asynchronously in loop(); client should watch SSE/status
        NetUtil::sendJson(server, ok ? 200 : 400, doc);
    });

    server.on("/api/ble/disconnect", HTTP_POST, [this]() {
        ble.disconnect();
        JsonDocument doc;
        doc["ok"] = true;
        doc["state"] = ble.stateName();
        NetUtil::sendJson(server, 200, doc);
    });

    server.on("/api/ble/remember", HTTP_POST, [this]() {
        JsonDocument body;
        if (!NetUtil::readJsonBody(server, body)) return;
        String mac = body["mac"] | "";
        String name = body["name"] | "";
        if (mac.length()) {
            config.rememberedMac = mac;
            config.rememberedName = name.length() ? name : "HR Sensor";
            config.save();
        } else {
            ble.rememberCurrent();
        }
        JsonDocument doc;
        doc["ok"] = true;
        doc["rememberedMac"] = config.rememberedMac;
        doc["rememberedName"] = config.rememberedName;
        NetUtil::sendJson(server, 200, doc);
    });

    server.on("/api/ble/forget", HTTP_POST, [this]() {
        ble.forgetRemembered();
        JsonDocument doc;
        doc["ok"] = true;
        NetUtil::sendJson(server, 200, doc);
    });

    server.on("/api/session/mode", HTTP_POST, [this]() {
        JsonDocument body;
        if (!NetUtil::readJsonBody(server, body)) return;
        const char* mode = body["mode"] | "";
        SessionMode m = sessionModeFromName(mode);
        if (!mode[0]) {
            NetUtil::sendError(server, 400, "mode fehlt (rest|activity|training|recovery|idle)");
            return;
        }
        session.setMode(m);
        JsonDocument doc;
        doc["ok"] = true;
        doc["mode"] = sessionModeName(session.mode());
        NetUtil::sendJson(server, 200, doc);
    });

    server.on("/api/session/baseline", HTTP_POST, [this]() {
        JsonDocument body;
        if (!NetUtil::readJsonBody(server, body)) return;
        const char* action = body["action"] | "capture";
        JsonDocument doc;
        if (strcasecmp(action, "clear") == 0) {
            session.clearBaseline();
            doc["ok"] = true;
            doc["restingHr"] = 0;
        } else if (strcasecmp(action, "guide") == 0) {
            if (!session.active()) {
                NetUtil::sendError(server, 400, "keine aktive BLE-Session");
                return;
            }
            if (!session.startGuidedBaseline()) {
                NetUtil::sendError(server, 400, "Guided Baseline konnte nicht starten");
                return;
            }
            doc["ok"] = true;
            doc["mode"] = sessionModeName(session.mode());
            doc["guided"] = true;
        } else if (strcasecmp(action, "cancel") == 0) {
            session.cancelGuidedBaseline();
            doc["ok"] = true;
            doc["cancelled"] = true;
        } else if (strcasecmp(action, "capture") == 0) {
            bool ok = session.captureBaseline(false);
            if (!ok) {
                NetUtil::sendError(server, 400, "keine Rest-Daten (Rest-Modus, >1 min nach Warmup)");
                return;
            }
            doc["ok"] = true;
            doc["restingHr"] = config.restingHr;
        } else {
            NetUtil::sendError(server, 400, "action fehlt (guide|capture|clear|cancel)");
            return;
        }
        NetUtil::sendJson(server, 200, doc);
    });

    server.on("/api/sessions", HTTP_GET, [this]() {
        JsonDocument doc;
        doc["count"] = archive.count();
        doc["max"] = SessionArchive::kMax;
        doc["nextId"] = archive.nextId();
        archive.appendListJson(doc["sessions"].to<JsonArray>());
        NetUtil::sendJson(server, 200, doc);
    });

    server.on("/api/sessions/clear", HTTP_POST, [this]() {
        archive.clear();
        JsonDocument doc;
        doc["ok"] = true;
        doc["count"] = 0;
        NetUtil::sendJson(server, 200, doc);
    });

    server.on("/api/session/export", HTTP_GET, [this]() {
        JsonDocument doc;
        if (!buildExportDoc(doc, true)) {
            NetUtil::sendError(server, 404, "keine Session-Daten");
            return;
        }
        String body = NetUtil::jsonToString(doc);
        server.sendHeader(F("Content-Disposition"), F("attachment; filename=\"hr-session.json\""));
        server.sendHeader(F("Cache-Control"), F("no-store"));
        server.send(200, F("application/json"), body);
    });

    server.on("/api/session/export.csv", HTTP_GET, [this]() {
        if (!series.count() && !session.active() && !haveLastEnded_) {
            NetUtil::sendError(server, 404, "keine Serie");
            return;
        }
        String body = buildExportCsv();
        server.sendHeader(F("Content-Disposition"), F("attachment; filename=\"hr-session.csv\""));
        server.sendHeader(F("Cache-Control"), F("no-store"));
        server.send(200, F("text/csv"), body);
    });

    server.on("/api/session/export/send", HTTP_POST, [this]() {
        JsonDocument doc;
        if (!buildExportDoc(doc, true)) {
            NetUtil::sendError(server, 404, "keine Session-Daten");
            return;
        }
        if (!config.enableHub) {
            NetUtil::sendError(server, 400, "Hub deaktiviert");
            return;
        }
        String payload = NetUtil::jsonToString(doc);
        int code = hub.postJson("/api/session-export", payload, 25000);
        JsonDocument resp;
        resp["ok"] = (code == 200);
        resp["http"] = code;
        resp["bytes"] = payload.length();
        resp["seriesPoints"] = series.count();
        if (code != 200) resp["error"] = "Hub-Antwort nicht OK";
        NetUtil::sendJson(server, code == 200 ? 200 : 502, resp);
    });

    server.on("/api/config/get", HTTP_GET, [this]() {
        JsonDocument doc;
        config.toJson(doc.to<JsonObject>());
        NetUtil::sendJson(server, 200, doc);
    });

    server.on("/api/config/save", HTTP_POST, [this]() {
        JsonDocument body;
        if (!NetUtil::readJsonBody(server, body)) return;
        if (!config.fromJson(body.as<JsonVariantConst>())) {
            NetUtil::sendError(server, 400, "ungueltig");
            return;
        }
        config.save();
        NetUtil::configureNtp(config);
        if (config.enableHub) hub.sendNow();
        JsonDocument doc;
        doc["ok"] = true;
        config.toJson(doc["config"].to<JsonObject>());
        NetUtil::sendJson(server, 200, doc);
    });

    server.on("/api/system/restart", HTTP_POST, [this]() {
        restartPending_ = true;
        restartAt_ = millis() + 400;
        JsonDocument doc;
        doc["ok"] = true;
        NetUtil::sendJson(server, 200, doc);
    });

    server.on("/api/relay", HTTP_GET, [this]() {
#if HR_RELAY
        JsonDocument doc;
        relay.appendStatusJson(doc.to<JsonObject>());
        NetUtil::sendJson(server, 200, doc);
#else
        NetUtil::sendError(server, 501, "relay unsupported");
#endif
    });

    server.on("/api/relay", HTTP_POST, [this]() {
#if HR_RELAY
        JsonDocument body;
        if (!NetUtil::readJsonBody(server, body)) return;
        bool restartRequired = false;
        if (!body["name"].isNull()) {
            String newName = body["name"].as<String>();
            if (newName != config.relayName) {
                config.relayName = newName;
                restartRequired = true;
            }
        }
        if (!body["maxClients"].isNull()) {
            uint8_t mc = body["maxClients"].as<uint8_t>();
            if (mc < 1) mc = 1;
            if (mc > HR_RELAY_MAX_CLIENTS) mc = HR_RELAY_MAX_CLIENTS;
            config.relayMaxClients = mc;
        }
        if (!body["enabled"].isNull()) {
            bool en = body["enabled"].as<bool>();
            relay.setEnabled(en);
            config.relayEnabled = en;
        }
        if (!body["battery"].isNull()) {
            config.relayBattery = body["battery"].as<bool>();
        }
        config.save();
        JsonDocument doc;
        relay.appendStatusJson(doc.to<JsonObject>());
        doc["ok"] = true;
        if (restartRequired) doc["restartRequired"] = true;
        NetUtil::sendJson(server, 200, doc);
#else
        NetUtil::sendError(server, 501, "relay unsupported");
#endif
    });
}

void App::loop() {
    server.handleClient();
    ble.loop();
#if HR_RELAY
    relay.loop();
#endif
    hub.loop();

    bool sess = ble.sessionActive();
    if (sess != lastBleSession_) {
        if (!sess && lastBleSession_) {
            // end first so endedMs_ is set, then archive snapshot
            session.setSessionActive(false);
            SessionSummary sum;
            if (session.buildSummary(sum)) {
                const char* peer = ble.peerName();
                if (peer && peer[0]) {
                    strncpy(sum.peer, peer, sizeof(sum.peer) - 1);
                    sum.peer[sizeof(sum.peer) - 1] = 0;
                } else if (config.rememberedName.length()) {
                    strncpy(sum.peer, config.rememberedName.c_str(), sizeof(sum.peer) - 1);
                    sum.peer[sizeof(sum.peer) - 1] = 0;
                }
                sum.endedUnix = NetUtil::unixNow();
                sum.reconnectCount = ble.reconnectOkCount();
                uint32_t gapMs = beats.totalGapMs();
                if (sum.durationS > 0) {
                    uint32_t durMs = sum.durationS * 1000UL;
                    float pct = (gapMs < durMs) ? (100.0f * (1.0f - (float)gapMs / (float)durMs)) : 0;
                    if (pct < 0) pct = 0;
                    if (pct > 100) pct = 100;
                    sum.continuityPct = (uint8_t)(pct + 0.5f);
                }
                int8_t rMin = 0, rMax = 0, rAvg = 0;
                if (ble.linkRssiStats(rMin, rMax, rAvg)) {
                    sum.rssiMin = rMin;
                    sum.rssiAvg = rAvg;
                }
                archive.push(sum);
                lastEnded_ = sum;
                // id assigned inside push — reload newest for correct id
                archive.get(0, lastEnded_);
                haveLastEnded_ = true;
            }
        } else {
            session.setSessionActive(sess);
            if (sess) {
                beats.reset();
                series.reset();
            }
        }
        lastBleSession_ = sess;
    }

    unsigned long now = millis();
    bool fresh = ble.takeSampleDirty();
    if (fresh) {
        beats.onPacket(ble.lastSample());
        session.onSample(ble.lastSample(), beats);
        if (session.active()) series.onSample(ble.lastSample().heartRate, session.durationMs());
#if HR_RELAY
        relay.onSample(ble.lastSample());
#endif
    }

    unsigned long sseInterval = (ble.state() == BleState::Ready) ? 1000UL : 3000UL;
    if (sseClient_ && sseClient_.connected() && (fresh || now - lastSse_ >= sseInterval)) {
        lastSse_ = now;
        sseSend(statusString());
    }

    if (now - lastWifiCheck_ > 5000UL) {
        lastWifiCheck_ = now;
        if (WiFi.status() != WL_CONNECTED) WiFi.reconnect();
    }

    if (config.enableHub && config.watchdogS > 0) {
        if (now - hub.lastSuccessMs() > (unsigned long)config.watchdogS * 1000UL) {
            Serial.println("[WD] Hub silent, restart");
            delay(200);
            ESP.restart();
        }
    }

    if (restartPending_ && now >= restartAt_) ESP.restart();
    delay(2);
}
