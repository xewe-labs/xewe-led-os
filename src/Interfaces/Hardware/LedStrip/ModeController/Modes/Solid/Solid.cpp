// src/Interfaces/Hardware/LedStrip/Modes/Solid/Solid.cpp

#include "Solid.h"

// --- AUTO REGISTRATION ---
static ModeRegistrar<Solid> registrar_solid(0);

// --- CONFIGURATION DEFINITION ---
const ModeConfig Solid::config = {
    0,
    "Solid Color",
    {
        // key, display_name, min_value, max_value, default_value, step_value
        {"hue", "Hue", 0, 255, 0, 1},
        {"sat", "Saturation", 0, 255, 0, 1},
        {"val", "Brightness", 0, 255, 0, 1}
    }
};

Solid::Solid(const std::map<std::string, uint16_t>& params) {
    // Dynamically build state based on the config struct
    for (const auto& param_def : config.params) {
        if (params.count(param_def.key)) {
            // Clamp the provided value to enforce the config limits safely
            current_params[param_def.key] = std::clamp(params.at(param_def.key), param_def.min_value, param_def.max_value);
        } else {
            // Fall back to the configured default
            current_params[param_def.key] = param_def.default_value;
        }
    }
}

void Solid::loop(CRGB* leds, uint16_t num_leds) {
    // Read directly from the validated parameters
    fill_solid(leds, num_leds, CHSV(
        current_params["hue"],
        current_params["sat"],
        current_params["val"]
    ));
}

uint8_t Solid::get_id() const {
    return config.mode_id;
}

ModeConfig Solid::get_config() const {
    return config;
}

std::map<std::string, uint16_t> Solid::get_params() const {
    return current_params;
}