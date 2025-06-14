// File: ColorChanging.cpp
#include "ColorChanging.h"

ColorChanging::ColorChanging(LedStrip* controller,
                             uint8_t current_r, uint8_t current_g, uint8_t current_b,
                             uint8_t t0, uint8_t t1, uint8_t t2,
                             char mode, uint32_t duration_ms)
  : LedMode(controller),
    timer(nullptr)
{
    if (mode == 'r') {
        set_rgb(t0, t1, t2);
    } else if (mode == 'h') {
        set_hsv(t0, t1, t2);
    }
    std::array<uint8_t, 3> start_tmp{current_r, current_g, current_b};
    std::array<uint8_t, 3> target_tmp{get_r(), get_g(), get_b()};
    timer = std::make_unique<AsyncTimerArray>(duration_ms, start_tmp, target_tmp);
    timer->initiate();
}

void ColorChanging::loop() {
    std::array<uint8_t,3>    current_color = timer->get_current_value();
    set_rgb                 (current_color[0], current_color[1], current_color[2]);
}

bool ColorChanging::is_done() {
    return timer->is_done();
}

uint8_t ColorChanging::get_mode_id() {
    return 1;
}

String ColorChanging::get_mode_name() {
    return "Color Changing";
}

std::array<uint8_t, 3> ColorChanging::get_target_rgb() {
    return timer->get_target_value();
}

uint8_t ColorChanging::get_target_r() {
    return get_target_rgb()[0];
}

uint8_t ColorChanging::get_target_g() {
    return get_target_rgb()[1];
}

uint8_t ColorChanging::get_target_b() {
    return get_target_rgb()[2];
}

std::array<uint8_t, 3> ColorChanging::get_target_hsv() {
    return rgb_to_hsv(timer->get_target_value());
}

uint8_t ColorChanging::get_target_h() {
    return get_target_hsv()[0];
}

uint8_t ColorChanging::get_target_s() {
    return get_target_hsv()[1];
}

uint8_t ColorChanging::get_target_v() {
    return get_target_hsv()[2];
}