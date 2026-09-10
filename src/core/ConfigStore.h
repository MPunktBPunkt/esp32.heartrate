#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "BuildFlags.h"

class ConfigStore {
public:
    String deviceName = DEVICE_NAME_DEFAULT;
    String hubHost = HUB_HOST_DEFAULT;
    int hubPort = HUB_PORT_DEFAULT;
    bool enableHub = true;
    uint16_t heartbeatIntervalS = 30;
    bool enableMdns = true;
    uint16_t watchdogS = 300;

    String rememberedMac;
    String rememberedName;
    uint16_t idleDisconnectS = HR_IDLE_DISCONNECT_S_DEFAULT;
    bool sessionReconnect = true;
    bool autoConnect = false;  // default OFF — Gurt fuer Bike/Phone freihalten

    // Profile for zones / calories (0 = feature off)
    uint8_t userAge = 0;
    uint8_t userWeightKg = 0;
    bool userFemale = false;
    /** Last captured resting HR (BPM); 0 = unset */
    uint8_t restingHr = 0;
    /** Zone calc: "auto" (HRR if resting set), "hrmax", "hrr" (Karvonen) */
    String zoneMode = "auto";

    bool enableNtp = true;
    String ntpServer = NTP_SERVER_DEFAULT;
    String tz = TZ_DEFAULT;

    void begin();
    void load();
    void save();
    void factoryReset();
    void applyDefaults();
    void toJson(JsonObject obj) const;
    bool fromJson(JsonVariantConst obj);
    /** Effective zone basis given current profile. */
    bool zonesUseHrr() const;

private:
    static constexpr uint8_t kConfigVersion = 5;
};
