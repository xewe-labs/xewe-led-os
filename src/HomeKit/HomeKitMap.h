// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/src/HomeKit/HomeKitMap.h
//
// Pure value mapping between the led module (bytes 0-255) and HomeKit / Home Assistant units
// (hue 0-360 degrees, saturation and brightness 0-100 %). Standard library only: host-tested by
// tests/unit/host_homekit_map.cpp. Every formula is computed in float on purpose: double
// arithmetic rounds some half-way inputs one lower.
#pragma once

#include <cmath>
#include <cstdint>

namespace led_map {

inline uint8_t clamp_byte(long v) {
    return static_cast<uint8_t>(v < 0 ? 0 : (v > 255 ? 255 : v));
}

// led -> HomeKit / HA. The results are whole numbers (rounded in float).
inline int hue_to_deg(uint8_t hue8) {
    return static_cast<int>(std::round((hue8 / 255.0f) * 360.0f));
}
inline int sat_to_pct(uint8_t sat8) {
    return static_cast<int>(std::round((sat8 / 255.0f) * 100.0f));
}
// Brightness bytes 0 and 1 give 0 %, below HomeKit's 1..100 range: HomeSpan stores the value
// anyway (it only logs a warning), so the Home app sees the strip's real brightness.
inline int bri_to_pct(uint8_t bri8) {
    return static_cast<int>(std::round((bri8 / 255.0f) * 100.0f));
}

// HomeKit / HA -> led. Inputs outside their range (and NaN) are clamped first; inside the range
// the result is the plain float formula.
inline uint8_t deg_to_hue(float deg) {
    if (!(deg >= 0.0f))
        deg = 0.0f; // also NaN
    if (deg > 360.0f)
        deg = 360.0f;
    return static_cast<uint8_t>(std::round((deg / 360.0f) * 255.0f));
}
inline uint8_t pct_to_byte(float pct) {
    if (!(pct >= 0.0f))
        pct = 0.0f; // also NaN
    if (pct > 100.0f)
        pct = 100.0f;
    return static_cast<uint8_t>(std::round((pct / 100.0f) * 255.0f));
}

} // namespace led_map
