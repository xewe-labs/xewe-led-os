// File: ColorSolid.cpp
#include "ColorSolid.h"

ColorSolid::ColorSolid(LedStrip* controller)
    : LedMode(controller) { }

ColorSolid::ColorSolid(LedStrip* controller, uint8_t r, uint8_t g, uint8_t b)
    : LedMode(controller)
{
    set_rgb(r, g, b);
}

void ColorSolid::frame() { led_controller->fill_all(LedMode::get_r(), LedMode::get_g(), LedMode::get_b()); }

bool ColorSolid::is_done() { return true; }

uint8_t ColorSolid::get_mode_id() {
    return 1;
}

uint8_t* ColorSolid::get_target_rgb() {
    // no extra buffer—just return pointer into LedMode::rgb
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
