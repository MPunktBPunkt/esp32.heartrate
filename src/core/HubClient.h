#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "ConfigStore.h"

class HubClient {
public:
    void begin(ConfigStore* config);
    void loop();
    void sendNow();
    /** POST JSON to hub path (e.g. /api/session-export). Returns HTTP code or -1. */
    int postJson(const char* path, const String& payload, uint16_t timeoutMs = 20000);
    bool lastOk() const { return lastOk_; }
    unsigned long lastSuccessMs() const { return lastSuccess_; }
    void setPayloadBuilder(void (*builder)(JsonDocument& doc));

private:
    void sendHeartbeat();
    void performOta(const String& url);

    ConfigStore* config_ = nullptr;
    void (*builder_)(JsonDocument& doc) = nullptr;
    unsigned long lastHeartbeat_ = 0;
    unsigned long lastSuccess_ = 0;
    bool lastOk_ = false;
    bool otaPending_ = false;
    String otaUrl_;
};
