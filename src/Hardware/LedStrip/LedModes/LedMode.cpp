// File: LedMode.cpp
#include "LedMode.h"

// Static member definitions
std::array<uint8_t,3> LedMode::rgb = {0,0,0};
std::array<uint8_t,3> LedMode::hsv = {0,0,0};

LedMode::LedMode(LedStrip* controller)
    : led_controller(controller)
{}

// Set RGB and update HSV
void LedMode::set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    set_r(r);
    set_g(g);
    set_b(b);
    rgb_to_hsv();
}
void LedMode::set_r(uint8_t r) { rgb[0] = r; }
void LedMode::set_g(uint8_t g) { rgb[1] = g; }
void LedMode::set_b(uint8_t b) { rgb[2] = b; }

// Set HSV and update RGB
void LedMode::set_hsv(uint8_t hue, uint8_t saturation, uint8_t value) {
    set_hue(hue);
    set_sat(saturation);
    set_val(value);
    hsv_to_rgb();
}

void LedMode::set_hue(uint8_t hue)       { hsv[0] = hue; }
void LedMode::set_sat(uint8_t saturation){ hsv[1] = saturation; }
void LedMode::set_val(uint8_t value)     { hsv[2] = value; }

// Getters
std::array<uint8_t, 3> LedMode::get_rgb()   { return rgb; }
uint8_t  LedMode::get_r()     { return rgb[0]; }
uint8_t  LedMode::get_g()     { return rgb[1]; }
uint8_t  LedMode::get_b()     { return rgb[2]; }

std::array<uint8_t, 3> LedMode::get_hsv() { return hsv; }
uint8_t  LedMode::get_hue()   { return hsv[0]; }
uint8_t  LedMode::get_sat()   { return hsv[1]; }
uint8_t  LedMode::get_val()   { return hsv[2]; }

std::array<uint8_t, 3> LedMode::get_target_rgb() { return get_target_rgb(); }
uint8_t  LedMode::get_target_r()   { return get_target_r(); }
uint8_t  LedMode::get_target_g()   { return get_target_g(); }
uint8_t  LedMode::get_target_b()   { return get_target_b(); }

// Convert RGB → HSV
void LedMode::rgb_to_hsv() {
    // Debug: Print input RGB values
    DBG_PRINTLN(LedMode, "LedMode: rgb_to_hsv - Input RGB:");
    DBG_PRINTF(LedMode, "R = %d, G = %d, B = %d\n", rgb[0], rgb[1], rgb[2]);

    float r = rgb[0] / (float) 255;
    float g = rgb[1] / (float) 255;
    float b = rgb[2] / (float) 255;

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
    // hsv[1] = d / (qx + 1e-10); not used for this lib
    // hsv[2] = qx; not used for this lib

    hsv[0] = (hue_float * 255);
    hsv[1] = 255;
    hsv[2] = 255;

    // Debug: Print output HSV values
    DBG_PRINTLN(LedMode, "LedMode: rgb_to_hsv - Output HSV:");
    DBG_PRINTF(LedMode, "H = %d, S = %d, V = %d\n", (int)hsv[0], (int)hsv[1], (int)hsv[2]);
}

void LedMode::hsv_to_rgb() {
    // Debug: Print input HSV values
    DBG_PRINTLN(LedMode, "LedMode: hsv_to_rgb - Input HSV:");
    DBG_PRINTF(LedMode, "H = %d, S = %d, V = %d\n", (int)hsv[0], (int)hsv[1], (int)hsv[2]);

    float h_float = map(hsv[0], 0, 255, 0, 360);
    float s_float = map(hsv[1], 0, 255, 0, 100);
    float v_float = map(hsv[2], 0, 255, 0, 100);

    int i;
    float m, n, f;

    s_float /= 100;
    v_float /= 100;

    if (s_float == 0) {
        rgb[0] = rgb[1] = rgb[2] = round(v_float * 255);

        // Debug: Print output RGB values for grayscale
        DBG_PRINTLN(LedMode, "LedMode: hsv_to_rgb - Output RGB (Grayscale):");
        DBG_PRINTF(LedMode, "R = %d, G = %d, B = %d\n", rgb[0], rgb[1], rgb[2]);
        return;
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
            rgb[0] = round(v_float * 255);
            rgb[1] = round(n * 255);
            rgb[2] = round(m * 255);
            break;
        case 1:
            rgb[0] = round(n * 255);
            rgb[1] = round(v_float * 255);
            rgb[2] = round(m * 255);
            break;
        case 2:
            rgb[0] = round(m * 255);
            rgb[1] = round(v_float * 255);
            rgb[2] = round(n * 255);
            break;
        case 3:
            rgb[0] = round(m * 255);
            rgb[1] = round(n * 255);
            rgb[2] = round(v_float * 255);
            break;
        case 4:
            rgb[0] = round(n * 255);
            rgb[1] = round(m * 255);
            rgb[2] = round(v_float * 255);
            break;
        case 5:
            rgb[0] = round(v_float * 255);
            rgb[1] = round(m * 255);
            rgb[2] = round(n * 255);
            break;
    }

    // Debug: Print output RGB values after conversion
    DBG_PRINTLN(LedMode, "LedMode: hsv_to_rgb - Output RGB:");
    DBG_PRINTF(LedMode, "R = %d, G = %d, B = %d\n", rgb[0], rgb[1], rgb[2]);
}


float LedMode::fract(float x) { return x - int(x); }

float LedMode::mix(float a, float b, float t) { return a + (b - a) * t; }

float LedMode::step(float e, float x) { return x < e ? 0.0 : 1.0; }