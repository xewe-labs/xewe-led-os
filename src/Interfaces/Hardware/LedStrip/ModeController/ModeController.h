/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Interfaces/Hardware/LedStrip/ModeController/ModeController.h
#pragma once

#include "Modes/Mode/Mode.h"
#include "Modes/Solid/Solid.h"
#include "Modes/Rainbow/Rainbow.h"

struct ModeParam {
    std::string key;
    std::string display_name;
    uint16_t min_value;
    uint16_t max_value;
    uint16_t default_value;
    uint16_t step_value;
};

struct ModeConfig {
    uint8_t mode_id;
    std::string mode_name;
    std::vector<ModeParam> params;
};

class ModeController : public Module {
public:
    ModeController                                          (CRGB* output_buffer, uint16_t num_leds, uint16_t transition_delay_ms);

    void                        loop                        ();

    void                        set_mode                    (const uint8_t mode);
    void                        set_mode_param              (std::string_view key, uint16_t value);
    void                        set_rgb                     (const std::array<uint8_t, 3> new_rgb);

    ModeConfig                  get_current_mode_config     () const;
    uint16_t                    get_mode_transition_delay   () const {return transition_timer->delay_ms;}

    static std::array<uint8_t, 3> hsv_to_rgb                (const std::array<uint8_t, 3> hsv);
    static std::array<uint8_t, 3> rgb_to_hsv                (const std::array<uint8_t, 3> rgb);
private:
    uint16_t                    num_leds;
    unique_ptr                  <AsyncTimer<uint8_t>>       transition_timer;

    // Smart pointers to manage mode lifecycles elegantly
    std::unique_ptr<Mode> current_mode;
    std::unique_ptr<Mode> old_mode;

    CRGB* output_buffer;
    std::vector<CRGB> buffer_current;
    std::vector<CRGB> buffer_old;
};