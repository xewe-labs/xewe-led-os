// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Hardware/LedStrip/ModeController/Modes/Solid/Solid.cpp

#include "Solid.h"


static ModeRegistrar<Solid> registrar_solid(0);

Solid::Solid(const std::map<std::string, uint16_t>& params)
    : Mode(ModeConfig(0, "Solid", {
        {"hue", "Hue", 0, 255, 0, 1, 'b'},
        {"sat", "Saturation", 0, 255, 255, 1, 'b'},
    }), params )
{
    std::array<uint8_t, 3> precise_rgb = hsv_to_rgb({
        static_cast<uint8_t>(get_param("hue")),
        static_cast<uint8_t>(get_param("sat")),
        255
    });

    rgb                                = CRGB(precise_rgb[0], precise_rgb[1], precise_rgb[2]);
}

void Solid::loop(CRGB* leds,
                 uint16_t num_leds) {
    fill_solid(leds, num_leds, rgb);
}

std::array<uint8_t, 3> Solid::get_rgb() {
    return {rgb.r, rgb.g, rgb.b};
}