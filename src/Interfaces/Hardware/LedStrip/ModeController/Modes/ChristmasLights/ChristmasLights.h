#pragma once

#include "../Mode/Mode.h"
#include "../../ModeRegistry/ModeRegistry.h"

class ChristmasLights : public Mode {
public:
    explicit                    ChristmasLights             (const std::map<std::string, uint16_t>& params);

    void                        loop                        (CRGB* leds,
                                                             uint16_t num_leds)             override;
    std::array<uint8_t, 3>      get_rgb                     ()                              override;

private:
    uint16_t                    z = 0;
    uint16_t                    noise_offsets[LED_STRIP_NUM_LEDS_MAX];

    static constexpr CRGB       palette[5] = {
        CRGB(184,   4,   0),
        CRGB(144,  44,   2),
        CRGB(  4,  96,   2),
        CRGB(  7,   7,  88),
        CRGB( 86,  94,  22)
    };
};