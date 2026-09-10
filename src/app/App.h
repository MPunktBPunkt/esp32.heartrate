#pragma once

#include <ArduinoJson.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include "core/ConfigStore.h"
#include "core/HistoryStore.h"
#include "core/HubClient.h"
#include "core/BeatTimeline.h"
#include "core/SessionStats.h"
#include "core/SessionArchive.h"
#include "core/SessionSeries.h"
#include "ble/BleCentral.h"
#if HR_RELAY
#include "ble/HrServer.h"
#endif

class App {
public:
    static App& instance();

    ConfigStore config;
    HistoryStore history;
    HubClient hub;
    BleCentral ble;
#if HR_RELAY
    HrServer relay;
#endif
    BeatTimeline beats;
    SessionStats session;
    SessionArchive archive;
    SessionSeries series;
    WebServer server{80};
    SessionSummary lastEnded_;
    bool haveLastEnded_ = false;

    void begin();
    void loop();
    void buildStatusJson(JsonDocument& doc);
    void buildHeartbeat(JsonDocument& doc);
    String statusString();

private:
    App() {}
    void checkResetButton();
    void setupWifi();
    void setupWeb();
    void registerRoutes();
    void handleMain();
    void handleOtaUpload();
    void handleOtaUploadFinish();
    void handleEvents();
    void sseSend(const String& data);
    bool buildExportDoc(JsonDocument& doc, bool includeSeries);
    String buildExportCsv();

    WiFiClient sseClient_;
    unsigned long lastSse_ = 0;
    unsigned long lastWifiCheck_ = 0;
    bool restartPending_ = false;
    unsigned long restartAt_ = 0;
    bool lastBleSession_ = false;
};
