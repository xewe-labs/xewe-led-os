/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Interfaces/Hardware/LedStrip/ModeController/Modes/FadeColor/FadeColor.cpp

#include "FadeColor.h"

static ModeRegistrar<FadeColor> registrar_fade_color(1);

FadeColor::FadeColor(const std::map<std::string, uint16_t>& params)
    : Mode(ModeConfig(1, "FadeColor", {
        {"hue", "Hue", 0, 255, 0, 1},
        {"sat", "Saturation", 0, 255, 255, 1},
        {"speed", "Animation Speed", 1, 50, 5, 1},
        {"hue_gap", "Hue Variance", 0, 32768, 10000, 100},
        {"fire_step", "Noise Step", 1, 255, 10, 1},
        {"min_bright", "Min Brightness", 0, 255, 100, 1},
        {"max_bright", "Max Brightness", 0, 255, 255, 1},
        {"min_sat", "Min Saturation", 0, 255, 245, 1},
        {"max_sat", "Max Saturation", 0, 255, 255, 1}
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

void FadeColor::loop(CRGB* leds, uint16_t num_leds) {
    long base_hue_16bit = static_cast<long>(get_param("hue")) * 256;
    uint16_t fire_step = get_param("fire_step");

    for (int i = 0; i < num_leds; i++) {
        uint8_t noise = inoise8(i * fire_step, counter);
        leds[i] = get_fire_color(noise, base_hue_16bit);
    }

    counter += get_param("speed");
}

CRGB FadeColor::get_fire_color(uint8_t val, long base_hue_16bit) {
    long hue_gap = get_param("hue_gap");
    long half_hue_gap = hue_gap / 2;
    long calculated_hue = base_hue_16bit - half_hue_gap + map(val, 0, 255, 0, hue_gap);

    uint8_t min_sat = get_param("min_sat");
    uint8_t max_sat = get_param("max_sat");
    uint8_t min_bright = get_param("min_bright");
    uint8_t max_bright = get_param("max_bright");

    uint8_t calculated_sat = constrain(map(val, 0, 255, max_sat, min_sat), 0, 255);
    uint8_t calculated_val = constrain(map(val, 0, 255, min_bright, max_bright), 0, 255);

    return ColorHSV(calculated_hue, calculated_sat, calculated_val);
}

CRGB FadeColor::ColorHSV(long hue, uint8_t sat, uint8_t val) {

    return CHSV(static_cast<uint16_t>(hue) >> 8, sat, val);
}

std::array<uint8_t, 3> FadeColor::get_rgb() {

    return {base_rgb.r, base_rgb.g, base_rgb.b};
}