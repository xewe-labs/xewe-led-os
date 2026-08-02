/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Interfaces/Hardware/LedStrip/Brightness/Brightness.h

#pragma once

#include <memory>
#include <array>

#include "../../../../Debug.h"
#include "../../../../Utils/XeWeTimer.h"

using namespace std;

class Brightness {

public:
    Brightness                                                      (const uint16_t transition_delay,
                                                                     const uint8_t initial_brightness,
                                                                     const uint8_t state);

    uint8_t                             get_start_value             () const;
    uint8_t                             get_current_value           () const;
    uint8_t                             get_target_value            () const;
    void                                set_brightness              (const uint8_t new_brightness);
    void                                turn_on                     ();
    void                                turn_off                    ();
    uint8_t                             get_dimmed_color            (const uint8_t color) const;
    array<uint8_t,3>                    get_dimmed_color            (const array<uint8_t,3> color_rgb) const;
    bool                                get_state                   () const;
    uint8_t                             get_last_brightness         () const;

private:
    unique_ptr<AsyncTimer<uint8_t>>     timer;
    uint8_t                             state;
    uint8_t                             last_brightness;
};