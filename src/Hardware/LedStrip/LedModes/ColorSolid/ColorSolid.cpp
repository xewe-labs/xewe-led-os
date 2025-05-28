// File: ColorSolid.cpp
#include "ColorSolid.h"

ColorSolid::ColorSolid(LedStrip* led_strip, uint8_t r, uint8_t g, uint8_t b)
    : LedMode(led_strip)
{
    set_rgb(r, g, b);
}

void ColorSolid::frame() { led_strip->fill_all(LedMode::get_r(), LedMode::get_g(), LedMode::get_b()); }

bool ColorSolid::is_done() { return true; }

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
