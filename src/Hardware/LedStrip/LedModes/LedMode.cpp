// File: LedMode.cpp
#include "LedMode.h"

LedMode::LedMode(LedStrip* led_strip)
    : led_strip(led_strip),
      rgb({0,0,0}),
      hsv({0,0,0})
{}

LedMode::~LedMode() {}

void                    LedMode::set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    rgb = {r, g, b};
    hsv = rgb_to_hsv(rgb);
}

void                    LedMode::set_r(uint8_t r)   { set_rgb(r, rgb[1], rgb[2]); }
void                    LedMode::set_g(uint8_t g)   { set_rgb(rgb[0], g, rgb[2]); }
void                    LedMode::set_b(uint8_t b)   { set_rgb(rgb[0], rgb[1], b); }

void                    LedMode::set_hsv(uint8_t h, uint8_t s, uint8_t v) {
    hsv = {h, s, v};
    rgb = hsv_to_rgb(hsv);
}

void                    LedMode::set_h(uint8_t h)   { set_hsv(h, hsv[1], hsv[2]); }
void                    LedMode::set_s(uint8_t s)   { set_hsv(hsv[0], s, hsv[2]); }
void                    LedMode::set_v(uint8_t v)   { set_hsv(hsv[0], hsv[1], v); }

// Getters
std::array<uint8_t, 3>  LedMode::get_rgb()          { return rgb; }
uint8_t                 LedMode::get_r()            { return rgb[0]; }
uint8_t                 LedMode::get_g()            { return rgb[1]; }
uint8_t                 LedMode::get_b()            { return rgb[2]; }
std::array<uint8_t, 3>  LedMode::get_hsv()          { return hsv; }
uint8_t                 LedMode::get_h()            { return hsv[0]; }
uint8_t                 LedMode::get_s()            { return hsv[1]; }
uint8_t                 LedMode::get_v()            { return hsv[2]; }

// Convert RGB → HSV
std::array<uint8_t, 3> LedMode::rgb_to_hsv(std::array<uint8_t, 3> input_rgb) {
    // Debug: Print input RGB values
    DBG_PRINTLN(LedMode, "LedMode: rgb_to_hsv - Input RGB:");
    DBG_PRINTF(LedMode, "R = %d, G = %d, B = %d\n", input_rgb[0] ,input_rgb[1], input_rgb[2]);

    float r = input_rgb[0] / (float) 255;
    float g = input_rgb[1] / (float) 255;
    float b = input_rgb[2] / (float) 255;

    float s = step(b, g);
    float px = mix(b, g, s);
    float py = mix(g, b, s);
    float pz = mix(-1.0, 0.0, s);
    float pw = mix(0.6666666, -0.3333333, s);
    s = step(px, r);
    float qx = mix(px, r, s);
    float qz = mix(pw, pz, s);
    float qw = mix(r, px, s);
    float d = qx - min(qw, py);
    float hue_float = abs(qz + (qw - py) / (6.0 * d + 1e-10));
    float sat_float = d / (qx + 1e-10);
//     hsv[2] = qx; not used for this lib

    DBG_PRINTLN(LedMode, "LedMode: rgb_to_hsv - Output HSV:");
    DBG_PRINTF(LedMode, "H = %d, S = %d, V = %d\n", (int)(hue_float * 255), (int)(sat_float * 255), (int)(qx * 255));

    return {(hue_float * 255), (sat_float * 255), (qx * 255)};
//    hsv[0] = (hue_float * 255);
//    hsv[1] = 255;
//    hsv[2] = 255;

    // Debug: Print output HSV values
}

std::array<uint8_t, 3> LedMode::hsv_to_rgb(std::array<uint8_t, 3> input_hsv) {
    // Debug: Print input HSV values
    DBG_PRINTLN(LedMode, "LedMode: hsv_to_rgb - Input HSV:");
    DBG_PRINTF(LedMode, "H = %d, S = %d, V = %d\n", (int)input_hsv[0], (int)input_hsv[1], (int)input_hsv[2]);

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

        // Debug: Print output RGB values for grayscale
        DBG_PRINTLN(LedMode, "LedMode: hsv_to_rgb - Output rgb_temp (Grayscale):");
        DBG_PRINTF(LedMode, "R = %d, G = %d, B = %d\n", rgb_temp[0], rgb_temp[1], rgb_temp[2]);
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

    // Debug: Print output RGB values after conversion
    DBG_PRINTLN(LedMode, "LedMode: hsv_to_rgb - Output rgb_temp:");
    DBG_PRINTF(LedMode, "R = %d, G = %d, B = %d\n", rgb_temp[0], rgb_temp[1], rgb_temp[2]);
    return {rgb_temp[0], rgb_temp[1], rgb_temp[2]};
}


float LedMode::fract(float x) { return x - int(x); }

float LedMode::mix(float a, float b, float t) { return a + (b - a) * t; }

float LedMode::step(float e, float x) { return x < e ? 0.0 : 1.0; }