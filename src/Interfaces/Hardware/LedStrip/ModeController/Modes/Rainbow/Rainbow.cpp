// src/Interfaces/Hardware/LedStrip/Modes/Rainbow/Rainbow.cpp

#include "Rainbow.h"

// --- AUTO REGISTRATION ---
// Registers Rainbow mode with ID 1. Runs automatically at startup.
static ModeRegistrar<Rainbow> registrar_rainbow(1);

Rainbow::Rainbow(const std::map<std::string, uint16_t>& params)
    : current_hue(0)
{
    // Extract parameters or fall back to sensible defaults
    speed = params.count("speed") ? params.at("speed") : 2;
    scale = params.count("scale") ? params.at("scale") : 5;
}

void Rainbow::loop(CRGB* leds, uint16_t num_leds) {
    fill_rainbow(leds, num_leds, current_hue, scale);

    // Increment the starting hue to animate the rainbow
    // In a real production app, you might want to tie this to a timer
    // rather than raw loop speed, but this works perfectly for testing.
    EVERY_N_MILLISECONDS(20) {
        current_hue += speed;
    }
}

uint8_t Rainbow::get_id() const {
    return 1;
}

ModeConfig Rainbow::get_config() const {
    return {
        1,
        "Classic Rainbow",
        {
            {"speed", "Animation Speed", 0, 20, 2, 1},
            {"scale", "Rainbow Density", 1, 50, 5, 1}
        }
    };
}

std::map<std::string, uint16_t> Rainbow::get_params() const {
    return {
        {"speed", speed},
        {"scale", scale}
    };
}