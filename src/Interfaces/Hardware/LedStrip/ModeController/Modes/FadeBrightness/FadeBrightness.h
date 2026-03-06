#pragma once

#include <map>
#include <string>
#include <array>
#include <FastLED.h>

#include "../Mode/Mode.h"
#include "../../ModeRegistry/ModeRegistry.h"

class FadeBrightness : public Mode {
public:
    explicit FadeBrightness(const std::map<std::string, uint16_t>& params);

    void loop(CRGB* leds, uint16_t num_leds) override;
    std::array<uint8_t, 3> get_rgb() override;

private:
    // Helper to calculate the color with noise-adjusted brightness
    CRGB get_brightness_color(uint8_t noise_val, uint8_t base_hue, uint8_t base_sat, uint8_t min_bright, uint8_t max_bright);

    // Animation state
    uint32_t counter;
    CRGB base_rgb;
};