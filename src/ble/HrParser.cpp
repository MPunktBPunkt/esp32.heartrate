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
