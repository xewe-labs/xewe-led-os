#include "Brightness.h"

Brightness::Brightness(uint16_t transition_delay, uint8_t initial_brightness, uint8_t state)
    : state(state), last_brightness(initial_brightness)
{
    DBG_PRINTF(Brightness, "Brightness::Brightness() - delay=%u, initial_brightness=%u, state=%u\n",
               transition_delay, initial_brightness, state);

    if (state) {
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
}

uint8_t Brightness::get_start_value() const {
    uint8_t v = timer->get_start_value();
    DBG_PRINTF(Brightness, "get_start_value() -> %u\n", v);
    return v;
}

uint8_t Brightness::get_current_value() const {
    uint8_t v = timer->get_current_value();
//    DBG_PRINTF(Brightness, "get_current_value() -> %u (done=%s)\n",
//               v, timer->is_done() ? "true" : "false");
    return v;
}

uint8_t Brightness::get_target_value() const {
    uint8_t v = timer->get_target_value();
    DBG_PRINTF(Brightness, "get_target_value() -> %u\n", v);
    return v;
}

void Brightness::set_brightness(uint8_t new_brightness) {
    DBG_PRINTF(Brightness, "set_brightness(%u) called (state=%s)\n",
               new_brightness, state ? "ON" : "OFF");

    if (state) {
        uint8_t current = timer->get_current_value();
        DBG_PRINTF(Brightness, "  Resetting timer: current_value=%u, new_target=%u\n",
                   current, new_brightness);
        timer->reset(current, new_brightness);
        timer->initiate();
        DBG_PRINTLN(Brightness, "  Timer re-initiated after set_brightness");
    }

    if (new_brightness) {
        last_brightness = new_brightness;
        DBG_PRINTF(Brightness, "  last_brightness updated -> %u\n", last_brightness);
    }
}

void Brightness::turn_on() {
    DBG_PRINTLN(Brightness, "turn_on() called");
    if (state) {
        DBG_PRINTLN(Brightness, "  Already on");
        Serial.println("Already on");
        return;
    }
    DBG_PRINTLN(Brightness, "  Was off, turning on");
    state = true;
    set_brightness(last_brightness);
    DBG_PRINTLN(Brightness, "  State set to ON");
}

void Brightness::turn_off() {
    DBG_PRINTLN(Brightness, "turn_off() called");
    if (!state) {
        DBG_PRINTLN(Brightness, "  Already off");
        Serial.println("Already off");
        return;
    }
    DBG_PRINTLN(Brightness, "  Turning off (set brightness to 0)");
    set_brightness(0);
    state = false;
    DBG_PRINTLN(Brightness, "  State set to OFF");
}

uint8_t Brightness::get_dimmed_color(uint8_t color) const {
    if (!state && timer->is_done()) {
//        DBG_PRINTF(Brightness, "get_dimmed_color(%u) -> OFF state, returning 0\n", color);
        return 0;
    }
    uint16_t current = get_current_value();
    uint8_t result  = static_cast<uint8_t>((static_cast<uint16_t>(color) * current) / 255);
//    DBG_PRINTF(Brightness, "get_dimmed_color(%u): current_value=%u -> result=%u\n",
//               color, current, result);
    return result;
}

bool Brightness::get_state() const {
    DBG_PRINTF(Brightness, "get_state() -> %s\n", state ? "true" : "false");
    return state;
}

uint8_t Brightness::get_last_brightness() const {
    DBG_PRINTF(Brightness, "get_last_brightness() -> %u\n", last_brightness);
    return last_brightness;
}
