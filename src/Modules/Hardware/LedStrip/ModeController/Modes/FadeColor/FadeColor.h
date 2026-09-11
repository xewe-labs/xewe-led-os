// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Hardware/LedStrip/ModeController/Modes/FadeColor/FadeColor.h
#pragma once

#include "../Mode/Mode.h"
#include "../../ModeRegistry/ModeRegistry.h"


class FadeColor : public Mode {
public:
    explicit               FadeColor      (const std::map<std::string, uint16_t>& params);

    void                   loop           (CRGB*    leds,
                                           uint16_t num_leds) override;
    std::array<uint8_t, 3> get_rgb        ()                  override;

private:
    CRGB                   get_fire_color (uint8_t noise_val) const;
    CRGB                   ColorHSV       (long    hue,
                                           uint8_t sat,
                                           uint8_t val)       const;

    // params cached at construction, loop() runs every frame
    const long             base_hue_16bit;
    const uint8_t          min_sat;
    const uint16_t         speed;
    const uint16_t         fire_step;
    const long             hue_gap;
    const uint8_t          min_bright;

    uint32_t               counter;
    CRGB                   base_rgb;
};