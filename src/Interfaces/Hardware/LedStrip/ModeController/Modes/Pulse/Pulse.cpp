#include "Pulse.h"

// Assuming Pulse gets ID 2, following Solid (0) and Rainbow (1)
static ModeRegistrar<Pulse> registrar_pulse(2);

Pulse::Pulse(const std::map<std::string, uint16_t>& params)
    : Mode(ModeConfig(2, "Pulse", {
        {"hue", "Hue", 0, 255, 0, 1},
        {"sat", "Saturation", 0, 255, 255, 1},
        {"speed", "Pulse Speed (BPM)", 1, 255, 30, 1}
      }), params)
{
    // Pre-calculate the base color using the injected params
    std::array<uint8_t, 3> precise_rgb = hsv_to_rgb({
        static_cast<uint8_t>(get_param("hue")),
        static_cast<uint8_t>(get_param("sat")),
        255
    });

    rgb = CRGB(precise_rgb[0], precise_rgb[1], precise_rgb[2]);
}

void Pulse::loop(CRGB* leds, uint16_t num_leds) {
    // Calculate brightness using a sine wave based on the speed parameter
    // beatsin8 returns a value between 0-255
    uint8_t brightness = beatsin8(get_param("speed"));

    // Copy the base color and scale it by the current brightness
    CRGB current_color = rgb;
    current_color.nscale8(brightness);

    // Apply to the strip
    fill_solid(leds, num_leds, current_color);
}

std::array<uint8_t, 3> Pulse::get_rgb() {
    return {rgb.r, rgb.g, rgb.b};
}