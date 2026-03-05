// src/Interfaces/Hardware/LedStrip/Modes/Rainbow/Rainbow.h
#pragma once

#include "../Mode/Mode.h"
#include "../../ModeRegistry/ModeRegistry.h"

// #include <FastLED.h> // Uncomment if not included globally

class Rainbow : public Mode {
public:
    Rainbow(const std::map<std::string, uint16_t>& params);
    ~Rainbow() override = default;

    void loop(CRGB* leds, uint16_t num_leds) override;

    uint8_t get_id() const override;
    ModeConfig get_config() const override;
    std::map<std::string, uint16_t> get_params() const override;

private:
    uint8_t speed;
    uint8_t scale;
    uint8_t current_hue; // Tracks the animation state internally
};