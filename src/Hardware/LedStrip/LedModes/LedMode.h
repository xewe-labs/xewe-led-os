// File: LedMode.h
#ifndef LEDMODE_H
#define LEDMODE_H

#include <cstdint>
#include <array>
#include <algorithm>
#include <cmath>
#include "../../../Debug.h"
#include "../AsyncTimer/AsyncTimerArray.h"

class LedStrip;

class LedMode {
protected:
    // Shared color state
    static std::array<uint8_t,3> rgb;
    static uint8_t               h, s, v;

    LedStrip* led_controller;

    // Utility functions
    static float fract(float x);
    static float mix(float a, float b, float t);
    static float step(float e, float x);

public:
    LedMode(LedStrip* controller);
    virtual ~LedMode() = default;

    virtual void    frame()       = 0;
    virtual bool    is_done()     = 0;
    virtual uint8_t get_mode_id() = 0;

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
    static std::array<uint8_t, 3> get_rgb();    // pointer to rgb[3]
    static uint8_t  get_r();
    static uint8_t  get_g();
    static uint8_t  get_b();

    static std::array<uint8_t, 3> get_hsv();    // pointer to [h,s,v]
    static uint8_t  get_hue();
    static uint8_t  get_sat();
    static uint8_t  get_val();

    // Target getters (override if a mode stores its own data)
    virtual std::array<uint8_t, 3> get_target_rgb();
    virtual uint8_t  get_target_r();
    virtual uint8_t  get_target_g();
    virtual uint8_t  get_target_b();

    // Conversion routines
    static void rgb_to_hsv();
    static void hsv_to_rgb();
};

#endif  // LEDMODE_H
