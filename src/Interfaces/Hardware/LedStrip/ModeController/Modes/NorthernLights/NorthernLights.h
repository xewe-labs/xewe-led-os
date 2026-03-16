// NorthernLights.h
#pragma once

#include "../Mode/Mode.h"
#include "../../ModeRegistry/ModeRegistry.h"

// Replace this include with the real header that defines xewe::color::hsv_to_rgb(...)
#include "../../Color/ColorUtils.h"

#include <array>
#include <vector>

class NorthernLights final : public Mode {
public:
    explicit NorthernLights(const std::map<std::string, uint16_t>& params);

    void loop(CRGB* leds, uint16_t num_leds) override;
    std::array<uint8_t, 3> get_rgb() override;

private:
    static constexpr uint16_t HUE_A_LOW          = 60;
    static constexpr uint16_t HUE_A_HIGH         = 70;
    static constexpr uint16_t HUE_B_LOW          = 235;
    static constexpr uint16_t HUE_B_HIGH         = 255;

    static constexpr uint16_t NOISE_SPATIAL_STEP = 8000;
    static constexpr uint8_t  MIN_BRIGHT         = 10;
    static constexpr uint8_t  MAX_BRIGHT         = 255;
    static constexpr uint8_t  MAX_SAT            = 255;
    static constexpr uint8_t  MIN_SAT_USED       = MAX_SAT - 40;
    static constexpr uint8_t  BLEND_AMOUNT       = 35;
    static constexpr uint16_t MAX_SPEED_UI       = 10;
    static constexpr size_t   PALETTE_SIZE       = 256;

    void build_palette();
    std::array<CRGB, PALETTE_SIZE> palette_{};

    uint32_t counter = 0;
    uint16_t fire_speed = 0;
};