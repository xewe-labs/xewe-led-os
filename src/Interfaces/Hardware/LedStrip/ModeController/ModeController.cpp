/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Interfaces/Hardware/LedStrip/ModeController/ModeController.cpp

#include "ModeController.h"

ModeController::ModeController(CRGB* output_buffer, uint16_t num_leds, uint16_t transition_delay_ms)
    : num_leds(num_leds),
      output_buffer(output_buffer),
      buffer_current(num_leds, CRGB::Black),
      buffer_old(num_leds, CRGB::Black)
{
    transition_timer = std::make_unique<AsyncTimer<uint8_t>>(transition_delay_ms);
    set_mode(0);
}

void ModeController::loop() {
    if (transition_timer->is_not_done()) {
        old_mode->loop(buffer_old, num_leds);
        current_mode->loop(buffer_current, num_leds);
        double progress = transition_timer->get_progress();

        for (uint16_t i = 0; i < num_leds; i++) {
            output_buffer[i] = blend(buffer_old[i], buffer_current[i], progress);
        }
        return;
    }

    current_mode->loop(output_buffer, num_leds);
    return;
}

void ModeController::set_mode(const uint8_t mode, params **) {
    // add check if the requested mode id exists
    mode_pointer = find_mode(mode)
    if (mode pointer == null)
        return;

    old_mode = std::move(current_mode);
    current_mode = std::make_unique<mode_pointer>(params **);
    transition_timer->reset();
    transition_timer->initiate();
}

void ModeController::set_mode_param(std::string_view key, uint16_t value) {
    current_params = current_mode->get_params();
    current_params_new = current_params.replace(key, value);
    set_mode(current_params_new);
}

void ModeController::set_rgb(const std::array<uint8_t, 3> new_rgb) {
    current_params = current_mode->get_params();
    if mode_has_color() // check for mode having "hue" "sat" "val" params
        hsv = rgb_to_hsv(new_rgb);
        current_params_new = current_params.replace("", value);
        set_mode(current_params_new);
    else
        print mode has no color to set
}

ModeConfig ModeController::get_current_mode_config() const {
    if (current_mode) {
        return current_mode->get_config();
    }
    return ModeConfig{0, "None", {}};
}


std::array<uint8_t, 3> ModeController::hsv_to_rgb(const std::array<uint8_t, 3> hsv) {
    CRGB rgb;
    hsv2rgb_rainbow(CHSV(hsv[0], hsv[1], hsv[2]), rgb);
    return {rgb.r, rgb.g, rgb.b};
}

std::array<uint8_t, 3> ModeController::rgb_to_hsv(const std::array<uint8_t, 3> rgb) {
    CHSV hsv = rgb2hsv_approximate(CRGB(rgb[0], rgb[1], rgb[2]));
    return {hsv.h, hsv.s, hsv.v};
}