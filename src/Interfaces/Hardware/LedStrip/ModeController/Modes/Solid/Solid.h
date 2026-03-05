// src/Interfaces/Hardware/LedStrip/Modes/Solid/Solid.h
#pragma once

#include "../Mode/Mode.h"
#include "../../ModeRegistry/ModeRegistry.h"

class Solid : public Mode {
public:
    Solid(const std::map<std::string, uint16_t>& params);
    ~Solid() override = default;

    void loop(CRGB* leds, uint16_t num_leds) override;

    uint8_t get_id() const override;
    ModeConfig get_config() const override;
    std::map<std::string, uint16_t> get_params() const override;

private:
    uint8_t hue;
    uint8_t sat;
    uint8_t val;
};