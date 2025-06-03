// File: ColorSolid.cpp
#include "ColorSolid.h"

ColorSolid::ColorSolid(LedStrip* led_strip, uint8_t r, uint8_t g, uint8_t b)
    : LedMode(led_strip), id_done_flag(false)
{
    set_rgb(r, g, b);
}

void ColorSolid::frame() {
    led_strip->fill_all(get_r(), get_g(), get_b());
    id_done_flag = true;
}

bool ColorSolid::is_done() { return id_done_flag; }

uint8_t ColorSolid::get_mode_id() {
    return 0;
}

String ColorSolid::get_mode_name() {
    return "Color Solid";
}

std::array<uint8_t, 3> ColorSolid::get_target_rgb() {
    return get_rgb();
}

uint8_t ColorSolid::get_target_r() {
    return get_r();
}

uint8_t ColorSolid::get_target_g() {
    return get_g();
}

uint8_t ColorSolid::get_target_b() {
    return get_b();
}

std::array<uint8_t, 3> ColorSolid::get_target_hsv() {
    return get_hsv();
}

uint8_t ColorSolid::get_target_h() {
    return get_h();
}

uint8_t ColorSolid::get_target_s() {
    return get_s();
}

uint8_t ColorSolid::get_target_v() {
    return get_v();
}
