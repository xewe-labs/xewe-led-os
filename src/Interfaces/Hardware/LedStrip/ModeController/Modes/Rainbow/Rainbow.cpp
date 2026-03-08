/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Interfaces/Hardware/LedStrip/ModeController/Modes/Rainbow/Rainbow.cpp

#include "Rainbow.h"

static ModeRegistrar<Rainbow> registrar_rainbow(4);

Rainbow::Rainbow(const std::map<std::string, uint16_t>& params)
    : Mode(ModeConfig(4, "Rainbow", {
        {"speed", "Animation Speed", 1, 20, 5, 1},
        {"density", "Color Density", 1, 30, 10, 1}
      }), params),
      current_hue(0)
{}

void Rainbow::loop(CRGB* leds, uint16_t num_leds) {
    fill_rainbow(leds, num_leds, current_hue, static_cast<uint8_t>(get_param("density")));
    current_hue += get_param("speed");
}

std::array<uint8_t, 3> Rainbow::get_rgb() {
    return {255, 255, 255}; // always return white since has no true color
}