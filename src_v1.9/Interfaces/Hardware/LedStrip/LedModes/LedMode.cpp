// File: LedMode.cpp
#include "LedMode.h"

// Assuming Debug.h is available and provides DBG_PRINTF/DBG_PRINTLN macros.

LedMode::LedMode(LedStrip* led_strip)
    : led_strip(led_strip),
      rgb({0,0,0})
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
    this->rgb = new_rgb;
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
    this->rgb = LedMode::hsv_to_rgb(new_hsv);
    DBG_PRINTLN(LedMode, "<- LedMode::set_hsv()");
}

void LedMode::set_h(uint8_t h) {
    DBG_PRINTF(LedMode, "-> LedMode::set_h(h: %u)\n", h);
    std::array<uint8_t, 3> current_hsv = get_hsv();
    set_hsv({h, current_hsv[1], current_hsv[2]});
    DBG_PRINTLN(LedMode, "<- LedMode::set_h()");
}

void LedMode::set_s(uint8_t s) {
    DBG_PRINTF(LedMode, "-> LedMode::set_s(s: %u)\n", s);
    std::array<uint8_t, 3> current_hsv = get_hsv();
    set_hsv({current_hsv[0], s, current_hsv[2]});
    DBG_PRINTLN(LedMode, "<- LedMode::set_s()");
}

void LedMode::set_v(uint8_t v) {
    DBG_PRINTF(LedMode, "-> LedMode::set_v(v: %u)\n", v);
    std::array<uint8_t, 3> current_hsv = get_hsv();
    set_hsv({current_hsv[0], current_hsv[1], v});
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
    std::array<uint8_t, 3> hsv = LedMode::rgb_to_hsv(this->rgb);
    DBG_PRINTF(LedMode, "<- LedMode::get_hsv() returns: {%u, %u, %u}\n", hsv[0], hsv[1], hsv[2]);
    return hsv;
}

uint8_t LedMode::get_h() {
    DBG_PRINTLN(LedMode, "-> LedMode::get_h()");
    uint8_t h = LedMode::rgb_to_hsv(this->rgb)[0];
    DBG_PRINTF(LedMode, "<- LedMode::get_h() returns: %u\n", h);
    return h;
}

uint8_t LedMode::get_s() {
    DBG_PRINTLN(LedMode, "-> LedMode::get_s()");
    uint8_t s = LedMode::rgb_to_hsv(this->rgb)[1];
    DBG_PRINTF(LedMode, "<- LedMode::get_s() returns: %u\n", s);
    return s;
}

uint8_t LedMode::get_v() {
    DBG_PRINTLN(LedMode, "-> LedMode::get_v()");
    uint8_t v = LedMode::rgb_to_hsv(this->rgb)[2];
    DBG_PRINTF(LedMode, "<- LedMode::get_v() returns: %u\n", v);
    return v;
}

// Convert RGB → HSV
std::array<uint8_t, 3> LedMode::rgb_to_hsv(std::array<uint8_t, 3> input_rgb) {
    DBG_PRINTF(LedMode, "-> LedMode::rgb_to_hsv(input_rgb: {%u, %u, %u})\n", input_rgb[0], input_rgb[1], input_rgb[2]);

    float r = input_rgb[0] / 255.0f;
    float g = input_rgb[1] / 255.0f;
    float b = input_rgb[2] / 255.0f;

    float s = step(b, g);
    float px = mix(b, g, s);
    float py = mix(g, b, s);
    float pz = mix(-1.0f, 0.0f, s);
    float pw = mix(0.6666666f, -0.3333333f, s);
    s = step(px, r);
    float qx = mix(px, r, s);
    float qz = mix(pw, pz, s);
    float qw = mix(r, px, s);
    float d = qx - min(qw, py);
    float hue_float = abs(qz + (qw - py) / (6.0f * d + 1e-10f));
    float sat_float = d / (qx + 1e-10f);
    // hsv[2] = qx; not used for this lib

    std::array<uint8_t, 3> output_hsv = {(uint8_t)(hue_float * 255), (uint8_t)(sat_float * 255), (uint8_t)(qx * 255)};

    DBG_PRINTF(LedMode, "<- LedMode::rgb_to_hsv() returns: {%u, %u, %u}\n", output_hsv[0], output_hsv[1], output_hsv[2]);

    return output_hsv;
}

