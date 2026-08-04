// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Hardware/LedStrip/ModeController/Modes/Pulse/Pulse.h
#pragma once

#include "../Mode/Mode.h"
#include "../../ModeRegistry/ModeRegistry.h"


class Pulse : public Mode {
public:
    explicit               Pulse   (const std::map<std::string, uint16_t>& params);

    void                   loop    (CRGB* leds,
                                    uint16_t num_leds) override;
    std::array<uint8_t, 3> get_rgb ()                  override;

private:
    CRGB                   rgb;
};