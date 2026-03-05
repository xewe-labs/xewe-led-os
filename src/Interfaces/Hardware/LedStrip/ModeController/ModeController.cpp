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
    transition_timer = std::make_unique<AsyncTimer<uint8_t>>(transition_delay_ms, 0, 255);
    set_mode(0, {});
}

void ModeController::loop() {
    if (transition_timer && transition_timer->is_not_done()) {
        old_mode->loop(buffer_old.data(), num_leds);
        current_mode->loop(buffer_current.data(), num_leds);
        uint8_t progress = transition_timer->get_current_value();

        for (uint16_t i = 0; i < num_leds; i++) {
            output_buffer[i] = blend(buffer_old[i], buffer_current[i], progress);
        }
        return;
    }
    current_mode->loop(output_buffer, num_leds);
}

void ModeController::set_mode(const uint8_t mode_id, const std::map<std::string, uint16_t>& params) {
    auto& registry = ModeRegistry::get_registry();
    ModeFactory factory = nullptr;

    // Find the requested mode, fallback to mode 0 if missing
    if (registry.count(mode_id)) {
        factory = registry[mode_id];
    } else if (registry.count(0)) {
        factory = registry[0];
    }

    if (factory == nullptr) {
        printf("Error: No modes registered in ModeRegistry!\n");
        return;
    }

    old_mode = std::move(current_mode);
    current_mode = factory(params);

    transition_timer->reset();
    transition_timer->initiate();
}

bool ModeController::set_mode_param(std::string_view key, uint16_t value) {
    auto current_params = current_mode->get_params();
    std::string key_str(key);

    if (current_params.count(key_str)) {
        current_params[key_str] = value;
        set_mode(current_mode->get_id(), current_params);
        return true;
    }
    return false;
}

void ModeController::set_rgb(const std::array<uint8_t, 3> new_rgb) {
    auto current_params = current_mode->get_params();

    if (current_params.count("hue") && current_params.count("sat") && current_params.count("val")) {
        std::array<uint8_t, 3> hsv = rgb_to_hsv(new_rgb);

        current_params["hue"] = hsv[0];
        current_params["sat"] = hsv[1];
        current_params["val"] = hsv[2];

        set_mode(current_mode->get_id(), current_params);
    } else {
        printf("Mode has no color to set\n");
    }
}

ModeConfig ModeController::get_current_mode_config() const {
    return current_mode->get_config();
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

uint8_t ModeController::get_current_mode_id() const {
    return current_mode ? current_mode->get_id() : 0;
}

std::string ModeController::get_current_mode_name() const {
    return current_mode ? std::string(current_mode->get_config().mode_name) : "None";
}

uint16_t ModeController::get_current_mode_param(std::string_view key) const {
    if (!current_mode) return 0;
    auto params = current_mode->get_params();
    std::string key_str(key);
    return params.count(key_str) ? params.at(key_str) : 0;
}