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
      buffer_old_static_flag(false)
{
    DBG_PRINTLN(ModeController, "-> ModeController::ModeController()");
    DBG_PRINTF(ModeController, "Init config - Num LEDs: %u, Transition Delay: %u ms\n", num_leds, transition_delay_ms);

    fill_solid(buffer_current.data(), LED_STRIP_NUM_LEDS_MAX, CRGB::Black);
    fill_solid(buffer_old.data(), LED_STRIP_NUM_LEDS_MAX, CRGB::Black);

    transition_timer = std::make_unique<AsyncTimer<uint8_t>>(transition_delay_ms, 0, 255);
    set_mode(0, {});

    DBG_PRINTLN(ModeController, "<- ModeController::ModeController()");
}

void ModeController::loop() {
    if (transition_timer->is_active()) {
        update_interpolate_buffers(output_buffer);
        if (transition_timer->is_done()) {
            DBG_PRINTLN(ModeController, "[ModeController] Transition finished. Terminating timer.");
            transition_timer->terminate();
            buffer_old_static_flag = false;
        }
        return;
    }
    current_mode->loop(output_buffer, num_leds);
}

void ModeController::set_mode(const uint8_t mode_id, const std::map<std::string, uint16_t>& params) {
    DBG_PRINTF(ModeController, "-> ModeController::set_mode(mode_id: %u)\n", mode_id);

    auto& registry = ModeRegistry::get_registry();
    ModeFactory factory = registry.count(mode_id) ? registry.at(mode_id) : registry.at(0);

    if (transition_timer->is_active()) {
        update_interpolate_buffers(buffer_old.data());
        buffer_old_static_flag = true;
    }

    old_mode = std::move(current_mode);
    current_mode = factory(params);

    transition_timer->reset();
    transition_timer->initiate();

    DBG_PRINTLN(ModeController, "<- ModeController::set_mode()");
}

bool ModeController::set_mode_param(std::string_view key, uint16_t value) {
    DBG_PRINTF(ModeController, "-> ModeController::set_mode_param(key: %.*s, value: %u)\n", (int)key.length(), key.data(), value);

    auto params_map = get_params_as_map();

    if (params_map.count(std::string(key))) {
        params_map[std::string(key)] = value;
        set_mode(get_current_mode_id(), params_map);
        DBG_PRINTLN(ModeController, "<- ModeController::set_mode_param() [Success]");
        return true;
    }

    DBG_PRINTLN(ModeController, "<- ModeController::set_mode_param() [Failed: Key not found]");
    return false;
}

void ModeController::set_rgb(const std::array<uint8_t, 3> new_rgb) {
    DBG_PRINTF(ModeController, "-> ModeController::set_rgb(%u, %u, %u)\n", new_rgb[0], new_rgb[1], new_rgb[2]);

    auto params_map = get_params_as_map();
    std::array<uint8_t, 3> new_hsv = rgb_to_hsv(new_rgb);

    if (params_map.count("r"))   params_map["r"] = new_rgb[0];
    if (params_map.count("g"))   params_map["g"] = new_rgb[1];
    if (params_map.count("b"))   params_map["b"] = new_rgb[2];

    if (params_map.count("hue")) params_map["hue"] = new_hsv[0];
    if (params_map.count("sat")) params_map["sat"] = new_hsv[1];

    set_mode(get_current_mode_id(), params_map);

    DBG_PRINTLN(ModeController, "<- ModeController::set_rgb()");
}

uint16_t ModeController::get_current_mode_param(std::string_view key) const {
    for (const auto& param : current_mode->get_params()) {
        if (param.key == key) {
            return param.default_value;
        }
    }
    return 0;
}

void ModeController::update_interpolate_buffers(CRGB* output_buffer_ref) {
    if (!buffer_old_static_flag && old_mode) {
        old_mode->loop(buffer_old.data(), num_leds);
    }
    current_mode->loop(buffer_current.data(), num_leds);
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