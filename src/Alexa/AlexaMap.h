// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/src/Alexa/AlexaMap.h
// Espalexa takes a 16-bit hue and a saturation below 255.
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
