// src/Interfaces/Hardware/LedStrip/Modes/Solid/Solid.h
#pragma once

#include "../Mode/Mode.h"
#include "../../ModeRegistry/ModeRegistry.h"

class Solid : public Mode {
public:
    explicit Solid(const std::map<std::string, uint16_t>& params);
    ~Solid() override = default;

    void loop(CRGB* leds, uint16_t num_leds) override;
};