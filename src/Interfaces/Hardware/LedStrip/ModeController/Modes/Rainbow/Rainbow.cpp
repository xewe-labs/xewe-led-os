// src/Interfaces/Hardware/LedStrip/Modes/Rainbow/Rainbow.cpp

#include "Rainbow.h"

// --- AUTO REGISTRATION ---
static ModeRegistrar<Rainbow> registrar_rainbow(1);

// --- CONFIGURATION DEFINITION ---
const ModeConfig Rainbow::config = {
    1,
    "Classic Rainbow",
    {
        // key, display_name, min_value, max_value, default_value, step_value
        {"speed", "Animation Speed", 0, 20, 2, 1},
        {"scale", "Rainbow Density", 1, 50, 5, 1}
    }
};

Rainbow::Rainbow(const std::map<std::string, uint16_t>& params)
    : current_hue(0)
{
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

void Rainbow::loop(CRGB* leds, uint16_t num_leds) {
    // Read directly from the validated parameters
    fill_rainbow(leds, num_leds, current_hue, current_params["scale"]);

    EVERY_N_MILLISECONDS(20) {
        current_hue += current_params["speed"];
    }
}

uint8_t Rainbow::get_id() const {
    return config.mode_id;
}

ModeConfig Rainbow::get_config() const {
    return config;
}

std::map<std::string, uint16_t> Rainbow::get_params() const {
    return current_params;
}