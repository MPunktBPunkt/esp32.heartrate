#include "ConfigStore.h"
#include <Preferences.h>

static Preferences prefs;

void ConfigStore::applyDefaults() {
    deviceName = DEVICE_NAME_DEFAULT;
    hubHost = HUB_HOST_DEFAULT;
    hubPort = HUB_PORT_DEFAULT;
    enableHub = true;
    heartbeatIntervalS = 30;
    enableMdns = true;
    watchdogS = 300;
    rememberedMac = "";
    rememberedName = "";
    idleDisconnectS = HR_IDLE_DISCONNECT_S_DEFAULT;
    sessionReconnect = true;
    autoConnect = false;
    userAge = 0;
    userWeightKg = 0;
    userFemale = false;
    restingHr = 0;
    zoneMode = "auto";
    enableNtp = true;
    ntpServer = NTP_SERVER_DEFAULT;
    tz = TZ_DEFAULT;
}

void ConfigStore::begin() {
    applyDefaults();
    load();
}

void ConfigStore::load() {
    prefs.begin("esphub", true);
    deviceName = prefs.getString("name", deviceName);
    hubHost = prefs.getString("hub_host", hubHost);
    hubPort = prefs.getInt("hub_port", hubPort);
    prefs.end();

    prefs.begin("hr", true);
    uint8_t ver = prefs.getUChar("cfg_ver", 0);
    if (ver == 0) {
        prefs.end();
        return;
    }
    enableHub = prefs.getBool("en_hub", enableHub);
    heartbeatIntervalS = prefs.getUShort("hb_s", heartbeatIntervalS);
    enableMdns = prefs.getBool("en_mdns", enableMdns);
    watchdogS = prefs.getUShort("wdt_s", watchdogS);
    rememberedMac = prefs.getString("rem_mac", rememberedMac);
    rememberedName = prefs.getString("rem_name", rememberedName);
    idleDisconnectS = prefs.getUShort("idle_s", idleDisconnectS);
    sessionReconnect = prefs.getBool("sess_rc", sessionReconnect);
    autoConnect = prefs.getBool("auto_c", false);  // force safe default if missing
    userAge = prefs.getUChar("age", userAge);
    userWeightKg = prefs.getUChar("weight", userWeightKg);
    userFemale = prefs.getBool("female", userFemale);
    restingHr = prefs.getUChar("rest_hr", restingHr);
    zoneMode = prefs.getString("zmode", zoneMode);
    enableNtp = prefs.getBool("en_ntp", enableNtp);
    ntpServer = prefs.getString("ntp", ntpServer);
    tz = prefs.getString("tz", tz);
    prefs.end();
    if (zoneMode != "hrmax" && zoneMode != "hrr" && zoneMode != "auto") zoneMode = "auto";
}

void ConfigStore::save() {
    prefs.begin("esphub", false);
    prefs.putString("name", deviceName);
    prefs.putString("hub_host", hubHost);
    prefs.putInt("hub_port", hubPort);
    prefs.end();

    prefs.begin("hr", false);
    prefs.putUChar("cfg_ver", kConfigVersion);
    prefs.putBool("en_hub", enableHub);
    prefs.putUShort("hb_s", heartbeatIntervalS);
    prefs.putBool("en_mdns", enableMdns);
    prefs.putUShort("wdt_s", watchdogS);
    prefs.putString("rem_mac", rememberedMac);
    prefs.putString("rem_name", rememberedName);
    prefs.putUShort("idle_s", idleDisconnectS);
    prefs.putBool("sess_rc", sessionReconnect);
    prefs.putBool("auto_c", autoConnect);
    prefs.putUChar("age", userAge);
    prefs.putUChar("weight", userWeightKg);
    prefs.putBool("female", userFemale);
    prefs.putUChar("rest_hr", restingHr);
    prefs.putString("zmode", zoneMode);
    prefs.putBool("en_ntp", enableNtp);
    prefs.putString("ntp", ntpServer);
    prefs.putString("tz", tz);
    prefs.end();
}

void ConfigStore::factoryReset() {
    prefs.begin("esphub", false);
    prefs.clear();
    prefs.end();
    prefs.begin("hr", false);
    prefs.clear();
    prefs.end();
    applyDefaults();
}

