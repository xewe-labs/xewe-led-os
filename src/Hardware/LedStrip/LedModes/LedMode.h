// File: LedMode.h
#ifndef LEDMODE_H
#define LEDMODE_H

#include <FastLED.h>
#include <array>
#include <cmath>
#include <algorithm>
#include "../../../Debug.h"
#include "../AsyncTimer/AsyncTimerArray.h"

class LedStrip;

class LedMode {
protected:
    // Current color state
    std::array<uint8_t,3> rgb;
    std::array<uint8_t,3> hsv;

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
    void set_rgb(uint8_t r, uint8_t g, uint8_t b);
    void set_r(uint8_t r);
    void set_g(uint8_t g);
    void set_b(uint8_t b);

    void set_hsv(uint8_t h, uint8_t s, uint8_t v);
    void set_h(uint8_t h);
    void set_s(uint8_t s);
    void set_v(uint8_t v);

    // Getters
    std::array<uint8_t, 3> get_rgb();
    uint8_t  get_r();
    uint8_t  get_g();
    uint8_t  get_b();
    virtual std::array<uint8_t, 3> get_target_rgb() = 0;
    virtual uint8_t get_target_r()                  = 0;
    virtual uint8_t get_target_g()                  = 0;
    virtual uint8_t get_target_b()                  = 0;

    std::array<uint8_t, 3> get_hsv();
    uint8_t  get_h();
    uint8_t  get_s();
    uint8_t  get_v();
    virtual std::array<uint8_t, 3> get_target_hsv() = 0;
    virtual uint8_t get_target_h()                  = 0;
    virtual uint8_t get_target_s()                  = 0;
    virtual uint8_t get_target_v()                  = 0;

    virtual uint8_t get_mode_id()                   = 0;
    virtual String  get_mode_name()                 = 0;
};

#endif  // LEDMODE_H
