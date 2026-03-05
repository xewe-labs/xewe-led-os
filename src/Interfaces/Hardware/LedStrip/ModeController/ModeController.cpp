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
    ModeFactory factory = registry.count(mode_id) ? registry.at(mode_id) : registry.at(0);

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
    auto params_map = get_params_as_map();

    if (params_map.count(key)) {
        params_map[key] = value;
        set_mode(get_current_mode_id(), params_map);
        return true;
    }
    return false;
}

void ModeController::set_rgb(const std::array<uint8_t, 3> new_rgb) {
    auto params_map = get_params_as_map();

    params_map["r"] = new_rgb[0];
    params_map["g"] = new_rgb[1];
    params_map["b"] = new_rgb[2];

    set_mode(get_current_mode_id(), params_map);
}

uint8_t ModeController::get_current_mode_id() const {
    return current_mode->get_id();
}

std::string_view ModeController::get_current_mode_name() const {
    return current_mode->get_name();
}

uint16_t ModeController::get_current_mode_param(std::string_view key) const {
    for (const auto& param : current_mode->get_params()) {
        if (param.key == key) {
            return param.default_value; // Default acts as the current state placeholder
        }
    }
    return 0; // Return 0 if key not found (or handle via exceptions if preferred)
}

std::vector<ModeParam> ModeController::get_current_mode_params() const {
    return current_mode->get_params();
}

const ModeConfig& ModeController::get_current_mode_config() const {
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

std::map<std::string, uint16_t> ModeController::get_params_as_map() const {
    std::map<std::string, uint16_t> map;
    for (const auto& param : current_mode->get_params()) {
        map[param.key] = param.default_value;
    }
    return map;
}