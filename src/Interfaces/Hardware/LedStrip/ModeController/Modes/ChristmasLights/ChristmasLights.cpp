#include "ChristmasLights.h"

static ModeRegistrar<ChristmasLights> registrar_christmas_lights(6);

constexpr CRGB ChristmasLights::palette[5];

ChristmasLights::ChristmasLights(const std::map<std::string, uint16_t>& params)
    : Mode(ModeConfig(6, "Christmas Lights", {
        {"speed", "Flicker", 0, 20, 3, 1, 'b'}
      }), params)
{
    for (uint16_t& offset : noise_offsets) {
        offset = random16();
    }
}

void ChristmasLights::loop(CRGB* leds, uint16_t num_leds) {

    for (uint16_t i = 0; i < num_leds; i++) {
        leds[i] = palette[i % 5];
        leds[i].nscale8_video(inoise8(noise_offsets[i], z));
    }

    z += get_param("speed");
}

std::array<uint8_t, 3> ChristmasLights::get_rgb() {

    return {85, 49, 22};
}