void ConfigStore::toJson(JsonObject obj) const {
    obj["deviceName"] = deviceName;
    obj["hubHost"] = hubHost;
    obj["hubPort"] = hubPort;
    obj["enableHub"] = enableHub;
    obj["heartbeatIntervalS"] = heartbeatIntervalS;
    obj["enableMdns"] = enableMdns;
    obj["watchdogS"] = watchdogS;
    obj["rememberedMac"] = rememberedMac;
    obj["rememberedName"] = rememberedName;
    obj["idleDisconnectS"] = idleDisconnectS;
    obj["sessionReconnect"] = sessionReconnect;
    obj["autoConnect"] = autoConnect;
    obj["userAge"] = userAge;
    obj["userWeightKg"] = userWeightKg;
    obj["userFemale"] = userFemale;
    obj["restingHr"] = restingHr;
    obj["zoneMode"] = zoneMode;
    obj["zonesUseHrr"] = zonesUseHrr();
    obj["enableNtp"] = enableNtp;
    obj["ntpServer"] = ntpServer;
    obj["tz"] = tz;
    obj["board"] = HR_BOARD_ID;
    obj["boardLabel"] = HR_BOARD_LABEL;
}

bool ConfigStore::zonesUseHrr() const {
    if (zoneMode == "hrmax") return false;
    if (zoneMode == "hrr") return restingHr > 0;
    // auto
    return restingHr > 0;
}

static String jsonString(JsonVariantConst v, const String& fallback) {
    if (v.isNull()) return fallback;
    return v.as<String>();
}

bool ConfigStore::fromJson(JsonVariantConst obj) {
    if (!obj.is<JsonObjectConst>()) return false;
    deviceName = jsonString(obj["deviceName"], deviceName);
    hubHost = jsonString(obj["hubHost"], hubHost);
    if (!obj["hubPort"].isNull()) hubPort = obj["hubPort"].as<int>();
    if (!obj["enableHub"].isNull()) enableHub = obj["enableHub"].as<bool>();
    if (!obj["heartbeatIntervalS"].isNull()) heartbeatIntervalS = obj["heartbeatIntervalS"].as<uint16_t>();
    if (!obj["enableMdns"].isNull()) enableMdns = obj["enableMdns"].as<bool>();
    if (!obj["watchdogS"].isNull()) watchdogS = obj["watchdogS"].as<uint16_t>();
    rememberedMac = jsonString(obj["rememberedMac"], rememberedMac);
    rememberedName = jsonString(obj["rememberedName"], rememberedName);
    if (!obj["idleDisconnectS"].isNull()) idleDisconnectS = obj["idleDisconnectS"].as<uint16_t>();
    if (!obj["sessionReconnect"].isNull()) sessionReconnect = obj["sessionReconnect"].as<bool>();
    if (!obj["autoConnect"].isNull()) autoConnect = obj["autoConnect"].as<bool>();
    if (!obj["userAge"].isNull()) userAge = obj["userAge"].as<uint8_t>();
    if (!obj["userWeightKg"].isNull()) userWeightKg = obj["userWeightKg"].as<uint8_t>();
    if (!obj["userFemale"].isNull()) userFemale = obj["userFemale"].as<bool>();
    if (!obj["restingHr"].isNull()) restingHr = obj["restingHr"].as<uint8_t>();
    zoneMode = jsonString(obj["zoneMode"], zoneMode);
    if (zoneMode != "hrmax" && zoneMode != "hrr" && zoneMode != "auto") zoneMode = "auto";
    if (!obj["enableNtp"].isNull()) enableNtp = obj["enableNtp"].as<bool>();
    ntpServer = jsonString(obj["ntpServer"], ntpServer);
    tz = jsonString(obj["tz"], tz);
    if (ntpServer.length() == 0) ntpServer = NTP_SERVER_DEFAULT;
    if (tz.length() == 0) tz = TZ_DEFAULT;
    if (heartbeatIntervalS < 5) heartbeatIntervalS = 5;
    if (userAge > 120) userAge = 120;
    if (userWeightKg > 250) userWeightKg = 250;
    if (restingHr > 0 && restingHr < 30) restingHr = 30;
    if (restingHr > 120) restingHr = 120;
    return true;
}
