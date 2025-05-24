#include "ColorSolid.h"

ColorSolid::ColorSolid(LedStrip* controller)
    : LedMode(controller) {
}

ColorSolid::ColorSolid(LedStrip* controller, uint8_t r, uint8_t g, uint8_t b)
    : LedMode(controller) {
    LedMode::set_rgb(r, g, b);
}

void ColorSolid::frame() { led_controller->fill_all(LedMode::get_r(), LedMode::get_g(), LedMode::get_b()); }

bool ColorSolid::is_done() { return true; }

uint8_t ColorSolid::get_mode_id() { return 1; }
