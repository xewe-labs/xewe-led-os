/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Interfaces/Hardware/LedStrip/ModeController/Modes/FadeBrightness/FadeBrightness.cpp

#include "FadeBrightness.h"

static ModeRegistrar<FadeBrightness> registrar_fade_brightness(2);

FadeBrightness::FadeBrightness(const std::map<std::string, uint16_t>& params)
    : Mode(ModeConfig(2, "Brightness Fade", {
        {"hue", "Hue", 0, 255, 0, 1, 'b'},
        {"sat", "Saturation", 0, 255, 255, 1, 'b'},
        {"speed", "Speed", 1, 50, 5, 1, 'a'},
        {"noise_step", "Density", 1, 255, 10, 1, 'a'},
        {"min_bright", "Min Brightness", 0, 255, 10, 1, 'a'},
      }), params),
      counter(0)
{
    std::array<uint8_t, 3> precise_rgb = hsv_to_rgb({
        static_cast<uint8_t>(get_param("hue")),
        static_cast<uint8_t>(get_param("sat")),
        255
    });

    base_rgb = CRGB(precise_rgb[0], precise_rgb[1], precise_rgb[2]);
}

void FadeBrightness::loop(CRGB* leds, uint16_t num_leds) {
    uint8_t hue = get_param("hue");
    uint8_t sat = get_param("sat");
    uint16_t noise_step = get_param("noise_step");
    uint8_t min_bright = get_param("min_bright");

    for (int i = 0; i < num_leds; i++) {
        uint8_t noise = inoise8(i * noise_step, counter);
        leds[i] = get_brightness_color(noise, hue, sat, min_bright, 255);
    }

    counter += get_param("speed");
}

CRGB FadeBrightness::get_brightness_color(uint8_t val, uint8_t base_hue, uint8_t base_sat, uint8_t min_bright, uint8_t max_bright) {
    uint8_t calculated_val = constrain(map(val, 0, 255, min_bright, max_bright), 0, 255);
    return CHSV(base_hue, base_sat, calculated_val);
}

std::array<uint8_t, 3> FadeBrightness::get_rgb() {

    return {base_rgb.r, base_rgb.g, base_rgb.b};
}