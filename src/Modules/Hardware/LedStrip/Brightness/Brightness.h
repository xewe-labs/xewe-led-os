// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Hardware/LedStrip/Brightness/Brightness.h
#pragma once

#include <memory>
#include <array>

#include "../../../../Utils/Debug.h"
#include "../../../../Utils/XeWeTimer.h"


class Brightness {
public:
                                         Brightness          (const uint16_t transition_delay,
                                                              const uint8_t initial_brightness,
                                                              const uint8_t state);

    uint8_t                              get_start_value     ()                                       const;
    uint8_t                              get_current_value   ()                                       const;
    uint8_t                              get_target_value    ()                                       const;
    void                                 set_brightness      (const uint8_t new_brightness);
    void                                 turn_on             ();
    void                                 turn_off            ();
    uint8_t                              get_dimmed_color    (const uint8_t color)                    const;
    std::array<uint8_t, 3>               get_dimmed_color    (const std::array<uint8_t, 3> color_rgb) const;
    bool                                 get_state           ()                                       const;
    uint8_t                              get_last_brightness ()                                       const;

private:
    std::unique_ptr<AsyncTimer<uint8_t>> timer;
    uint8_t                              state;
    uint8_t                              last_brightness;
};