std::array<uint8_t, 3> LedMode::hsv_to_rgb(std::array<uint8_t, 3> input_hsv) {
    DBG_PRINTF(LedMode, "-> LedMode::hsv_to_rgb(input_hsv: {%u, %u, %u})\n", input_hsv[0], input_hsv[1], input_hsv[2]);

    float h_float = map(input_hsv[0], 0, 255, 0, 360);
    float s_float = map(input_hsv[1], 0, 255, 0, 100);
    float v_float = map(input_hsv[2], 0, 255, 0, 100);

    int i;
    float m, n, f;
    std::array<uint8_t, 3> rgb_temp = {0, 0, 0};
    s_float /= 100;
    v_float /= 100;

    if (s_float == 0) {
        rgb_temp[0] = rgb_temp[1] = rgb_temp[2] = round(v_float * 255);
        DBG_PRINTF(LedMode, "<- LedMode::hsv_to_rgb() returns (grayscale): {%u, %u, %u}\n", rgb_temp[0], rgb_temp[1], rgb_temp[2]);
        return {rgb_temp[0], rgb_temp[1], rgb_temp[2]};
    }

    h_float /= 60;
    i = floor(h_float);
    f = h_float - i;

    if (!(i & 1)) {
        f = 1 - f;
    }

    m = v_float * (1 - s_float);
    n = v_float * (1 - s_float * f);

    switch (i) {
        case 0:
        case 6:
            rgb_temp[0] = round(v_float * 255);
            rgb_temp[1] = round(n * 255);
            rgb_temp[2] = round(m * 255);
            break;
        case 1:
            rgb_temp[0] = round(n * 255);
            rgb_temp[1] = round(v_float * 255);
            rgb_temp[2] = round(m * 255);
            break;
        case 2:
            rgb_temp[0] = round(m * 255);
            rgb_temp[1] = round(v_float * 255);
            rgb_temp[2] = round(n * 255);
            break;
        case 3:
            rgb_temp[0] = round(m * 255);
            rgb_temp[1] = round(n * 255);
            rgb_temp[2] = round(v_float * 255);
            break;
        case 4:
            rgb_temp[0] = round(n * 255);
            rgb_temp[1] = round(m * 255);
            rgb_temp[2] = round(v_float * 255);
            break;
        case 5:
            rgb_temp[0] = round(v_float * 255);
            rgb_temp[1] = round(m * 255);
            rgb_temp[2] = round(n * 255);
            break;
    }

    DBG_PRINTF(LedMode, "<- LedMode::hsv_to_rgb() returns: {%u, %u, %u}\n", rgb_temp[0], rgb_temp[1], rgb_temp[2]);
    return {rgb_temp[0], rgb_temp[1], rgb_temp[2]};
}

float LedMode::fract(float x) {
//    DBG_PRINTF(LedMode, "-> LedMode::fract(x: %f)\n", x);
    float result = x - int(x);
//    DBG_PRINTF(LedMode, "<- LedMode::fract() returns: %f\n", result);
    return result;
}

float LedMode::mix(float a, float b, float t) {
//    DBG_PRINTF(LedMode, "-> LedMode::mix(a: %f, b: %f, t: %f)\n", a, b, t);
    float result = a + (b - a) * t;
//    DBG_PRINTF(LedMode, "<- LedMode::mix() returns: %f\n", result);
    return result;
}

float LedMode::step(float e, float x) {
//    DBG_PRINTF(LedMode, "-> LedMode::step(e: %f, x: %f)\n", e, x);
    float result = x < e ? 0.0 : 1.0;
//    DBG_PRINTF(LedMode, "<- LedMode::step() returns: %f\n", result);
    return result;
}