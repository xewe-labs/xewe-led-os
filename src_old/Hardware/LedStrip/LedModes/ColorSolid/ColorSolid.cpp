// File: ColorSolid.cpp
#include "ColorSolid.h"
// Assuming Debug.h is available and provides DBG_PRINTF/DBG_PRINTLN macros.

ColorSolid::ColorSolid(LedStrip* led_strip, uint8_t r, uint8_t g, uint8_t b)
    : LedMode(led_strip), id_done_flag(false)
{
    DBG_PRINTF(ColorSolid, "-> ColorSolid::ColorSolid(led_strip: %p, r: %u, g: %u, b: %u)\n", (void*)led_strip, r, g, b);
    set_rgb({r, g, b});
    DBG_PRINTLN(ColorSolid, "<- ColorSolid::ColorSolid()");
}

void ColorSolid::loop() {
    // This function is empty, but we can log its call.
//    DBG_PRINTLN(ColorSolid, "-> ColorSolid::loop()");
//    DBG_PRINTLN(ColorSolid, "<- ColorSolid::loop()");
}

bool ColorSolid::is_done() {
    DBG_PRINTLN(ColorSolid, "-> ColorSolid::is_done()");
    DBG_PRINTF(ColorSolid, "<- ColorSolid::is_done() returns: %s\n", id_done_flag ? "true" : "false");
    return id_done_flag;
}

uint8_t ColorSolid::get_mode_id() {
    DBG_PRINTLN(ColorSolid, "-> ColorSolid::get_mode_id()");
    uint8_t result = 0;
    DBG_PRINTF(ColorSolid, "<- ColorSolid::get_mode_id() returns: %u\n", result);
    return result;
}

String ColorSolid::get_mode_name() {
    DBG_PRINTLN(ColorSolid, "-> ColorSolid::get_mode_name()");
    String result = "Color Solid";
    DBG_PRINTF(ColorSolid, "<- ColorSolid::get_mode_name() returns: %s\n", result.c_str());
    return result;
}

uint8_t ColorSolid::get_target_mode_id() {
    DBG_PRINTLN(ColorChanging, "-> ColorChanging::get_target_mode_id()");
    DBG_PRINTLN(ColorChanging, "<- ColorChanging::get_target_mode_id()");
    return get_mode_id();
}

String ColorSolid::get_target_mode_name() {
    DBG_PRINTLN(ColorChanging, "-> ColorChanging::get_target_mode_name()");
    DBG_PRINTLN(ColorChanging, "<- ColorChanging::get_target_mode_name()");
    return get_mode_name();
}

std::array<uint8_t, 3> ColorSolid::get_target_rgb() {
    DBG_PRINTLN(ColorSolid, "-> ColorSolid::get_target_rgb()");
    std::array<uint8_t, 3> result = get_rgb();
    DBG_PRINTF(ColorSolid, "<- ColorSolid::get_target_rgb() returns: {%u, %u, %u}\n", result[0], result[1], result[2]);
    return result;
}

uint8_t ColorSolid::get_target_r() {
    DBG_PRINTLN(ColorSolid, "-> ColorSolid::get_target_r()");
    uint8_t result = get_r();
    DBG_PRINTF(ColorSolid, "<- ColorSolid::get_target_r() returns: %u\n", result);
    return result;
}

uint8_t ColorSolid::get_target_g() {
    DBG_PRINTLN(ColorSolid, "-> ColorSolid::get_target_g()");
    uint8_t result = get_g();
    DBG_PRINTF(ColorSolid, "<- ColorSolid::get_target_g() returns: %u\n", result);
    return result;
}

uint8_t ColorSolid::get_target_b() {
    DBG_PRINTLN(ColorSolid, "-> ColorSolid::get_target_b()");
    uint8_t result = get_b();
    DBG_PRINTF(ColorSolid, "<- ColorSolid::get_target_b() returns: %u\n", result);
    return result;
}

std::array<uint8_t, 3> ColorSolid::get_target_hsv() {
    DBG_PRINTLN(ColorSolid, "-> ColorSolid::get_target_hsv()");
    std::array<uint8_t, 3> result = get_hsv();
    DBG_PRINTF(ColorSolid, "<- ColorSolid::get_target_hsv() returns: {%u, %u, %u}\n", result[0], result[1], result[2]);
    return result;
}

uint8_t ColorSolid::get_target_h() {
    DBG_PRINTLN(ColorSolid, "-> ColorSolid::get_target_h()");
    uint8_t result = get_h();
    DBG_PRINTF(ColorSolid, "<- ColorSolid::get_target_h() returns: %u\n", result);
    return result;
}

uint8_t ColorSolid::get_target_s() {
    DBG_PRINTLN(ColorSolid, "-> ColorSolid::get_target_s()");
    uint8_t result = get_s();
    DBG_PRINTF(ColorSolid, "<- ColorSolid::get_target_s() returns: %u\n", result);
    return result;
}

uint8_t ColorSolid::get_target_v() {
    DBG_PRINTLN(ColorSolid, "-> ColorSolid::get_target_v()");
    uint8_t result = get_v();
    DBG_PRINTF(ColorSolid, "<- ColorSolid::get_target_v() returns: %u\n", result);
    return result;
}