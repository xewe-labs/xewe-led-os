#include "NorthernLights.h"

static ModeRegistrar<NorthernLights> registrar_northern_lights(5);

NorthernLights::NorthernLights(const std::map<std::string, uint16_t>& params)
    : Mode(ModeConfig(5, "Northern Lights", {
        {"speed", "Speed", 1, 10, 4, 1, 'b'},
      }), params),
      counter(0),
      base_rgb(ColorHSV((HUE_A_LOW + HUE_A_HIGH) / 2, 255, 255))
{
}

void NorthernLights::ensure_buffer(uint16_t num_leds) {
    if (previous_frame.size() != num_leds) {
        previous_frame.assign(num_leds, CRGB(0, 0, 0));
    }
}

void NorthernLights::loop(CRGB* leds, uint16_t num_leds) {
    ensure_buffer(num_leds);

    for (uint16_t i = 0; i < num_leds; i++) {
        const uint16_t noise_val = inoise16(static_cast<uint32_t>(i) * NOISE_SPATIAL_STEP, counter);
        const CRGB target_color = get_weighted_color(noise_val);
        const CRGB smooth_color = blend_colors(previous_frame[i], target_color, BLEND_AMOUNT);

        leds[i] = smooth_color;
        previous_frame[i] = smooth_color;
    }

    counter += get_fire_step();
}

uint16_t NorthernLights::get_fire_step() const {
    // Map UI speed 0..10 to the raw step values your reference code expects.
    // 4 -> 1000, matching your "nice active flow" default.
    static constexpr uint16_t SPEED_MAP[11] = {
        0,    // 0
        200,  // 1
        400,  // 2
        700,  // 3
        1000, // 4
        1500, // 5
        2200, // 6
        3000, // 7
        3800, // 8
        4600, // 9
        5500  // 10
    };

    const uint16_t speed = get_param("speed");
    return SPEED_MAP[(speed <= 10) ? speed : 10];
}

CRGB NorthernLights::get_weighted_color(uint16_t val) const {
    uint16_t hue;

    if (val < 32768) {
        hue = map(val, 0, 32768, HUE_A_LOW, HUE_A_HIGH);
    } else {
        hue = map(val, 32768, 65535, HUE_B_LOW, HUE_B_HIGH);
    }

    const uint8_t sat = map(val, 0, 65535, MAX_SAT - 40, MAX_SAT);
    const uint8_t bri = map(val, 0, 65535, MIN_BRIGHT, MAX_BRIGHT);

    return ColorHSV(hue, sat, bri);
}

CRGB NorthernLights::blend_colors(const CRGB& color1, const CRGB& color2, uint8_t amount) const {
    const uint8_t r = color1.r + (((int16_t)color2.r - color1.r) * amount / 255);
    const uint8_t g = color1.g + (((int16_t)color2.g - color1.g) * amount / 255);
    const uint8_t b = color1.b + (((int16_t)color2.b - color1.b) * amount / 255);

    return CRGB(r, g, b);
}

CRGB NorthernLights::ColorHSV(uint16_t hue, uint8_t sat, uint8_t val) const {
    // Port of Adafruit_NeoPixel::ColorHSV behavior so the palette matches
    // your working reference instead of FastLED's CHSV conversion.
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

std::array<uint8_t, 3> NorthernLights::get_rgb() {
    return {base_rgb.r, base_rgb.g, base_rgb.b};
}