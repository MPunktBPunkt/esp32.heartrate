#include "BleTypes.h"

bool parseHeartRateMeasurement(const uint8_t* data, size_t len, HrSample& out) {
    if (!data || len < 2) return false;
    out.rrCount = 0;
    out.energyPresent = false;
    out.energyExpended = 0;

    uint8_t flags = data[0];
    out.flags = flags;
    bool hr16 = flags & 0x01;
    uint8_t contactBits = (flags >> 1) & 0x03;
    bool energy = flags & 0x08;
    bool rr = flags & 0x10;
    // Spec: bit2 = contact feature supported; bit1 = detected when bit2 set
    out.contactSupported = (contactBits & 0x02) != 0;

    size_t idx = 1;
    if (hr16) {
        if (len < idx + 2) return false;
        out.heartRate = (uint16_t)data[idx] | ((uint16_t)data[idx + 1] << 8);
        idx += 2;
    } else {
        if (len < idx + 1) return false;
        out.heartRate = data[idx++];
    }

    if (!out.contactSupported) out.contact = 0;
    else if (contactBits == 3) out.contact = 1;  // supported + detected
    else out.contact = 2;                        // supported + not detected

    if (energy) {
        if (len < idx + 2) return false;
        out.energyPresent = true;
        out.energyExpended = (uint16_t)data[idx] | ((uint16_t)data[idx + 1] << 8);
        idx += 2;
    }

    if (rr) {
        while (idx + 1 < len && out.rrCount < 8) {
            uint16_t raw = (uint16_t)data[idx] | ((uint16_t)data[idx + 1] << 8);
            idx += 2;
            out.rrRaw[out.rrCount++] = raw;
        }
    }

    out.tMs = millis();
    return out.heartRate > 0;
}

size_t buildHeartRateMeasurement(const HrSample& s, uint8_t* out, size_t cap,
                                 bool* truncatedOut) {
    if (truncatedOut) *truncatedOut = false;
    if (!out || cap < 2) return 0;

    const bool hr16 = s.heartRate > 255;
    uint8_t flags = 0;
    if (hr16) flags |= 0x01;
    if (s.contactSupported) {
        flags |= 0x04;  // bit2 supported
        if (s.contact == 1) flags |= 0x02;  // bit1 detected
    }
    if (s.energyPresent) flags |= 0x08;

    size_t fixed = 1;  // flags
    fixed += hr16 ? 2 : 1;
    if (s.energyPresent) fixed += 2;
    if (fixed > cap) return 0;

    uint8_t maxRr = 0;
    if (cap > fixed) maxRr = (uint8_t)((cap - fixed) / 2);
    uint8_t useRr = s.rrCount;
    if (useRr > 8) useRr = 8;
    if (useRr > maxRr) {
        if (truncatedOut) *truncatedOut = true;
        useRr = maxRr;
    }
    if (useRr > 0) flags |= 0x10;

    size_t idx = 0;
    out[idx++] = flags;
    if (hr16) {
        out[idx++] = (uint8_t)(s.heartRate & 0xFF);
        out[idx++] = (uint8_t)((s.heartRate >> 8) & 0xFF);
    } else {
        out[idx++] = (uint8_t)(s.heartRate & 0xFF);
    }
    if (s.energyPresent) {
        out[idx++] = (uint8_t)(s.energyExpended & 0xFF);
        out[idx++] = (uint8_t)((s.energyExpended >> 8) & 0xFF);
    }
    for (uint8_t i = 0; i < useRr; i++) {
        out[idx++] = (uint8_t)(s.rrRaw[i] & 0xFF);
        out[idx++] = (uint8_t)((s.rrRaw[i] >> 8) & 0xFF);
    }
    return idx;
}
