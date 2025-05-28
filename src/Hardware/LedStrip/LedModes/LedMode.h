// File: LedMode.h
#ifndef LEDMODE_H
#define LEDMODE_H

#include <array>
#include <cmath>
#include <algorithm>
#include "../../../Debug.h"
#include "../AsyncTimer/AsyncTimerArray.h"

class LedStrip;

class LedMode {
protected:
    // Shared color state
    static std::array<uint8_t,3> rgb;
    static std::array<uint8_t,3> hsv;

    LedStrip* led_strip;

    // Utility functions
    static float fract(float x);
    static float mix(float a, float b, float t);
    static float step(float e, float x);

    // Conversion routines
    static void rgb_to_hsv();
    static void hsv_to_rgb();
public:
    LedMode(LedStrip* led_strip);
    virtual ~LedMode();

    virtual void    frame()                         = 0;
    virtual bool    is_done()                       = 0;
    virtual uint8_t get_mode_id()                   = 0;
    virtual String  get_mode_name()                 = 0;

    virtual std::array<uint8_t, 3> get_target_rgb() = 0;
    virtual uint8_t get_target_r()                  = 0;
    virtual uint8_t get_target_g()                  = 0;
    virtual uint8_t get_target_b()                  = 0;

    // Setters
    static void set_rgb(uint8_t r, uint8_t g, uint8_t b);
    static void set_r(uint8_t r);
    static void set_g(uint8_t g);
    static void set_b(uint8_t b);
    static void set_hsv(uint8_t hue, uint8_t saturation, uint8_t value);
    static void set_hue(uint8_t hue);
    static void set_sat(uint8_t saturation);
    static void set_val(uint8_t value);

    // Getters
    static std::array<uint8_t, 3> get_rgb();
    static uint8_t  get_r();
    static uint8_t  get_g();
    static uint8_t  get_b();

    static std::array<uint8_t, 3> get_hsv();
    static uint8_t  get_hue();
    static uint8_t  get_sat();
    static uint8_t  get_val();
};

#endif  // LEDMODE_H
