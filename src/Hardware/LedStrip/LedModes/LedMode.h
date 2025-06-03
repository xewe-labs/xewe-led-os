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
    // Current color state
    static std::array<uint8_t,3> rgb;
    static std::array<uint8_t,3> hsv;

    LedStrip* led_strip;

    // Conversion routines
    static std::array<uint8_t, 3> rgb_to_hsv(std::array<uint8_t, 3> input_rgb);
    static std::array<uint8_t, 3> hsv_to_rgb(std::array<uint8_t, 3> input_hsv);
public:
    LedMode(LedStrip* led_strip);
    virtual ~LedMode();

    virtual void    frame()                         = 0;
    virtual bool    is_done()                       = 0;

    // Setters
    static void set_rgb(uint8_t r, uint8_t g, uint8_t b);
    static void set_r(uint8_t r);
    static void set_g(uint8_t g);
    static void set_b(uint8_t b);

    static void set_hsv(uint8_t h, uint8_t s, uint8_t v);
    static void set_h(uint8_t h);
    static void set_s(uint8_t s);
    static void set_v(uint8_t v);

    // Getters
    static std::array<uint8_t, 3> get_rgb();
    static uint8_t  get_r();
    static uint8_t  get_g();
    static uint8_t  get_b();
    virtual std::array<uint8_t, 3> get_target_rgb() = 0;
    virtual uint8_t get_target_r()                  = 0;
    virtual uint8_t get_target_g()                  = 0;
    virtual uint8_t get_target_b()                  = 0;

    static std::array<uint8_t, 3> get_hsv();
    static uint8_t  get_h();
    static uint8_t  get_s();
    static uint8_t  get_v();
    virtual std::array<uint8_t, 3> get_target_hsv() = 0;
    virtual uint8_t get_target_h()                  = 0;
    virtual uint8_t get_target_s()                  = 0;
    virtual uint8_t get_target_v()                  = 0;

    virtual uint8_t get_mode_id()                   = 0;
    virtual String  get_mode_name()                 = 0;
};

#endif  // LEDMODE_H
