// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Hardware/LedStrip/ModeController/Modes/FadeBrightness/FadeBrightness.cpp

#include "FadeBrightness.h"


static ModeRegistrar<FadeBrightness> registrar_fade_brightness(3);

FadeBrightness::FadeBrightness(const std::map<std::string, uint16_t>& params)
    : Mode(ModeConfig(3, "Brightness Fade", {
        {"hue", "Hue", 0, 255, 0, 1, 'b'},
        {"sat", "Saturation", 0, 255, 255, 1, 'b'},
        {"speed", "Speed", 1, 50, 5, 1, 'a'},
        {"noise_step", "Density", 1, 255, 10, 1, 'a'},
        {"min_bright", "Min Brightness", 0, 255, 10, 1, 'a'},
    }), params )
    , hue(get_param("hue"))
    , sat(get_param("sat"))
    , speed(get_param("speed"))
    , noise_step(get_param("noise_step"))
    , min_bright(get_param("min_bright"))
    , counter(0)
{
    std::array<uint8_t, 3> precise_rgb = hsv_to_rgb({hue, sat, 255});

    base_rgb                           = CRGB(precise_rgb[0], precise_rgb[1], precise_rgb[2]);
}

void FadeBrightness::loop(CRGB* leds,
                          uint16_t num_leds) {
    for (int i = 0; i < num_leds; i++) {
        uint8_t noise = inoise8(i * noise_step, counter);
        leds[i]       = get_brightness_color(noise);
    }

    counter += speed;
}

std::array<uint8_t, 3> FadeBrightness::get_rgb() {
    return {base_rgb.r, base_rgb.g, base_rgb.b};
}
CRGB FadeBrightness::get_brightness_color(uint8_t val) const {
    uint8_t calculated_val = constrain(map(val, 0, 255, min_bright, 255), 0, 255);
    return CHSV(hue, sat, calculated_val);
}
