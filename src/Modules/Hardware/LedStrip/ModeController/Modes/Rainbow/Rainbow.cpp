// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Hardware/LedStrip/ModeController/Modes/Rainbow/Rainbow.cpp

#include "Rainbow.h"


static ModeRegistrar<Rainbow> registrar_rainbow(5);

Rainbow::Rainbow(const std::map<std::string, uint16_t>& params)
    : Mode(ModeConfig(5, "Rainbow", {
        {"speed", "Speed", 1, 20, 5, 1, 'b'},
        {"density", "Density", 1, 30, 10, 1, 'a'},
    }), params)
    , current_hue(0)
{}

void Rainbow::loop(CRGB* leds,
                   uint16_t num_leds) {
    fill_rainbow(leds, num_leds, current_hue, static_cast<uint8_t>(get_param("density")));
    current_hue += get_param("speed");
}

std::array<uint8_t, 3> Rainbow::get_rgb() {
    return {255, 255, 255}; // always return white since has no true color
}