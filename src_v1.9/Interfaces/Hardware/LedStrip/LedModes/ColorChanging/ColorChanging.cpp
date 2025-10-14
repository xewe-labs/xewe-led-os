// File: ColorChanging.cpp
#include "ColorChanging.h"

ColorChanging::ColorChanging(LedStrip* controller,
                             uint8_t current_r, uint8_t current_g, uint8_t current_b,
                             uint8_t t0, uint8_t t1, uint8_t t2,
                             char mode, uint32_t duration_ms)
  : LedMode(controller),
    timer(nullptr)
{
    DBG_PRINTF(ColorChanging, "-> ColorChanging::ColorChanging(controller: %p, current_rgb: {%u, %u, %u}, target_vals: {%u, %u, %u}, mode: %c, duration: %lu)\n",
               (void*)controller, current_r, current_g, current_b, t0, t1, t2, mode, duration_ms);

    std::array<uint8_t, 3> start_rgb{current_r, current_g, current_b};
    std::array<uint8_t, 3> target_rgb;

    if (mode == 'r') { // Target is provided in RGB
        target_rgb = {t0, t1, t2};
    } else { // Target is provided in HSV, convert it to RGB
        target_rgb = LedMode::hsv_to_rgb({t0, t1, t2});
    }

    timer = std::make_unique<AsyncTimerArray>(duration_ms, start_rgb, target_rgb);
    timer->initiate();

    // Set the initial color of the transition immediately
    loop();

    DBG_PRINTLN(ColorChanging, "<- ColorChanging::ColorChanging()");
}

void ColorChanging::loop() {
    // DBG_PRINTLN(ColorChanging, "-> ColorChanging::loop()");
    std::array<uint8_t,3> current_color = timer->get_current_value();
    // DBG_PRINTF(ColorChanging, "   current_color: {%u, %u, %u}\n", current_color[0], current_color[1], current_color[2]);
    set_rgb(current_color);
    // DBG_PRINTLN(ColorChanging, "<- ColorChanging::loop()");
}

bool ColorChanging::is_done() {
    DBG_PRINTLN(ColorChanging, "-> ColorChanging::is_done()");
    bool result = timer->is_done();
    DBG_PRINTF(ColorChanging, "<- ColorChanging::is_done() returns: %s\n", result ? "true" : "false");
    return result;
}

uint8_t ColorChanging::get_mode_id() {
    DBG_PRINTLN(ColorChanging, "-> ColorChanging::get_mode_id()");
    uint8_t result = 1;
    DBG_PRINTF(ColorChanging, "<- ColorChanging::get_mode_id() returns: %u\n", result);
    return result;
}

String ColorChanging::get_mode_name() {
    DBG_PRINTLN(ColorChanging, "-> ColorChanging::get_mode_name()");
    String result = "Color Changing";
    DBG_PRINTF(ColorChanging, "<- ColorChanging::get_mode_name() returns: %s\n", result.c_str());
    return result;
}

uint8_t ColorChanging::get_target_mode_id() {
    DBG_PRINTLN(ColorChanging, "-> ColorChanging::get_target_mode_id()");
    uint8_t result = 0;
    DBG_PRINTF(ColorChanging, "<- ColorChanging::get_target_mode_id() returns: %u\n", result);
    return result;
}

String ColorChanging::get_target_mode_name() {
    DBG_PRINTLN(ColorChanging, "-> ColorChanging::get_target_mode_name()");
    String result = "Color Solid";
    DBG_PRINTF(ColorChanging, "<- ColorChanging::get_target_mode_name() returns: %s\n", result.c_str());
    return result;
}

std::array<uint8_t, 3> ColorChanging::get_target_rgb() {
    DBG_PRINTLN(ColorChanging, "-> ColorChanging::get_target_rgb()");
    std::array<uint8_t, 3> result = timer->get_target_value();
    DBG_PRINTF(ColorChanging, "<- ColorChanging::get_target_rgb() returns: {%u, %u, %u}\n", result[0], result[1], result[2]);
    return result;
}

uint8_t ColorChanging::get_target_r() {
    DBG_PRINTLN(ColorChanging, "-> ColorChanging::get_target_r()");
    uint8_t result = get_target_rgb()[0];
    DBG_PRINTF(ColorChanging, "<- ColorChanging::get_target_r() returns: %u\n", result);
    return result;
}

uint8_t ColorChanging::get_target_g() {
    DBG_PRINTLN(ColorChanging, "-> ColorChanging::get_target_g()");
    uint8_t result = get_target_rgb()[1];
    DBG_PRINTF(ColorChanging, "<- ColorChanging::get_target_g() returns: %u\n", result);
    return result;
}

uint8_t ColorChanging::get_target_b() {
    DBG_PRINTLN(ColorChanging, "-> ColorChanging::get_target_b()");
    uint8_t result = get_target_rgb()[2];
    DBG_PRINTF(ColorChanging, "<- ColorChanging::get_target_b() returns: %u\n", result);
    return result;
}

std::array<uint8_t, 3> ColorChanging::get_target_hsv() {
    DBG_PRINTLN(ColorChanging, "-> ColorChanging::get_target_hsv()");
    std::array<uint8_t, 3> result = LedMode::rgb_to_hsv(timer->get_target_value());
    DBG_PRINTF(ColorChanging, "<- ColorChanging::get_target_hsv() returns: {%u, %u, %u}\n", result[0], result[1], result[2]);
    return result;
}

uint8_t ColorChanging::get_target_h() {
    DBG_PRINTLN(ColorChanging, "-> ColorChanging::get_target_h()");
    uint8_t result = get_target_hsv()[0];
    DBG_PRINTF(ColorChanging, "<- ColorChanging::get_target_h() returns: %u\n", result);
    return result;
}

uint8_t ColorChanging::get_target_s() {
    DBG_PRINTLN(ColorChanging, "-> ColorChanging::get_target_s()");
    uint8_t result = get_target_hsv()[1];
    DBG_PRINTF(ColorChanging, "<- ColorChanging::get_target_s() returns: %u\n", result);
    return result;
}

uint8_t ColorChanging::get_target_v() {
    DBG_PRINTLN(ColorChanging, "-> ColorChanging::get_target_v()");
    uint8_t result = get_target_hsv()[2];
    DBG_PRINTF(ColorChanging, "<- ColorChanging::get_target_v() returns: %u\n", result);
    return result;
}