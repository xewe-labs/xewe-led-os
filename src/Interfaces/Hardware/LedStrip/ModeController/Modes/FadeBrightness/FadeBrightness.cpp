#include "FadeBrightness.h"

// Assuming FadeBrightness gets ID 4
static ModeRegistrar<FadeBrightness> registrar_fade_brightness(4);

FadeBrightness::FadeBrightness(const std::map<std::string, uint16_t>& params)
    : Mode(ModeConfig(4, "FadeBrightness", {
        {"hue", "Hue", 0, 255, 0, 1},
        {"sat", "Saturation", 0, 255, 255, 1},
        {"speed", "Animation Speed", 1, 50, 5, 1},
        {"noise_step", "Noise Step", 1, 255, 10, 1},
        {"min_bright", "Min Brightness", 0, 255, 10, 1},
        {"max_bright", "Max Brightness", 0, 255, 255, 1}
      }), params),
      counter(0)
{
    // Cache the base RGB for the get_rgb() requirement
    std::array<uint8_t, 3> precise_rgb = hsv_to_rgb({
        static_cast<uint8_t>(get_param("hue")),
        static_cast<uint8_t>(get_param("sat")),
        255
    });

    base_rgb = CRGB(precise_rgb[0], precise_rgb[1], precise_rgb[2]);
}

void FadeBrightness::loop(CRGB* leds, uint16_t num_leds) {
    // 1. Fetch parameters needed for the frame
    // Pulling these outside the loop saves CPU cycles since get_param iterates over a vector
    uint8_t hue = get_param("hue");
    uint8_t sat = get_param("sat");
    uint16_t noise_step = get_param("noise_step");
    uint8_t min_bright = get_param("min_bright");
    uint8_t max_bright = get_param("max_bright");

    // 2. Frame Update
    for (int i = 0; i < num_leds; i++) {
        // Calculate noise
        uint8_t noise = inoise8(i * noise_step, counter);

        // Calculate the final color keeping hue/sat constant and varying brightness
        leds[i] = get_brightness_color(noise, hue, sat, min_bright, max_bright);
    }

    // 3. Increment counter (Speed)
    counter += get_param("speed");
}

CRGB FadeBrightness::get_brightness_color(uint8_t val, uint8_t base_hue, uint8_t base_sat, uint8_t min_bright, uint8_t max_bright) {
    // Map the 0-255 noise value to our desired brightness range
    uint8_t calculated_val = constrain(map(val, 0, 255, min_bright, max_bright), 0, 255);

    return CHSV(base_hue, base_sat, calculated_val);
}

std::array<uint8_t, 3> FadeBrightness::get_rgb() {
    return {base_rgb.r, base_rgb.g, base_rgb.b};
}