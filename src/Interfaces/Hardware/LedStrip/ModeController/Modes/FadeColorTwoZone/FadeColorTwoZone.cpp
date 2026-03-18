#include "FadeColorTwoZone.h"

static ModeRegistrar<FadeColorTwoZone> registrar_color_fade_two_zone(5);

FadeColorTwoZone::FadeColorTwoZone(const std::map<std::string, uint16_t>& params)
    : Mode(ModeConfig(5, "Color Fade Two Zone", {
        {"hue_a",      "Hue A",   0,     65535, 20000, 1, 'a'},
        {"sat_a",      "Sat A",   0,     255,   255,   1, 'a'},
        {"hue_b",      "Hue B",   0,     65535, 63000, 1, 'a'},
        {"sat_b",      "Sat B",   0,     255,   255,   1, 'a'},
        {"blend",      "Blend",   1,     255,   35,    1, 'a'},
        {"speed",      "Speed",   1,     50,    4,     1, 'a'},
        {"fire_step",  "Density", 1,     255,   20,    1, 'a'},
        {"min_bright", "Depth",   0,     255,   150,   1, 'a'},
      }), params),
      counter(0),
      base_rgb(0, 0, 0)
{
    const CRGB color_a = ColorHSV(get_param("hue_a"), get_param("sat_a"), 255);
    const CRGB color_b = ColorHSV(get_param("hue_b"), get_param("sat_b"), 255);

    base_rgb = blend_colors(color_a, color_b, 128);
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

    if (speed < 1) {
        speed = 1;
    } else if (speed > 50) {
        speed = 50;
    }

    // Keeps default speed=4 aligned with your old 1000-step behavior.
    return static_cast<uint16_t>(speed * 250);
}

uint32_t FadeColorTwoZone::get_noise_spatial_step() const {
    uint16_t density = get_param("fire_step");

    if (density < 1) {
        density = 1;
    } else if (density > 255) {
        density = 255;
    }

    // Keeps default density=20 aligned with the old 8000 spatial step.
    return static_cast<uint32_t>(density) * 400UL;
}

uint8_t FadeColorTwoZone::get_blend_amount() const {
    uint16_t blend = get_param("blend");

    if (blend < 1) {
        blend = 1;
    } else if (blend > 255) {
        blend = 255;
    }

    return static_cast<uint8_t>(blend);
}

CRGB FadeColorTwoZone::get_weighted_color(uint16_t val) const {
    const uint16_t hue_a = get_param("hue_a");
    const uint8_t sat_a = static_cast<uint8_t>(get_param("sat_a") > 255 ? 255 : get_param("sat_a"));
    const uint16_t hue_b = get_param("hue_b");
    const uint8_t sat_b = static_cast<uint8_t>(get_param("sat_b") > 255 ? 255 : get_param("sat_b"));

    uint16_t min_bright = get_param("min_bright");
    if (min_bright > 255) {
        min_bright = 255;
    }

    const CRGB color_a = ColorHSV(hue_a, sat_a, 255);
    const CRGB color_b = ColorHSV(hue_b, sat_b, 255);

    const uint8_t mix = static_cast<uint8_t>(val >> 8);
    const uint8_t bri = static_cast<uint8_t>(map(val, 0, 65535, min_bright, MAX_BRIGHT));

    const CRGB mixed = blend_colors(color_a, color_b, mix);
    return scale_color(mixed, bri);
}

CRGB FadeColorTwoZone::scale_color(const CRGB& color, uint8_t brightness) const {
    const uint8_t r = static_cast<uint8_t>((static_cast<uint16_t>(color.r) * brightness) / 255);
    const uint8_t g = static_cast<uint8_t>((static_cast<uint16_t>(color.g) * brightness) / 255);
    const uint8_t b = static_cast<uint8_t>((static_cast<uint16_t>(color.b) * brightness) / 255);

    return CRGB(r, g, b);
}

CRGB FadeColorTwoZone::blend_colors(const CRGB& color1, const CRGB& color2, uint8_t amount) const {
    const uint8_t r = color1.r + (((int16_t)color2.r - color1.r) * amount / 255);
    const uint8_t g = color1.g + (((int16_t)color2.g - color1.g) * amount / 255);
    const uint8_t b = color1.b + (((int16_t)color2.b - color1.b) * amount / 255);

    return CRGB(r, g, b);
}

CRGB FadeColorTwoZone::ColorHSV(uint16_t hue, uint8_t sat, uint8_t val) const {
    // Port of Adafruit_NeoPixel::ColorHSV behavior for palette consistency.
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