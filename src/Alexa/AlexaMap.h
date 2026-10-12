// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/src/Alexa/AlexaMap.h
// Project-local module of xewe-led-os (was src/Modules/Software/SmartHome/Alexa in 2.3.x)
//
// Pure mapping led -> Espalexa colour (2.3.x Alexa::sync_color): Espalexa takes a 16-bit hue and an
// 8-bit saturation below 255. Standard library only; host-tested by tests/unit/test_alexa.py.
#pragma once

#include <cstdint>

namespace led_map {

inline uint16_t hue8_to_hue16(uint8_t hue8) {
    return static_cast<uint16_t>((hue8 << 8) | hue8);
}
inline uint8_t sat8_to_alexa(uint8_t sat8) {
    return sat8 == 255 ? 254 : sat8;
}

} // namespace led_map
