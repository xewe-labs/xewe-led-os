// src/Interfaces/Hardware/LedStrip/Modes/Solid/Solid.cpp
#include "Solid.h"

Solid::Solid(const std::map<std::string, uint16_t>& params)
    : Mode(ModeConfig(0, "Solid", {
        {"hue", "Hue", 0, 255, 0, 1},
        {"sat", "Saturation", 0, 255, 255, 1}
      }), params)
{
    std::array<uint8_t, 3> precise_rgb = hsv_to_rgb({
        static_cast<uint8_t>(get_param("hue")),
        static_cast<uint8_t>(get_param("sat")),
        255
    });

    rgb = CRGB(precise_rgb[0], precise_rgb[1], precise_rgb[2]);
}

void Solid::loop(CRGB* leds, uint16_t num_leds) {
    fill_solid(leds, num_leds, rgb);
}

std::array<uint8_t, 3> Solid::get_rgb() {
    return {rgb.r, rgb.g, rgb.b};
}