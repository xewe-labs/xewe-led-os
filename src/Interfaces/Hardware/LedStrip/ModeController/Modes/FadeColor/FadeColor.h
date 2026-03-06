#pragma once

#include <map>
#include <string>
#include <array>
#include <FastLED.h>

#include "../Mode/Mode.h"
#include "../../ModeRegistry/ModeRegistry.h"

class FadeColor : public Mode {
public:
    explicit FadeColor(const std::map<std::string, uint16_t>& params);

    void loop(CRGB* leds, uint16_t num_leds) override;
    std::array<uint8_t, 3> get_rgb() override;

private:
    // Helper to replicate the 16-bit math from your original algorithm
    CRGB get_fire_color(uint8_t noise_val, long base_hue_16bit);

    // Wrapper to handle 16-bit hue -> 8-bit FastLED conversion
    CRGB ColorHSV(long hue, uint8_t sat, uint8_t val);

    // Animation state
    uint32_t counter;
    CRGB base_rgb;
};