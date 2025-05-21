// LedMode.h
#ifndef LEDMODE_H
#define LEDMODE_H

// Toggle all LedMode debug output on/off:
#define DEBUG_LEDMODE 1  // set to 0 to disable

#if DEBUG_LEDMODE
  #define DEBUG_PRINT(msg)    Serial.print(msg)
  #define DEBUG_PRINTLN(msg)    Serial.println(msg)
  #define DEBUG_PRINTF(fmt, ...) Serial.printf((fmt), ##__VA_ARGS__)
#else
  #define DEBUG_PRINT(msg)
  #define DEBUG_PRINTLN(msg)
  #define DEBUG_PRINTF(fmt, ...)
#endif

#include <cstdint>
#include "../AsyncTimer/AsyncTimerArray.h"

class LedController;

class LedMode {
protected:
    static uint8_t rgb[3];
    static uint8_t h, s, v;

    LedController* led_controller;
    static float fract(float x);
    static float mix(float a, float b, float t);
    static float step(float e, float x);
public:
    LedMode(LedController* controller);

    virtual void frame() = 0;
    virtual bool is_done() = 0;
    virtual uint8_t get_mode_id() = 0;

    static void set_rgb(uint8_t r, uint8_t g, uint8_t b);
    static void set_r(uint8_t r);
    static void set_g(uint8_t g);
    static void set_b(uint8_t b);

    static void set_hsv(uint8_t hue, uint8_t saturation, uint8_t value);
    static void set_hue(uint8_t hue);
    static void set_sat(uint8_t saturation);
    static void set_val(uint8_t value);

    static uint8_t* get_rgb();
    static uint8_t get_r();
    static uint8_t get_g();
    static uint8_t get_b();
    static uint8_t* get_hsv();
    static uint8_t get_hue();
    static uint8_t get_sat();
    static uint8_t get_val();

    static void rgb_to_hsv();
    static void hsv_to_rgb();
};

#endif  // LEDMODE_H
