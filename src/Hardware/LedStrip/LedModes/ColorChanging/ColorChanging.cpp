// File: ColorChanging.cpp
#include "ColorChanging.h"

ColorChanging::ColorChanging(LedStrip* controller,
                             uint8_t current_r, uint8_t current_g, uint8_t current_b,
                             uint8_t t0, uint8_t t1, uint8_t t2,
                             char mode, uint32_t duration_ms)
  : LedMode(controller),
    timer(nullptr)
{
    // decide whether our target is RGB or HSV→RGB
    if (mode == 'r') {
        set_rgb(t0, t1, t2);
    } else {
        set_hsv(t0, t1, t2);
    }
    // now LedMode::rgb[] holds the *target* RGB

    // hand off the current & target arrays to AsyncTimerArray;
    // it will copy both into its own storage
    uint8_t start_tmp[3]  = { current_r, current_g, current_b };
    uint8_t target_tmp[3] = { get_r(), get_g(), get_b() };
    timer = new AsyncTimerArray(duration_ms,
                                start_tmp,
                                target_tmp);
    timer->initiate();
}

ColorChanging::~ColorChanging() {
    delete timer;
}

void ColorChanging::frame() {
    if (!timer || timer->is_not_initiated()) return;

    if (timer->is_done()) {
        timer->terminate();
    }

    auto current_color = timer->get_current_value();
    led_controller->fill_all(current_color[0], current_color[1], current_color[2]);
    set_rgb(current_color[0], current_color[1], current_color[2]);
}

bool ColorChanging::is_done() {
    return timer && timer->is_done();
}

uint8_t ColorChanging::get_mode_id() {
    return 2;
}

std::array<uint8_t, 3> ColorChanging::get_target_rgb() {
    return timer->get_target_value();
}

uint8_t ColorChanging::get_target_r() {
    return timer->get_target_value()[0];
}

uint8_t ColorChanging::get_target_g() {
    return timer->get_target_value()[1];
}

uint8_t ColorChanging::get_target_b() {
    return timer->get_target_value()[2];
}
