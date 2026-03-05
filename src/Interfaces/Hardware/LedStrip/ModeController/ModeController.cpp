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
      buffer_old(num_leds, CRGB::Black),
      buffer_old_static_flag(false)
{
    transition_timer = std::make_unique<AsyncTimer<uint8_t>>(transition_delay_ms, 0, 255);
    set_mode(0, {});
}

void ModeController::loop() {
    if (transition_timer->is_not_done()) {
        update_interpolate_buffers(output_buffer);
        return;
    } else if (transition_timer->is_active()) {
        transition_timer->terminate();
        buffer_old_static_flag = false;
    }
    current_mode->loop(output_buffer, num_leds);
}

void ModeController::set_mode(const uint8_t mode_id, const std::map<std::string, uint16_t>& params) {
    auto& registry = ModeRegistry::get_registry();
    ModeFactory factory = nullptr;

    if (registry.count(mode_id)) {
        factory = registry[mode_id];
    } else {
        factory = registry[0];
    }

    // mode change in progress, record snapshot of the current transition
    if (transition_timer->is_not_done()) {
        update_interpolate_buffers(buffer_old);
        buffer_old_static_flag = true;
    }

    old_mode = std::move(current_mode);
    current_mode = factory(params);

    transition_timer->reset();
    transition_timer->initiate();
}

bool ModeController::set_mode_param(std::string_view key, uint16_t value) {
    auto current_params = current_mode->get_params();

    if (current_params.count(key)) {
        current_params[key_str] = value;
        set_mode(current_mode->get_id(), current_params);
        return true;
    }
    return false;
}

void ModeController::set_rgb(const std::array<uint8_t, 3> new_rgb) {
    auto current_params = current_mode->get_params();
    auto new_params = current_params.replace({{"r", new_rgb[0], {"g", new_rgb[1], {"b", new_rgb[2]}};
    set_mode(get_current_mode_id(), new_params);
}

uint8_t ModeController::get_current_mode_id() const {
    return current_mode->get_config().id;
}

std::string ModeController::get_current_mode_name() const {
    return current_mode->get_config().mode_name;
}

uint16_t ModeController::get_current_mode_param(std::string_view key) const {
    return current_mode->get_config().params[key];
}

vector<ModeParam> ModeController::get_current_mode_params() const {
    return current_mode->get_config().params;
}

ModeConfig ModeController::get_current_mode_config() const {
    return current_mode->get_config();
}

void ModeController::update_interpolate_buffers(CRGB* output_buffer_ref) {
    if (!buffer_old_static_flag) {
        old_mode->loop(buffer_old, num_leds);
    }
    current_mode->loop(buffer_current, num_leds);
    uint8_t progress = transition_timer->get_current_value();

    for (uint16_t i = 0; i < num_leds; i++) {
        output_buffer_ref[i] = blend(buffer_old[i], buffer_current[i], progress);
    }
}