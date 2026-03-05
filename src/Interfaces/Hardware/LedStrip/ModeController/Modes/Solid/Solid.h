// src/Interfaces/Hardware/LedStrip/Modes/Solid/Solid.h
#pragma once

#include <map>
#include <string>
#include <FastLED.h>

#include "../Mode/Mode.h"
#include "../../ModeRegistry/ModeRegistry.h"

class Solid : public Mode {
public:
    explicit Solid(const std::map<std::string, uint16_t>& params);

    void                loop(CRGB* leds, uint16_t num_leds) override;
    std::array<uint8_t, 3>   get_rgb()       override;

};

static ModeRegistrar<Solid> registrar_solid(0);
