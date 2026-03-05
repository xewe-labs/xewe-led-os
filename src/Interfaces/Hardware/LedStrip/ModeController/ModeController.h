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

#include "ModeRegistry/ModeRegistry.h"
#include "Modes/Mode/Mode.h"

class ModeController : public Module {
public:
    ModeController(CRGB* output_buffer, uint16_t num_leds, uint16_t transition_delay_ms);

    void loop();

    void set_mode(const uint8_t mode);
    void set_mode(const uint8_t mode, const std::map<std::string, uint16_t>& params);

    bool set_mode_param(std::string_view key, uint16_t value);
    void set_rgb(const std::array<uint8_t, 3> new_rgb);

    ModeConfig get_current_mode_config() const;
    uint16_t get_mode_transition_delay() const { return transition_timer->get_delay_ms(); }

    static std::array<uint8_t, 3> hsv_to_rgb(const std::array<uint8_t, 3> hsv);
    static std::array<uint8_t, 3> rgb_to_hsv(const std::array<uint8_t, 3> rgb);

private:
    uint16_t num_leds;
    std::unique_ptr<AsyncTimer<uint8_t>> transition_timer;

    std::unique_ptr<Mode> current_mode;
    std::unique_ptr<Mode> old_mode;

    CRGB* output_buffer;
    std::vector<CRGB> buffer_current;
    std::vector<CRGB> buffer_old;
};