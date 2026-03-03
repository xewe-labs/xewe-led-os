#include "FadeBrightness.h"

FadeBrightness::FadeBrightness(uint16_t num_leds, const std::array<uint8_t, 3>& rgb)
    : Mode(num_leds, 1, "FadeBrightness", rgb), counter(0)
{}

const CRGB* FadeBrightness::loop() {
    // 1. Get the current Base Hue and Saturation
    CRGB current_rgb = CRGB(_rgb[0], _rgb[1], _rgb[2]);
    CHSV base_hsv = rgb2hsv_approximate(current_rgb);

    // 2. Frame Update
    for (int i = 0; i < _num_leds; i++) {
        // Calculate noise
        uint8_t noise = inoise8(i * NOISE_STEP, counter);

        // Calculate the final color keeping hue/sat constant and varying brightness
        _leds[i] = get_brightness_color(noise, base_hsv.hue, base_hsv.sat);
    }

    // 3. Increment counter (Speed)
    counter += SPEED;

    return _leds.data();
}

CRGB FadeBrightness::get_brightness_color(uint8_t val, uint8_t base_hue, uint8_t base_sat) {
    // Map the 0-255 noise value to our desired brightness range
    uint8_t calculated_val = constrain(map(val, 0, 255, MIN_BRIGHT, MAX_BRIGHT), 0, 255);

    return CHSV(base_hue, base_sat, calculated_val);
}

// Factory Implementation
std::unique_ptr<Mode> make_mode_fadebrightness(uint16_t num_leds, const std::array<uint8_t, 3>& rgb) {
    return std::make_unique<FadeBrightness>(num_leds, rgb);
}