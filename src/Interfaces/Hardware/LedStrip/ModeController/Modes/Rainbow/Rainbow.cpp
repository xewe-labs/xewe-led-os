// src/Interfaces/Hardware/LedStrip/Modes/Rainbow/Rainbow.cpp

#include "Rainbow.h"

Rainbow::Rainbow(const std::map<std::string, uint16_t>& params)
    : Mode(ModeConfig(2, "Rainbow", {
        {"speed", "Animation Speed", 0, 255, 5, 1},
        {"density", "Color Density", 1, 255, 10, 1}
      })),
      current_hue(0)
{
    // Inject the requested parameters into the config structure
    for (auto& param : config.params) {
        if (params.count(param.key)) {
            param.default_value = params.at(param.key);
        }
    }
}

void Rainbow::loop(CRGB* leds, uint16_t num_leds) {
    fill_rainbow(leds, num_leds, current_hue, static_cast<uint8_t>(get_param("density")));
    current_hue += get_param("speed");
}

std::array<uint8_t, 3>   Rainbow::get_rgb() {
    return {255, 255, 255}; // always return white since has no true color
}