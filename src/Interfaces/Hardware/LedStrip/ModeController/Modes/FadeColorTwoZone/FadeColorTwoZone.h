#pragma once

#include "../Mode/Mode.h"
#include "../../ModeRegistry/ModeRegistry.h"
#include <vector>

class FadeColorTwoZone : public Mode {
public:
    explicit                    FadeColorTwoZone            (const std::map<std::string, uint16_t>& params);

    void                        loop                        (CRGB* leds,
                                                             uint16_t num_leds)             override;
    std::array<uint8_t, 3>      get_rgb                     ()                              override;

private:
    static constexpr uint8_t    MAX_BRIGHT                  = 255;

    void                        ensure_buffer               (uint16_t num_leds);
    uint16_t                    get_speed_step              () const;
    uint32_t                    get_noise_spatial_step      () const;
    uint8_t                     get_blend_amount            () const;
    CRGB                        get_weighted_color          (uint16_t val)                  const;
    CRGB                        scale_color                 (const CRGB& color,
                                                             uint8_t brightness)           const;
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