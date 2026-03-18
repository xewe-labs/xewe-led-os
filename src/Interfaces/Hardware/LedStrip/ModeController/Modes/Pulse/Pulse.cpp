/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Interfaces/Hardware/LedStrip/ModeController/Modes/Pulse/Pulse.h

#include "Pulse.h"

static ModeRegistrar<Pulse> registrar_pulse(4);

Pulse::Pulse(const std::map<std::string, uint16_t>& params)
    : Mode(ModeConfig(4, "Pulse", {
        {"hue", "Hue", 0, 255, 0, 1, 'b'},
        {"sat", "Saturation", 0, 255, 255, 1, 'b'},
        {"speed", "Speed", 1, 255, 30, 1, 'a'}
      }), params)
{
    std::array<uint8_t, 3> precise_rgb = hsv_to_rgb({
        static_cast<uint8_t>(get_param("hue")),
        static_cast<uint8_t>(get_param("sat")),
        255
    });

    rgb = CRGB(precise_rgb[0], precise_rgb[1], precise_rgb[2]);
}

void Pulse::loop(CRGB* leds, uint16_t num_leds) {
    CRGB current_color = rgb;
    current_color.nscale8(beatsin8(get_param("speed")));
    fill_solid(leds, num_leds, current_color);
}

std::array<uint8_t, 3> Pulse::get_rgb() {

    return {rgb.r, rgb.g, rgb.b};
}