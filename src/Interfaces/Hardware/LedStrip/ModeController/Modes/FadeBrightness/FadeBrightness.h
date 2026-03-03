#pragma once

#include "../Mode/Mode.h"
#include <memory>

class FadeBrightness : public Mode {
public:
    FadeBrightness(uint16_t num_leds, const std::array<uint8_t, 3>& rgb);

    const CRGB* loop() override;

private:
    // Helper to calculate the color with noise-adjusted brightness
    CRGB get_brightness_color(uint8_t noise_val, uint8_t base_hue, uint8_t base_sat);

    // Animation state
    uint32_t counter;

    // --- Configuration ---
    static const int NOISE_STEP  = 10;
    static const int SPEED       = 5;
    static const int MIN_BRIGHT  = 10;
    static const int MAX_BRIGHT  = 255;
};

// Factory declaration
std::unique_ptr<Mode> make_mode_fadebrightness(uint16_t num_leds, const std::array<uint8_t, 3>& rgb);