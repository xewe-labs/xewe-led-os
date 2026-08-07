// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Hardware/LedStrip/Brightness/Brightness.cpp

#include "Brightness.h"


Brightness::Brightness(const uint16_t transition_delay,
                       const uint8_t initial_brightness,
                       const bool state_param
)
    : state(state_param)
    , last_brightness(initial_brightness)
{
    if (state) {
        timer = std::make_unique<AsyncTimer<uint8_t>>(transition_delay, last_brightness, initial_brightness);
    } else {
        timer = std::make_unique<AsyncTimer<uint8_t>>(transition_delay, 0, 0);
    }

    timer->initiate();
}

uint8_t Brightness::get_start_value() const {
    return timer->get_start_value();
    ;
}

uint8_t Brightness::get_current_value() const {
    return timer->get_current_value();
}

uint8_t Brightness::get_target_value() const {
    return timer->get_target_value();
}

void Brightness::set_brightness(const uint8_t new_brightness) {
    if (state) {
        uint8_t current = timer->get_current_value();
        timer->reset(current, new_brightness);
        timer->initiate();
    }

    if (new_brightness) {
        last_brightness = new_brightness;
    }
}

void Brightness::turn_on() {
    if (state) return;
    state = true;
    set_brightness(last_brightness);
}

void Brightness::turn_off() {
    if (!state) return;
    set_brightness(0);
    state = false;
}

uint8_t Brightness::get_dimmed_color(const uint8_t color) const {
    if (!state && timer->is_done()) return 0;

    uint8_t result = static_cast<uint8_t>((static_cast<uint32_t>(color) * timer->get_current_value()) / 255); // Use uint32_t for intermediate multiplication

    return result;
}

std::array<uint8_t, 3> Brightness::get_dimmed_color(const std::array<uint8_t, 3> color_rgb) const {
    if (!state && timer->is_done()) return {0, 0, 0};

    uint8_t current_timer_val = timer->get_current_value();

    return {
        static_cast<uint8_t>((static_cast<uint32_t>(color_rgb[0]) * current_timer_val) / 255),
        static_cast<uint8_t>((static_cast<uint32_t>(color_rgb[1]) * current_timer_val) / 255),
        static_cast<uint8_t>((static_cast<uint32_t>(color_rgb[2]) * current_timer_val) / 255)
    };
}

bool Brightness::get_state() const {
    return state;
}

uint8_t Brightness::get_last_brightness() const {
    return last_brightness;
}