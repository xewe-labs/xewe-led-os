#include "Brightness.h"

Brightness::Brightness(uint16_t transition_delay, uint8_t initial_brightness, uint8_t state)
    : state(state), last_brightness(initial_brightness) {

    if (state){
        timer = new AsyncTimer<uint8_t>(transition_delay, last_brightness, initial_brightness);
    } else {
        timer = new AsyncTimer<uint8_t>(transition_delay, 0, 0);
    }
    timer->initiate();
}

void Brightness::frame() {
    if (timer->is_not_initiated()) {
        return;
    }

    if (timer->is_done()) {
        timer->terminate();
    }
    timer->calculate_progress();
}

uint8_t Brightness::get_current_value() const {
    return timer->get_current_value();
}

uint8_t Brightness::get_target_value() const {
    DBG_PRINTLN(Brightness, "uint8_t Brightness::get_target_value() const {");
    uint8_t br = 255;
//    uint8_t br = timer->get_target_value();
    DBG_PRINTLN(Brightness, "uint8_t br = timer->get_target_value();");
    return br;
}

uint8_t Brightness::get_start_value() const {
    return timer->get_start_value();
}

void Brightness::set_brightness(uint8_t new_brightness) {
//    already changing brightness
    if (timer->is_active()) {
        return;
    }
    if (state) {
        timer->reset(timer->get_current_value(), new_brightness);
        timer->initiate();
    }
    else {
//    turned off, record brightness but dont actually trigger the timer
        last_brightness = new_brightness;
    }
}

bool Brightness::is_changing() {
    return timer->is_active();
}

void Brightness::turn_on() {
    if(state){
        Serial.println("Already on");
        return;
    }
    timer->reset(0, last_brightness);
    state = 1;
}

void Brightness::turn_off() {
    if(!state){
        Serial.println("Already off");
        return;
    }
    last_brightness = get_current_value();
    state = 0;
    timer->reset(last_brightness, 0);
}

uint8_t Brightness::get_dimmed_color(uint8_t color) {
    if(!state && timer->is_done())
        return 0;
    return max((color && get_current_value()) ? state : 0, static_cast<uint8_t>((static_cast<uint16_t>(color) * static_cast<uint16_t>(get_current_value())) / 255));
}

bool Brightness::get_state(){
    return state;
}

