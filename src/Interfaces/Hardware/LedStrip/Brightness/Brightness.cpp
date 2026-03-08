/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Interfaces/Hardware/LedStrip/Brightness/Brightness.cpp

#include "Brightness.h"

Brightness::Brightness(uint16_t transition_delay, uint8_t initial_brightness, uint8_t state_param)
    : state(state_param), last_brightness(initial_brightness)
{
    DBG_PRINTF(Brightness, "-> Brightness::Brightness(delay=%u, initial_brightness=%u, state=%u)\n",
               transition_delay, initial_brightness, this->state); // Use this->state

    if (this->state) { // Use this->state
        timer = std::make_unique<AsyncTimer<uint8_t>>(transition_delay, last_brightness, initial_brightness);
        DBG_PRINTF(Brightness, "  Created timer for ON state: start=%u, target=%u\n",
                   last_brightness, initial_brightness);
    } else {
        timer = std::make_unique<AsyncTimer<uint8_t>>(transition_delay, 0, 0);
        DBG_PRINTLN(Brightness, "  Created timer for OFF state: start=0, target=0");
    }

    timer->initiate();

    DBG_PRINTF(Brightness, "  Timer initiated: get_start_value()=%u, get_target_value()=%u\n",
               timer->get_start_value(), timer->get_target_value());
    DBG_PRINTLN(Brightness, "<- Brightness::Brightness()");
}

uint8_t Brightness::get_start_value() const {
    DBG_PRINTLN(Brightness, "-> Brightness::get_start_value()");
    uint8_t v = timer->get_start_value();
    DBG_PRINTF(Brightness, "<- Brightness::get_start_value() returns: %u\n", v);
    return v;
}

uint8_t Brightness::get_current_value() const {
    DBG_PRINTLN(Brightness, "-> Brightness::get_current_value()");
    uint8_t v = timer->get_current_value();
    DBG_PRINTF(Brightness, "<- Brightness::get_current_value() returns: %u\n", v);
    return v;
}

uint8_t Brightness::get_target_value() const {
    DBG_PRINTLN(Brightness, "-> Brightness::get_target_value()");
    uint8_t v = timer->get_target_value();
    DBG_PRINTF(Brightness, "<- Brightness::get_target_value() returns: %u\n", v);
    return v;
}

void Brightness::set_brightness(uint8_t new_brightness) {
    DBG_PRINTF(Brightness, "-> Brightness::set_brightness(new_brightness: %u)\n", new_brightness);
    DBG_PRINTF(Brightness, "   (state=%s)\n", state ? "ON" : "OFF");

    if (state) {
        uint8_t current = timer->get_current_value(); // Reading timer state
        DBG_PRINTF(Brightness, "  Resetting timer: current_value=%u, new_target=%u\n",
                   current, new_brightness);
        timer->reset(current, new_brightness); // Modifying timer state
        timer->initiate();                     // Modifying timer state
        DBG_PRINTLN(Brightness, "  Timer re-initiated after set_brightness");
    }

    if (new_brightness) { // Update last_brightness if setting a non-zero value, or if turning on
        last_brightness = new_brightness;
        DBG_PRINTF(Brightness, "  last_brightness updated -> %u\n", last_brightness);
    }

    DBG_PRINTLN(Brightness, "<- Brightness::set_brightness()");
}

void Brightness::turn_on() {
    DBG_PRINTLN(Brightness, "-> Brightness::turn_on()");

    if (state) {
        DBG_PRINTLN(Brightness, "  Already on");
        DBG_PRINTLN(Brightness, "<- Brightness::turn_on() (already on)");
        return;
    }

    DBG_PRINTLN(Brightness, "  Was off, turning on");
    state = true;
    uint8_t brightness_to_set = last_brightness;

    set_brightness(brightness_to_set);

    DBG_PRINTLN(Brightness, "  State set to ON after set_brightness call");
    DBG_PRINTLN(Brightness, "<- Brightness::turn_on()");
}

void Brightness::turn_off() {
    DBG_PRINTLN(Brightness, "-> Brightness::turn_off()");

    if (!state) {
        DBG_PRINTLN(Brightness, "  Already off");
        DBG_PRINTLN(Brightness, "<- Brightness::turn_off() (already off)");
        return;
    }

    DBG_PRINTLN(Brightness, "  Turning off (set brightness to 0)");

    set_brightness(0);

    state = false;
    DBG_PRINTLN(Brightness, "  State set to OFF after set_brightness call");
    DBG_PRINTLN(Brightness, "<- Brightness::turn_off()");
}

uint8_t Brightness::get_dimmed_color(uint8_t color) const {
//    DBG_PRINTF(Brightness, "-> Brightness::get_dimmed_color(color: %u)\n", color);

    bool local_state = this->state;
    bool timer_is_done = timer->is_done();
    uint8_t current_timer_val = timer->get_current_value();

    if (!local_state && timer_is_done) {
//        DBG_PRINTF(Brightness, "<- Brightness::get_dimmed_color() returns: 0 (off and done)\n");
        return 0;
    }

    uint8_t result = static_cast<uint8_t>((static_cast<uint32_t>(color) * current_timer_val) / 255); // Use uint32_t for intermediate multiplication

//    DBG_PRINTF(Brightness, "<- Brightness::get_dimmed_color() returns: %u\n", result);
    return result;
}

std::array<uint8_t,3> Brightness::get_dimmed_color (std::array<uint8_t,3> color_rgb) const {
//    DBG_PRINTF(Brightness, "-> Brightness::get_dimmed_color(color: %u %u %u)\n", color_rgb[0], color_rgb[1], color_rgb[2]);

    bool local_state = this->state;
    bool timer_is_done = timer->is_done();
    uint8_t current_timer_val = timer->get_current_value();

    if (!local_state && timer_is_done) {
//        DBG_PRINTF(Brightness, "<- Brightness::get_dimmed_color() returns: 0 (off and done)\n");
        return {0, 0, 0};
    }

    std::array<uint8_t,3> result = {
        static_cast<uint8_t>((static_cast<uint32_t>(color_rgb[0]) * current_timer_val) / 255),
        static_cast<uint8_t>((static_cast<uint32_t>(color_rgb[1]) * current_timer_val) / 255),
        static_cast<uint8_t>((static_cast<uint32_t>(color_rgb[2]) * current_timer_val) / 255)
    };

//    DBG_PRINTF(Brightness, "<- Brightness::get_dimmed_color() returns: %u %u %u\n", result[0], result[1], result[2]);
    return result;
}

bool Brightness::get_state() const {
    DBG_PRINTLN(Brightness, "-> Brightness::get_state()");
    bool s = state;
    DBG_PRINTF(Brightness, "<- Brightness::get_state() returns: %s\n", s ? "true" : "false");
    return s;
}

uint8_t Brightness::get_last_brightness() const {
    DBG_PRINTLN(Brightness, "-> Brightness::get_last_brightness()");
    uint8_t lb = last_brightness;
    DBG_PRINTF(Brightness, "<- Brightness::get_last_brightness() returns: %u\n", lb);
    return lb;
}