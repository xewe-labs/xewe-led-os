#include "Solid.h"

Solid::Solid() : h(127), s(127), v(127) {}

void Solid::render(CRGB* buffer, uint16_t num_leds) {
    // Fill the entire buffer with the specified HSV color
    fill_solid(buffer, num_leds, CHSV(h, s, v));
}

std::vector<ModeParameter> Solid::get_params() const {
    return {
        {"h", 0, 255, h, 127},
        {"s", 0, 255, s, 127},
        {"v", 0, 255, v, 127}
    };
}

bool Solid::set_param(const std::string& name, int value) {
    if (name == "h") { h = constrain(value, 0, 255); return true; }
    if (name == "s") { s = constrain(value, 0, 255); return true; }
    if (name == "v") { v = constrain(value, 0, 255); return true; }
    return false;
}