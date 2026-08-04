// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Hardware/LedStrip/ModeController/Modes/FadeBrightness/FadeBrightness.h
#pragma once

#include "../Mode/Mode.h"
#include "../../ModeRegistry/ModeRegistry.h"


class FadeBrightness : public Mode {
public:
    explicit               FadeBrightness       (const std::map<std::string, uint16_t>& params);

    void                   loop                 (CRGB*    leds,
                                                 uint16_t num_leds) override;

    std::array<uint8_t, 3> get_rgb              ()                  override;

private:
    CRGB                   get_brightness_color (uint8_t noise_val,
                                                 uint8_t base_hue,
                                                 uint8_t base_sat,
                                                 uint8_t min_bright,
                                                 uint8_t max_bright);

    uint32_t               counter;
    CRGB                   base_rgb;
};