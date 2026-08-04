// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Hardware/LedStrip/ModeController/Modes/ChristmasLights/ChristmasLights.cpp

#include "ChristmasLights.h"


static ModeRegistrar<ChristmasLights> registrar_christmas_lights(6);

constexpr CRGB                        ChristmasLights::palette[5];

ChristmasLights::ChristmasLights(const std::map<std::string, uint16_t>& params)
    : Mode(ModeConfig(6, "Christmas Lights", {
        {"density", "Density", 1, 10, 1, 1, 'b'},
        {"speed", "Flicker", 0, 20, 5, 1, 'a'},
    }), params)
{
    for (uint16_t& offset : noise_offsets) {
        offset = random16();
    }
}

void ChristmasLights::loop(CRGB* leds,
                           uint16_t num_leds) {
    const uint16_t density = get_param("density");

    for (uint16_t i = 0; i < num_leds; i++) {
        leds[i] = palette[(i / density) % 5];
        leds[i].nscale8_video(inoise8(noise_offsets[i], z));
    }

    z += get_param("speed");
}

std::array<uint8_t, 3> ChristmasLights::get_rgb() {
    return {85, 49, 22};
}