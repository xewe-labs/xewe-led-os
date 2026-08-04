// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Hardware/LedStrip/ModeController/Modes/FadeColorTwoZone/FadeColorTwoZone.cpp

#include "FadeColorTwoZone.h"


static ModeRegistrar<FadeColorTwoZone> registrar_fade_color_two_zone(2);

FadeColorTwoZone::FadeColorTwoZone(const std::map<std::string, uint16_t>& params)
    : Mode(ModeConfig(2, "Color Fade Two Zone", {
        {"hue", "Hue A", 0, 255, 81, 1, 'b'},
        {"hue_b", "Hue B", 0, 255, 225, 1, 'b'},
        {"blend", "Blend", 2, 255, 150, 1, 'a'},
        {"speed", "Speed", 1, 50, 3, 1, 'a'},
        {"fire_step", "Density", 1, 255, 10, 1, 'a'},
        {"min_bright", "Depth", 0, 255, 245, 1, 'a'},
        {"min_sat", "Min Sat", 0, 255, 215, 1, 'a'},
    }), params)
    , counter(0)
{
    std::array<uint8_t, 3> base_rgb = hsv_to_rgb({
        static_cast<uint8_t>(get_param("hue")),
        255, // Max saturation (matching your original hardcoded 255)
        255  // Max value (matching your original hardcoded 255)
    });
}

void FadeColorTwoZone::loop(CRGB* leds,
                            uint16_t num_leds) {
    ensure_buffer(num_leds);

    const uint32_t spatial_step = get_noise_spatial_step();
    const uint8_t  blend_amount = get_blend_amount();

    for (uint16_t i = 0; i < num_leds; i++) {
        const uint16_t noise_val    = inoise16(static_cast<uint32_t>(i) * spatial_step, counter);
        const CRGB     target_color = get_weighted_color(noise_val);
        const CRGB     smooth_color = blend_colors(previous_frame[i], target_color, blend_amount);

        leds[i]                     = smooth_color;
        previous_frame[i]           = smooth_color;
    }

    counter += get_speed_step();
}

std::array<uint8_t, 3> FadeColorTwoZone::get_rgb() {
    return base_rgb;
}
void FadeColorTwoZone::ensure_buffer(uint16_t num_leds) {
    if (previous_frame.size() != num_leds) {
        previous_frame.assign(num_leds, CRGB(0, 0, 0));
    }
}

uint16_t FadeColorTwoZone::get_speed_step() const {
    uint8_t speed = static_cast<uint8_t>(get_param("speed"));
    if (speed < 1) speed = 1;
    if (speed > 50) speed = 50;

    return static_cast<uint16_t>(speed * 250);
}

uint32_t FadeColorTwoZone::get_noise_spatial_step() const {
    uint8_t density = static_cast<uint8_t>(get_param("fire_step"));
    if (density < 1) density = 1;

    return static_cast<uint32_t>(density) * 400UL;
}

uint8_t FadeColorTwoZone::get_blend_amount() const {
    uint8_t blend = static_cast<uint8_t>(get_param("blend"));
    if (blend < 1) blend = 1;
    return blend;
}

CRGB FadeColorTwoZone::get_weighted_color(uint16_t val) const {
    const uint8_t hue_a      = static_cast<uint8_t>(get_param("hue"));
    const uint8_t hue_b      = static_cast<uint8_t>(get_param("hue_b"));
    const uint8_t min_bright = static_cast<uint8_t>(get_param("min_bright"));
    const uint8_t min_sat    = static_cast<uint8_t>(get_param("min_sat"));

    const uint8_t hue        = map(val, 0, 65535, hue_a, hue_b);
    const uint8_t sat        = static_cast<uint8_t>(map(val, 0, 65535, min_sat, MAX_SAT));
    const uint8_t bri        = static_cast<uint8_t>(map(val, 0, 65535, min_bright, MAX_BRIGHT));

    return ColorHSV(hue, sat, bri);
}

CRGB FadeColorTwoZone::ColorHSV(uint8_t hue8,
                                uint8_t sat,
                                uint8_t val) const {
    uint8_t  r, g, b;

    uint16_t hue = hue8 * 6;

    if (hue < 510) {
        b = 0;
        if (hue < 255) {
            r = 255;
            g = hue;
        } else {
            r = 510 - hue;
            g = 255;
        }
    } else if (hue < 1020) {
        r = 0;
        if (hue < 765) {
            g = 255;
            b = hue - 510;
        } else {
            g = 1020 - hue;
            b = 255;
        }
    } else if (hue < 1530) {
        g = 0;
        if (hue < 1275) {
            r = hue - 1020;
            b = 255;
        } else {
            r = 255;
            b = 1530 - hue;
        }
    } else {
        r = 255;
        g = 0;
        b = 0;
    }

    const uint32_t v1 = 1 + val;
    const uint16_t s1 = 1 + sat;
    const uint8_t  s2 = 255 - sat;

    r                 = (((((uint16_t)r * s1) >> 8) + s2) * v1) >> 8;
    g                 = (((((uint16_t)g * s1) >> 8) + s2) * v1) >> 8;
    b                 = (((((uint16_t)b * s1) >> 8) + s2) * v1) >> 8;

    return CRGB(r, g, b);
}

CRGB FadeColorTwoZone::blend_colors(const CRGB& color1,
                                    const CRGB& color2,
                                    uint8_t amount) const {
    const uint8_t r = color1.r + (((int16_t)color2.r - color1.r) * amount / 255);
    const uint8_t g = color1.g + (((int16_t)color2.g - color1.g) * amount / 255);
    const uint8_t b = color1.b + (((int16_t)color2.b - color1.b) * amount / 255);

    return CRGB(r, g, b);
}
