// File: LedStrip.h
#ifndef LEDSTRIP_H
#define LEDSTRIP_H

#include <FastLED.h>
#include <array>
#include <cstdint>
#include "AsyncTimer/AsyncTimer.h"
#include "Brightness/Brightness.h"
#include "LedModes/ColorSolid/ColorSolid.h"
#include "LedModes/ColorChanging/ColorChanging.h"

class LedStrip {
private:
    CRGB*                        leds;                   // Pointer to FastLED LED array
    uint16_t                     num_led;                // Number of LEDs in the strip
    AsyncTimer<uint8_t>*         frame_timer;            // Timer to manage frame updates
    LedMode*                     led_mode;
    Brightness*                  brightness;

    uint16_t                     color_transition_delay    = 900;
    uint8_t                      led_controller_frame_delay = 10;
    uint16_t                     brightness_transition_delay = 500;

public:
    // Constructor: pass pointer to FastLED array and initial settings
    LedStrip(CRGB* leds_ptr,
             uint16_t init_length,
             uint8_t init_r,
             uint8_t init_g,
             uint8_t init_b,
             uint8_t init_brightness,
             uint8_t init_state,
             uint8_t init_mode);

    ~LedStrip();

    // Main frame update loop
    void frame();

    // Mode and color control
    void set_mode(uint8_t new_mode);
    void set_rgb(uint8_t r, uint8_t g, uint8_t b);
    void set_r(uint8_t r);
    void set_g(uint8_t g);
    void set_b(uint8_t b);
    void set_hsv(uint8_t hue, uint8_t saturation, uint8_t value);
    void set_hue(uint8_t hue);
    void set_sat(uint8_t saturation);
    void set_val(uint8_t value);

    // Brightness and power control
    void set_brightness(uint8_t new_brightness);
    void set_state(uint8_t state);
    void turn_on();
    void turn_off();

    // Direct pixel operations
    void fill_all(uint8_t r, uint8_t g, uint8_t b);
    void set_all_strips_pixel_color(uint16_t i, uint8_t r, uint8_t g, uint8_t b);

    // Runtime configuration
    void set_length(uint16_t length);

    // Get current color values
    std::array<uint8_t, 3>  get_rgb() const;
    uint8_t                 get_r()   const;
    uint8_t                 get_g()   const;
    uint8_t                 get_b()   const;
    std::array<uint8_t, 3>  get_target_rgb() const;
    uint8_t                 get_target_r()   const;
    uint8_t                 get_target_g()   const;
    uint8_t                 get_target_b()   const;
    uint8_t                 get_brightness()  const;
    bool                    get_state()       const;
};

#endif  // LEDSTRIP_H
