// src/Interfaces/Hardware/LedStrip/Modes/Mode/Mode.h
#pragma once

#include <map>
#include <FastLED.h>
#include <string>
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
};

class Mode {
public:
    virtual void loop(CRGB* leds, uint16_t num_leds) = 0;


    uint8_t get_id() const { return config.id; };
    string_view get_name() const { return config.name; };
    vector<ModeParam> get_params() const { return config.params; };
    ModeConfig get_config() const { return config; };

    std::map<std::string, uint16_t> get_params() const { return config->params; };
private :
    static const ModeConfig config;
};