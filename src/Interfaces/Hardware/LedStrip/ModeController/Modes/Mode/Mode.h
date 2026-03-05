// src/Interfaces/Hardware/LedStrip/Modes/Mode/Mode.h
#pragma once

#include <map>
#include <FastLED.h>
#include <string>
#include <vector> // I'm also adding this proactively, as your ModeConfig uses std::vector!

struct ModeParam {
    std::string key;
    std::string display_name;
    uint16_t min_value;
    uint16_t max_value;
    uint16_t default_value;
    uint16_t step_value;
};

struct ModeConfig {
    uint8_t mode_id;
    std::string mode_name;
    std::vector<ModeParam> params;
};

class Mode {
public:
    virtual ~Mode() = default;

    virtual void loop(CRGB* leds, uint16_t num_leds) = 0;

    virtual uint8_t get_id() const = 0;
    virtual ModeConfig get_config() const = 0;

    virtual std::map<std::string, uint16_t> get_params() const = 0;
};