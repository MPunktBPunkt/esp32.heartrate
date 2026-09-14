#pragma once

#ifndef FW_VERSION
#define FW_VERSION "0.3.2"
#endif

#ifndef HR_RELAY
#define HR_RELAY 0
#endif
#ifndef HR_RELAY_STALE_S
#define HR_RELAY_STALE_S 10
#endif
#ifndef HR_RELAY_MAX_CLIENTS
#define HR_RELAY_MAX_CLIENTS 2
#endif

#ifndef DEVICE_NAME_DEFAULT
#define DEVICE_NAME_DEFAULT "Heart-Rate"
#endif

#ifndef HUB_HOST_DEFAULT
#define HUB_HOST_DEFAULT "192.168.178.113"
#endif

#ifndef HUB_PORT_DEFAULT
#define HUB_PORT_DEFAULT 8093
#endif

#ifndef WIFI_AP_NAME
#define WIFI_AP_NAME "ESP-HR-Setup"
#endif

#ifndef WIFI_PORTAL_TIMEOUT_S
#define WIFI_PORTAL_TIMEOUT_S 180
#endif

#ifndef RESET_BUTTON_PIN
#define RESET_BUTTON_PIN 0
#endif

#ifndef RESET_HOLD_SEC
#define RESET_HOLD_SEC 3
#endif

#ifndef HR_HISTORY_SIZE
#define HR_HISTORY_SIZE 300
#endif

#ifndef HR_RR_HISTORY_SIZE
#define HR_RR_HISTORY_SIZE 600
#endif

#ifndef HR_MAX_SCAN
#define HR_MAX_SCAN 24
#endif

#ifndef HR_IDLE_DISCONNECT_S_DEFAULT
#define HR_IDLE_DISCONNECT_S_DEFAULT 900
#endif

#ifndef HR_BEAT_HISTORY
#define HR_BEAT_HISTORY 192
#endif

#ifndef HR_MAX_ZONE_GAP_MS
#define HR_MAX_ZONE_GAP_MS 2000
#endif

#ifndef NTP_SERVER_DEFAULT
#define NTP_SERVER_DEFAULT "pool.ntp.org"
#endif

/** Europe/Berlin POSIX TZ (CET/CEST) */
#ifndef TZ_DEFAULT
#define TZ_DEFAULT "CET-1CEST,M3.5.0,M10.5.0/3"
#endif

#if defined(CONFIG_IDF_TARGET_ESP32S3)
#define HR_BOARD_ID "esp32-s3"
#define HR_BOARD_LABEL "ESP32-S3"
#define HR_HW_TYPE "esp32s3"
#else
#define HR_BOARD_ID "esp32-mini"
#define HR_BOARD_LABEL "ESP32 Mini D1"
#define HR_HW_TYPE "esp32"
#endif
