#include "FadeColorTwoZone.h"

static ModeRegistrar<FadeColorTwoZone> registrar_fade_color_two_zone(2);

FadeColorTwoZone::FadeColorTwoZone(const std::map<std::string, uint16_t>& params)
    : Mode(ModeConfig(2, "Color Fade Two Zone", {
        {"hue_a",      "Hue A",   0,     65535, 21000, 1, 'b'},
        {"hue_b",      "Hue B",   0,     65535, 58000, 1, 'b'},
        {"blend",      "Blend",   2,     255,   150,   1, 'a'},
        {"speed",      "Speed",   1,     50,    3,     1, 'a'},
        {"fire_step",  "Density", 1,     255,   10,    1, 'a'},
        {"min_bright", "Depth",   0,     255,   245,   1, 'a'},
        {"min_sat",    "Min Sat", 0,     255,   215,   1, 'a'}
      }), params),
      counter(0),
      base_rgb(ColorHSV((20500 + 63250) / 2, 255, 255))
{
}

void FadeColorTwoZone::ensure_buffer(uint16_t num_leds) {
    if (previous_frame.size() != num_leds) {
        previous_frame.assign(num_leds, CRGB(0, 0, 0));
    }
}

void FadeColorTwoZone::loop(CRGB* leds, uint16_t num_leds) {
    ensure_buffer(num_leds);

    const uint32_t spatial_step = get_noise_spatial_step();
    const uint8_t blend_amount = get_blend_amount();

    for (uint16_t i = 0; i < num_leds; i++) {
        const uint16_t noise_val = inoise16(static_cast<uint32_t>(i) * spatial_step, counter);
        const CRGB target_color = get_weighted_color(noise_val);
        const CRGB smooth_color = blend_colors(previous_frame[i], target_color, blend_amount);

        leds[i] = smooth_color;
        previous_frame[i] = smooth_color;
    }

    counter += get_speed_step();
}

uint16_t FadeColorTwoZone::get_speed_step() const {
    uint16_t speed = get_param("speed");
    if (speed < 1) speed = 1;
    if (speed > 50) speed = 50;

    // default 4 -> 1000, matching old NorthernLights feel
    return static_cast<uint16_t>(speed * 250);
}

uint32_t FadeColorTwoZone::get_noise_spatial_step() const {
    uint16_t density = get_param("fire_step");
    if (density < 1) density = 1;
    if (density > 255) density = 255;

    // default 20 -> 8000, matching old NorthernLights spacing
    return static_cast<uint32_t>(density) * 400UL;
}

uint8_t FadeColorTwoZone::get_blend_amount() const {
    uint16_t blend = get_param("blend");
    if (blend < 1) blend = 1;
    if (blend > 255) blend = 255;
    return static_cast<uint8_t>(blend);
}

CRGB FadeColorTwoZone::get_weighted_color(uint16_t val) const {
    const uint16_t hue_a = get_param("hue_a");
    const uint16_t hue_b = get_param("hue_b");

    uint16_t min_bright = get_param("min_bright");
    if (min_bright > 255) min_bright = 255;

    uint16_t min_sat = get_param("min_sat");
    if (min_sat > 255) min_sat = 255;

    const uint16_t hue = map(val, 0, 65535, hue_a, hue_b);
    const uint8_t sat = static_cast<uint8_t>(map(val, 0, 65535, min_sat, MAX_SAT));
    const uint8_t bri = static_cast<uint8_t>(map(val, 0, 65535, min_bright, MAX_BRIGHT));

    return ColorHSV(hue, sat, bri);
}

CRGB FadeColorTwoZone::blend_colors(const CRGB& color1, const CRGB& color2, uint8_t amount) const {
    const uint8_t r = color1.r + (((int16_t)color2.r - color1.r) * amount / 255);
    const uint8_t g = color1.g + (((int16_t)color2.g - color1.g) * amount / 255);
    const uint8_t b = color1.b + (((int16_t)color2.b - color1.b) * amount / 255);

    return CRGB(r, g, b);
}

CRGB FadeColorTwoZone::ColorHSV(uint16_t hue, uint8_t sat, uint8_t val) const {
    uint8_t r, g, b;

    hue = (uint32_t(hue) * 1530L + 32768) / 65536;

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
    const uint8_t s2 = 255 - sat;

    r = (((((uint16_t)r * s1) >> 8) + s2) * v1) >> 8;
    g = (((((uint16_t)g * s1) >> 8) + s2) * v1) >> 8;
    b = (((((uint16_t)b * s1) >> 8) + s2) * v1) >> 8;

    return CRGB(r, g, b);
}

std::array<uint8_t, 3> FadeColorTwoZone::get_rgb() {
    return {base_rgb.r, base_rgb.g, base_rgb.b};
}