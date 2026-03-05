// src/Interfaces/Hardware/LedStrip/Modes/Rainbow/Rainbow.h
#pragma once

#include "../Mode/Mode.h"
#include "../../ModeRegistry/ModeRegistry.h"

class Rainbow : public Mode {
public:
    explicit Rainbow(const std::map<std::string, uint16_t>& params);
    ~Rainbow() override = default;

    void loop(CRGB* leds, uint16_t num_leds) override;

    uint8_t get_id() const override;
    ModeConfig get_config() const override;
    std::map<std::string, uint16_t> get_params() const override;

private:
    std::map<std::string, uint16_t> current_params;
    uint8_t current_hue; // Internal animation state (not a configurable parameter)

    static const ModeConfig config; // The single source of truth
};