// File: LedMode.cpp
#include "LedMode.h"

std::array<uint8_t,3> LedMode::rgb = {0,0,0};
std::array<uint8_t,3> LedMode::hsv = {0,0,0};

LedMode::LedMode(LedStrip* led_strip)
    : led_strip(led_strip)
{}

void                    LedMode::set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    set_r(r);
    set_g(g);
    set_b(b);
    hsv = rgb_to_hsv({r, g, b});
}

void                    LedMode::set_r(uint8_t r)   { set_rgb(r, rgb[1], rgb[2]) }
void                    LedMode::set_g(uint8_t g)   { set_rgb(rgb[0], g, rgb[2]) }
void                    LedMode::set_b(uint8_t b)   { set_rgb(rgb[0], rgb[1], b) }

void                    LedMode::set_hsv(uint8_t h, uint8_t s, uint8_t v) {
    set_hue(h);
    set_sat(s);
    set_val(v);
    rgb = hsv_to_rgb({h, s, v});
}

void                    LedMode::set_h(uint8_t h)   { set_hsv(h, hsv[1], hsv[2]) }
void                    LedMode::set_s(uint8_t s)   { set_hsv(hsv[0], s, hsv[2]) }
void                    LedMode::set_v(uint8_t v)   { set_hsv(hsv[0], hsv[1], v) }

// Getters
std::array<uint8_t, 3>  LedMode::get_rgb()          { return rgb; }
uint8_t                 LedMode::get_r()            { return rgb[0]; }
uint8_t                 LedMode::get_g()            { return rgb[1]; }
uint8_t                 LedMode::get_b()            { return rgb[2]; }
std::array<uint8_t, 3>  LedMode::get_hsv()          { return hsv; }
uint8_t                 LedMode::get_hue()          { return hsv[0]; }
uint8_t                 LedMode::get_sat()          { return hsv[1]; }
uint8_t                 LedMode::get_val()          { return hsv[2]; }

// Convert RGB → HSV
static std::array<uint8_t, 3> rgb_to_hsv(std::array<uint8_t, 3> input_rgb) {
    DBG_PRINTLN(LedMode, "LedMode: rgb_to_hsv - Input RGB:");
    DBG_PRINTF(LedMode, "R = %d, G = %d, B = %d\n", input_rgb[0], input_rgb[1], input_rgb[2]);

    CRGB fastled_rgb(input_rgb[0], input_rgb[1], input_rgb[2]);
    CHSV fastled_hsv = fastled_rgb; // This is the magic!
    uint8_t h_uint8 = fastled_hsv.h;
    uint8_t s_uint8 = fastled_hsv.s;
    uint8_t v_uint8 = fastled_hsv.v;

    DBG_PRINTLN(LedMode, "LedMode: rgb_to_hsv - Output HSV (FastLED 0-255):");
    DBG_PRINTF(LedMode, "H = %d, S = %d, V = %d\n", h_uint8, s_uint8, v_uint8);

    return {h_uint8, s_uint8, v_uint8};
}


static std::array<uint8_t, 3> hsv_to_rgb(std::array<uint8_t, 3> input_hsv) {
    DBG_PRINTLN(LedMode, "LedMode: hsv_to_rgb - Input HSV (FastLED 0-255):");
    DBG_PRINTF(LedMode, "H = %d, S = %d, V = %d\n", input_hsv[0], input_hsv[1], input_hsv[2]);

    CHSV fastled_hsv(input_hsv[0], input_hsv[1], input_hsv[2]);
    CRGB fastled_rgb = fastled_hsv; // This is the magic!

    uint8_t r_uint8 = fastled_rgb.r;
    uint8_t g_uint8 = fastled_rgb.g;
    uint8_t b_uint8 = fastled_rgb.b;

    DBG_PRINTLN(LedMode, "LedMode: hsv_to_rgb - Output RGB:");
    DBG_PRINTF(LedMode, "R = %d, G = %d, B = %d\n", r_uint8, g_uint8, b_uint8);

    return {r_uint8, g_uint8, b_uint8};
}
