#include "Brightness.h"

Brightness::Brightness(uint16_t transition_delay, uint8_t initial_brightness, uint8_t state)
    : state(state), last_brightness(initial_brightness) {

    if (state) {
        timer = std::make_unique<AsyncTimer<uint8_t>>(transition_delay, last_brightness, initial_brightness);
    } else {
        timer = std::make_unique<AsyncTimer<uint8_t>>(transition_delay, 0, 0);
    }
    timer->initiate();
}

uint8_t Brightness::get_start_value() const {
    return timer->get_start_value();
}

uint8_t Brightness::get_current_value() {
    return timer->get_current_value();
}

uint8_t Brightness::get_target_value() const {
    return timer->get_target_value();
}

void Brightness::set_brightness(uint8_t new_brightness) {
    if (state) {
        timer->reset(timer->get_current_value(), new_brightness);
        timer->initiate();
    }
    //    update last nonzero brightness
    if (new_brightness) {
        last_brightness = new_brightness;
    }
}

void Brightness::turn_on() {
    if (state) {
        Serial.println("Already on");
        return;
    }
    state = true;
    set_brightness(last_brightness);
}

void Brightness::turn_off() {
    if (!state) {
        Serial.println("Already off");
        return;
    }
    set_brightness(0);
    state = false;
}

uint8_t Brightness::get_dimmed_color(uint8_t color) const {
    if (!state)
        return 0;
    return static_cast<uint8_t>((static_cast<uint16_t>(color) * get_current_value()) / 255);
}

bool Brightness::get_state() const {
    return state;
}

uint8_t Brightness::get_last_brightness() const {
    return last_brightness;
}
