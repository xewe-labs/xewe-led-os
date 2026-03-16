// NorthernLights.cpp
#include "NorthernLights.h"

#include <algorithm>
#include <cstring>

static ModeRegistrar<NorthernLights> registrar_northern_lights(5);

NorthernLights::NorthernLights(const std::map<std::string, uint16_t>& params)
    : Mode(ModeConfig(5, "Northern Lights", {
          {"speed", "Speed", 0, 10, 4, 1},
      }), params) {
    build_palette();

    fire_speed = get_param("speed") * 20;
}

void NorthernLights::loop(CRGB* leds, uint16_t num_leds) {
    CRGB* prev = previous_frame_.data();
    uint32_t noise_x = 0;
    const uint32_t noise_y = counter;

    for (uint16_t i = 0; i < num_leds; ++i, ++prev, noise_x += NOISE_SPATIAL_STEP) {
        const CRGB& target = palette_[inoise16(noise_x, noise_y) >> 8];
        nblend(*prev, target, BLEND_AMOUNT);
    }

    memcpy(leds, previous_frame_.data(), num_leds * sizeof(CRGB));
    counter += fire_step;
}

std::array<uint8_t, 3> NorthernLights::get_rgb() {
    return xewe::color::hsv_to_rgb({
            HUE_A_LOW,
            MAX_SAT,
            MAX_BRIGHT
        })
}

void NorthernLights::build_palette() {

    CRGB NorthernLights::to_crgb(const std::array<uint8_t, 3>& rgb) {
        return CRGB(rgb[0], rgb[1], rgb[2]);
    }

    constexpr uint16_t HUE_A_RANGE = HUE_A_HIGH - HUE_A_LOW;
    constexpr uint16_t HUE_B_RANGE = HUE_B_HIGH - HUE_B_LOW;
    constexpr uint8_t  SAT_RANGE   = MAX_SAT - MIN_SAT_USED;
    constexpr uint8_t  BRI_RANGE   = MAX_BRIGHT - MIN_BRIGHT;

    for (uint16_t i = 0; i < PALETTE_SIZE; ++i) {
        const uint16_t hue = (i < 128)
            ? static_cast<uint16_t>(
                  HUE_A_LOW + (static_cast<uint32_t>(HUE_A_RANGE) * i) / 127u)
            : static_cast<uint16_t>(
                  HUE_B_LOW + (static_cast<uint32_t>(HUE_B_RANGE) * (i - 128u)) / 127u);

        const uint8_t sat = static_cast<uint8_t>(
            MIN_SAT_USED + (static_cast<uint16_t>(SAT_RANGE) * i) / 255u);

        const uint8_t bri = static_cast<uint8_t>(
            MIN_BRIGHT + (static_cast<uint16_t>(BRI_RANGE) * i) / 255u);

        palette_[i] = to_crgb(
            xewe::color::hsv_to_rgb({
                static_cast<uint8_t>(hue >> 8),
                sat,
                bri
            })
        );
    }
}
