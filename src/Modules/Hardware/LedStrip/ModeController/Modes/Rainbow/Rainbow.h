// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Hardware/LedStrip/ModeController/Modes/Rainbow/Rainbow.h
#pragma once

#include "../Mode/Mode.h"
#include "../../ModeRegistry/ModeRegistry.h"


class Rainbow : public Mode {
public:
    explicit               Rainbow     (const std::map<std::string, uint16_t>& params);

    void                   loop        (CRGB*    leds,
                                        uint16_t num_leds) override;
    std::array<uint8_t, 3> get_rgb     ()                  override;

private:
    uint16_t               current_hue;
};