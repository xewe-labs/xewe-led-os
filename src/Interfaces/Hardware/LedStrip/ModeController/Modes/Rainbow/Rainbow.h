// src/Interfaces/Hardware/LedStrip/Modes/Rainbow/Rainbow.h
#pragma once

#include "../Mode/Mode.h"
#include "../../ModeRegistry/ModeRegistry.h"

class Rainbow : public Mode {
public:
    explicit Rainbow(const std::map<std::string, uint16_t>& params);
    ~Rainbow() override = default;

    void loop(CRGB* leds, uint16_t num_leds) override;

private:
    uint8_t current_hue;
};