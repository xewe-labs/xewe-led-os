// src/Interfaces/Hardware/LedStrip/Modes/Rainbow/Rainbow.h
#pragma once

#include <map>
#include <string>
#include <FastLED.h>
#include "../Mode/Mode.h"
#include "../../ModeController/ModeRegistry/ModeRegistry.h"

static ModeRegistrar<Rainbow> registrar_rainbow(1);

class Rainbow : public Mode {
public:
    explicit Rainbow(const std::map<std::string, uint16_t>& params);
    void loop(CRGB* leds, uint16_t num_leds) override;

private:
    uint16_t current_hue; // Internal animation state, NOT a parameter
};