/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Interfaces/Hardware/LedStrip/ModeController/Modes/FadeColor/FadeColor.h

#pragma once

#include "../Mode/Mode.h"
#include "../../ModeRegistry/ModeRegistry.h"

class FadeColor : public Mode {
public:
    explicit                    FadeColor                   (const std::map<std::string, uint16_t>& params);

    void                        loop                        (CRGB* leds,
                                                             uint16_t num_leds)             override;
    std::array<uint8_t, 3>      get_rgb                     ()                              override;

private:
    CRGB                        get_fire_color              (uint8_t noise_val,
                                                             long base_hue_16bit);
    CRGB                        ColorHSV                    (long hue,
                                                             uint8_t sat,
                                                             uint8_t val);

    uint32_t                    counter;
    CRGB                        base_rgb;
};