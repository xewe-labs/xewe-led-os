// src/Interfaces/Hardware/LedStrip/Modes/Solid/Solid.cpp

#include "Solid.h"

// --- AUTO REGISTRATION ---
// Registers Solid mode with ID 0. Runs automatically at startup.
static ModeRegistrar<Solid> registrar_solid(0);

Solid::Solid(const std::map<std::string, uint16_t>& params) {
    // Extract parameters or fall back to defaults (e.g., pure Red)
    hue = params.count("hue") ? params.at("hue") : 0;
    sat = params.count("sat") ? params.at("sat") : 255;
    val = params.count("val") ? params.at("val") : 255;
}

void Solid::loop(CRGB* leds, uint16_t num_leds) {
    fill_solid(leds, num_leds, CHSV(hue, sat, val));
}

uint8_t Solid::get_id() const {
    return 0;
}

ModeConfig Solid::get_config() const {
    return {
        0,
        "Solid Color",
        {
            {"hue", "Hue", 0, 255, 0, 1},
            {"sat", "Saturation", 0, 255, 255, 1},
            {"val", "Brightness", 0, 255, 255, 1}
        }
    };
}

std::map<std::string, uint16_t> Solid::get_params() const {
    return {
        {"hue", hue},
        {"sat", sat},
        {"val", val}
    };
}