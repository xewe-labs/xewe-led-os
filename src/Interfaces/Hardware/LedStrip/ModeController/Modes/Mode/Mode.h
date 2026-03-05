// src/Interfaces/Hardware/LedStrip/Modes/Mode/Mode.h
#pragma once

#include <FastLED.h>
#include <string>
#include <string_view>
#include <vector>

struct ModeParam {
    std::string key;
    std::string display_name;
    uint16_t min_value;
    uint16_t max_value;
    uint16_t default_value;
    uint16_t step_value;
};

struct ModeConfig {
    uint8_t id;
    std::string name;
    std::vector<ModeParam> params;

    ModeConfig(uint8_t init_id, std::string init_name, const std::vector<ModeParam>& custom_params = {})
        : id(init_id), name(init_name)
    {
        params.push_back({"r", "R",   0, 255, 255, 1});
        params.push_back({"g", "G", 0, 255, 255, 1});
        params.push_back({"b", "B",  0, 255, 255, 1});
        params.insert(params.end(), custom_params.begin(), custom_params.end());
    }
};

class Mode {
public:
    explicit Mode(const ModeConfig& mode_config) : config(mode_config) {}
    virtual ~Mode() = default;

    virtual void loop(CRGB* leds, uint16_t num_leds) = 0;

    uint8_t get_id() const { return config.id; }
    std::string_view get_name() const { return config.name; }
    std::vector<ModeParam> get_params() const { return config.params; }
    const ModeConfig& get_config() const { return config; }

protected:
    ModeConfig& config;
};