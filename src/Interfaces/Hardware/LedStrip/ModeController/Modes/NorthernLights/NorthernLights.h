#pragma once

#include "../Mode/Mode.h"
#include "../../ModeRegistry/ModeRegistry.h"
#include <vector>

class NorthernLights : public Mode {
public:
    explicit                    NorthernLights              (const std::map<std::string, uint16_t>& params);

    void                        loop                        (CRGB* leds,
                                                             uint16_t num_leds)             override;
    std::array<uint8_t, 3>      get_rgb                     ()                              override;

private:
    static constexpr uint16_t   HUE_A_LOW                   = 16000;
    static constexpr uint16_t   HUE_A_HIGH                  = 25000;
    static constexpr uint16_t   HUE_B_LOW                   = 61000;
    static constexpr uint16_t   HUE_B_HIGH                  = 65535;

    static constexpr uint16_t   NOISE_SPATIAL_STEP          = 8000;
    static constexpr uint8_t    MIN_BRIGHT                  = 10;
    static constexpr uint8_t    MAX_BRIGHT                  = 255;
    static constexpr uint8_t    MIN_SAT                     = 240;
    static constexpr uint8_t    MAX_SAT                     = 255;
    static constexpr uint8_t    BLEND_AMOUNT                = 35;

    void                        ensure_buffer               (uint16_t num_leds);
    uint16_t                    get_fire_step               () const;
    CRGB                        get_weighted_color          (uint16_t val)                  const;
    CRGB                        ColorHSV                    (uint16_t hue,
                                                             uint8_t sat,
                                                             uint8_t val)                  const;
    CRGB                        blend_colors                (const CRGB& color1,
                                                             const CRGB& color2,
                                                             uint8_t amount)               const;

    uint32_t                    counter;
    CRGB                        base_rgb;
    std::vector<CRGB>           previous_frame;
};