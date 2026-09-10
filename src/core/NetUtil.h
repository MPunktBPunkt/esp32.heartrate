#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include "ConfigStore.h"

namespace NetUtil {
String macNoColon();
String localIp();
String fmtUptime(unsigned long seconds);
String chipModel();
void sendJson(WebServer& server, int code, const JsonDocument& doc);
void sendError(WebServer& server, int code, const char* message);
bool readJsonBody(WebServer& server, JsonDocument& doc);
void addCors(WebServer& server);
String jsonToString(const JsonDocument& doc);

/** Start/reconfigure SNTP (non-blocking). Internal math stays on millis(). */
void configureNtp(const ConfigStore& cfg);
bool timeSynced();
uint32_t unixNow();
String localNowStr();
String formatUnixLocal(uint32_t unix);
}
