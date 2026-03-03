#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <FastLED.h>

using std::array;
using std::string;
using std::unique_ptr;
using std::vector;


#include "../AsyncTimer/AsyncTimer.h"

#include "Modes/Mode/Mode.h"
#include "Modes/Solid/Solid.h"
#include "Modes/Fade/Fade.h"
#include "Modes/FadeBrightness/FadeBrightness.h"
#include "Modes/Pulse/Pulse.h"
#include "Modes/Rainbow/Rainbow.h"

#define MODE_COUNT 5

class LedStrip;

class ModeController {
public:
    explicit                    ModeController              (LedStrip& led_strip,
                                                             uint16_t mode_transition_delay);

    CRGB* loop                  ();

    void                        set_rgb                     (const array<uint8_t, 3> new_rgb);
    array<uint8_t, 3>           get_rgb                     () const;

    void                        set_mode                    (const uint8_t new_mode);
    uint8_t                     get_mode_id                 () const;
    string                      get_mode_name               () const;
    string                      get_all_modes               () const;

    static std::array<uint8_t, 3> hsv_to_rgb                (const std::array<uint8_t, 3> hsv);
    static std::array<uint8_t, 3> rgb_to_hsv                (const std::array<uint8_t, 3> rgb);

private:
    // UPDATE: MakeFn now accepts num_leds
    using MakeFn = unique_ptr<Mode>(*)(uint16_t num_leds, const array<uint8_t, 3>&);

    struct ModeDesc {
        uint8_t                 id;
        const char* name;
        MakeFn                  make;
    };

    LedStrip&                   led_strip;

    std::array<ModeDesc, MODE_COUNT>     mode_registry;

    unique_ptr<Mode>            current_mode;
    unique_ptr<Mode>            old_mode;

    unique_ptr<AsyncTimer<uint16_t>> transition_timer;

    // UPDATE: Changed from fixed array to vector
    std::vector<CRGB>           frame;

    const ModeDesc* find_mode                   (uint8_t id) const;
    void                        begin_transition            (unique_ptr<Mode> next);
};