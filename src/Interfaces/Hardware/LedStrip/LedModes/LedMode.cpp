// File: LedMode.cpp
#include "LedMode.h"

// Assuming Debug.h is available and provides DBG_PRINTF/DBG_PRINTLN macros.

LedMode::LedMode(LedStrip* led_strip)
    : led_strip(led_strip),
      rgb({0,0,0}),
      hsv({0,0,0})
{
    DBG_PRINTF(LedMode, "-> LedMode::LedMode(led_strip: %p)\n", (void*)led_strip);
    DBG_PRINTLN(LedMode, "<- LedMode::LedMode()");
}

LedMode::~LedMode() {
    DBG_PRINTLN(LedMode, "-> LedMode::~LedMode()");
    DBG_PRINTLN(LedMode, "<- LedMode::~LedMode()");
}

void LedMode::set_rgb(std::array<uint8_t, 3> new_rgb) {
    DBG_PRINTF(LedMode, "-> LedMode::set_rgb(rgb: {%u, %u, %u})\n", new_rgb[0], new_rgb[1], new_rgb[2]);
    this->rgb = new_rgb; // Store the new rgb value first
    this->hsv = rgb_to_hsv(new_rgb);
    DBG_PRINTLN(LedMode, "<- LedMode::set_rgb()");
}

void LedMode::set_r(uint8_t r) {
    DBG_PRINTF(LedMode, "-> LedMode::set_r(r: %u)\n", r);
    set_rgb({r, this->rgb[1], this->rgb[2]});
    DBG_PRINTLN(LedMode, "<- LedMode::set_r()");
}

void LedMode::set_g(uint8_t g) {
    DBG_PRINTF(LedMode, "-> LedMode::set_g(g: %u)\n", g);
    set_rgb({this->rgb[0], g, this->rgb[2]});
    DBG_PRINTLN(LedMode, "<- LedMode::set_g()");
}

void LedMode::set_b(uint8_t b) {
    DBG_PRINTF(LedMode, "-> LedMode::set_b(b: %u)\n", b);
    set_rgb({this->rgb[0], this->rgb[1], b});
    DBG_PRINTLN(LedMode, "<- LedMode::set_b()");
}

void LedMode::set_hsv(std::array<uint8_t, 3> new_hsv) {
    DBG_PRINTF(LedMode, "-> LedMode::set_hsv(hsv: {%u, %u, %u})\n", new_hsv[0], new_hsv[1], new_hsv[2]);
    this->hsv = new_hsv; // Store the new hsv value first
    this->rgb = hsv_to_rgb(new_hsv);
    DBG_PRINTLN(LedMode, "<- LedMode::set_hsv()");
}

void LedMode::set_h(uint8_t h) {
    DBG_PRINTF(LedMode, "-> LedMode::set_h(h: %u)\n", h);
    set_hsv({h, this->hsv[1], this->hsv[2]});
    DBG_PRINTLN(LedMode, "<- LedMode::set_h()");
}

void LedMode::set_s(uint8_t s) {
    DBG_PRINTF(LedMode, "-> LedMode::set_s(s: %u)\n", s);
    set_hsv({this->hsv[0], s, this->hsv[2]});
    DBG_PRINTLN(LedMode, "<- LedMode::set_s()");
}

void LedMode::set_v(uint8_t v) {
    DBG_PRINTF(LedMode, "-> LedMode::set_v(v: %u)\n", v);
    set_hsv({this->hsv[0], this->hsv[1], v});
    DBG_PRINTLN(LedMode, "<- LedMode::set_v()");
}

// Getters
std::array<uint8_t, 3> LedMode::get_rgb() {
    DBG_PRINTLN(LedMode, "-> LedMode::get_rgb()");
    DBG_PRINTF(LedMode, "<- LedMode::get_rgb() returns: {%u, %u, %u}\n", rgb[0], rgb[1], rgb[2]);
    return rgb;
}

uint8_t LedMode::get_r() {
    DBG_PRINTLN(LedMode, "-> LedMode::get_r()");
    DBG_PRINTF(LedMode, "<- LedMode::get_r() returns: %u\n", rgb[0]);
    return rgb[0];
}

uint8_t LedMode::get_g() {
    DBG_PRINTLN(LedMode, "-> LedMode::get_g()");
    DBG_PRINTF(LedMode, "<- LedMode::get_g() returns: %u\n", rgb[1]);
    return rgb[1];
}

uint8_t LedMode::get_b() {
    DBG_PRINTLN(LedMode, "-> LedMode::get_b()");
    DBG_PRINTF(LedMode, "<- LedMode::get_b() returns: %u\n", rgb[2]);
    return rgb[2];
}

std::array<uint8_t, 3> LedMode::get_hsv() {
    DBG_PRINTLN(LedMode, "-> LedMode::get_hsv()");
    DBG_PRINTF(LedMode, "<- LedMode::get_hsv() returns: {%u, %u, %u}\n", hsv[0], hsv[1], hsv[2]);
    return hsv;
}

uint8_t LedMode::get_h() {
    DBG_PRINTLN(LedMode, "-> LedMode::get_h()");
    DBG_PRINTF(LedMode, "<- LedMode::get_h() returns: %u\n", hsv[0]);
    return hsv[0];
}

uint8_t LedMode::get_s() {
    DBG_PRINTLN(LedMode, "-> LedMode::get_s()");
    DBG_PRINTF(LedMode, "<- LedMode::get_s() returns: %u\n", hsv[1]);
    return hsv[1];
}

uint8_t LedMode::get_v() {
    DBG_PRINTLN(LedMode, "-> LedMode::get_v()");
    DBG_PRINTF(LedMode, "<- LedMode::get_v() returns: %u\n", hsv[2]);
    return hsv[2];
}
