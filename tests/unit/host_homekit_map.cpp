// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/tests/unit/host_homekit_map.cpp
//
// Host test of src/HomeKit/HomeKitMap.h (driven by test_homekit.py): led bytes <-> HomeKit units.
// Every expected value below is a literal produced by the xewe-led-os 2.3.0 HomeKit expressions
// (float arithmetic): led -> HomeKit std::round((b / 255.0f) * 360.0f) and * 100.0f; HomeKit -> led
// (uint8_t)std::round((x / 360.0f) * 255.0f) and (x / 100.0f) * 255.0f.
#include <cstdio>
#include <limits>

#include "../../src/HomeKit/HomeKitMap.h"

static int checks = 0, failures = 0;
#define CHECK(cond) \
    do { \
        ++checks; \
        if (!(cond)) { \
            ++failures; \
            std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); \
        } \
    } while (0)

// led byte -> HomeKit Hue degrees, for bytes 0..255
static const int HUE_DEG[256] = {0, 1, 3, 4, 6, 7, 8, 10, 11, 13, 14, 16, 17, 18, 20, 21, 23, 24,
    25, 27, 28, 30, 31, 32, 34, 35, 37, 38, 40, 41, 42, 44, 45, 47, 48, 49, 51, 52, 54, 55, 56, 58,
    59, 61, 62, 64, 65, 66, 68, 69, 71, 72, 73, 75, 76, 78, 79, 80, 82, 83, 85, 86, 88, 89, 90, 92,
    93, 95, 96, 97, 99, 100, 102, 103, 104, 106, 107, 109, 110, 112, 113, 114, 116, 117, 119, 120,
    121, 123, 124, 126, 127, 128, 130, 131, 133, 134, 136, 137, 138, 140, 141, 143, 144, 145, 147,
    148, 150, 151, 152, 154, 155, 157, 158, 160, 161, 162, 164, 165, 167, 168, 169, 171, 172, 174,
    175, 176, 178, 179, 181, 182, 184, 185, 186, 188, 189, 191, 192, 193, 195, 196, 198, 199, 200,
    202, 203, 205, 206, 208, 209, 210, 212, 213, 215, 216, 217, 219, 220, 222, 223, 224, 226, 227,
    229, 230, 232, 233, 234, 236, 237, 239, 240, 241, 243, 244, 246, 247, 248, 250, 251, 253, 254,
    256, 257, 258, 260, 261, 263, 264, 265, 267, 268, 270, 271, 272, 274, 275, 277, 278, 280, 281,
    282, 284, 285, 287, 288, 289, 291, 292, 294, 295, 296, 298, 299, 301, 302, 304, 305, 306, 308,
    309, 311, 312, 313, 315, 316, 318, 319, 320, 322, 323, 325, 326, 328, 329, 330, 332, 333, 335,
    336, 337, 339, 340, 342, 343, 344, 346, 347, 349, 350, 352, 353, 354, 356, 357, 359, 360};

// led byte -> HomeKit percent (Saturation and Brightness), for bytes 0..255
static const int PCT[256] = {0, 0, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 7, 7, 7, 8, 8, 9, 9,
    9, 10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 15, 16, 16, 16, 17, 17, 18, 18, 18,
    19, 19, 20, 20, 20, 21, 21, 22, 22, 22, 23, 23, 24, 24, 24, 25, 25, 25, 26, 26, 27, 27, 27, 28,
    28, 29, 29, 29, 30, 30, 31, 31, 31, 32, 32, 33, 33, 33, 34, 34, 35, 35, 35, 36, 36, 36, 37, 37,
    38, 38, 38, 39, 39, 40, 40, 40, 41, 41, 42, 42, 42, 43, 43, 44, 44, 44, 45, 45, 45, 46, 46, 47,
    47, 47, 48, 48, 49, 49, 49, 50, 50, 51, 51, 51, 52, 52, 53, 53, 53, 54, 54, 55, 55, 55, 56, 56,
    56, 57, 57, 58, 58, 58, 59, 59, 60, 60, 60, 61, 61, 62, 62, 62, 63, 63, 64, 64, 64, 65, 65, 65,
    66, 66, 67, 67, 67, 68, 68, 69, 69, 69, 70, 70, 71, 71, 71, 72, 72, 73, 73, 73, 74, 74, 75, 75,
    75, 76, 76, 76, 77, 77, 78, 78, 78, 79, 79, 80, 80, 80, 81, 81, 82, 82, 82, 83, 83, 84, 84, 84,
    85, 85, 85, 86, 86, 87, 87, 87, 88, 88, 89, 89, 89, 90, 90, 91, 91, 91, 92, 92, 93, 93, 93, 94,
    94, 95, 95, 95, 96, 96, 96, 97, 97, 98, 98, 98, 99, 99, 100, 100};

// HomeKit Hue 0..360 (whole degrees) -> led byte
static const int DEG_IN[361] = {0, 1, 1, 2, 3, 4, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 11, 12, 13, 13,
    14, 15, 16, 16, 17, 18, 18, 19, 20, 21, 21, 22, 23, 23, 24, 25, 26, 26, 27, 28, 28, 29, 30, 30,
    31, 32, 33, 33, 34, 35, 35, 36, 37, 38, 38, 39, 40, 40, 41, 42, 43, 43, 44, 45, 45, 46, 47, 47,
    48, 49, 50, 50, 51, 52, 52, 53, 54, 55, 55, 56, 57, 57, 58, 59, 60, 60, 61, 62, 62, 63, 64, 64,
    65, 66, 67, 67, 68, 69, 69, 70, 71, 72, 72, 73, 74, 74, 75, 76, 77, 77, 78, 79, 79, 80, 81, 81,
    82, 83, 84, 84, 85, 86, 86, 87, 88, 89, 89, 90, 91, 91, 92, 93, 94, 94, 95, 96, 96, 97, 98, 98,
    99, 100, 101, 101, 102, 103, 103, 104, 105, 106, 106, 107, 108, 108, 109, 110, 111, 111, 112,
    113, 113, 114, 115, 115, 116, 117, 118, 118, 119, 120, 120, 121, 122, 123, 123, 124, 125, 125,
    126, 127, 128, 128, 129, 130, 130, 131, 132, 132, 133, 134, 135, 135, 136, 137, 137, 138, 139,
    140, 140, 141, 142, 142, 143, 144, 145, 145, 146, 147, 147, 148, 149, 149, 150, 151, 152, 152,
    153, 154, 154, 155, 156, 157, 157, 158, 159, 159, 160, 161, 162, 162, 163, 164, 164, 165, 166,
    166, 167, 168, 169, 169, 170, 171, 171, 172, 173, 174, 174, 175, 176, 176, 177, 178, 179, 179,
    180, 181, 181, 182, 183, 183, 184, 185, 186, 186, 187, 188, 188, 189, 190, 191, 191, 192, 193,
    193, 194, 195, 196, 196, 197, 198, 198, 199, 200, 200, 201, 202, 203, 203, 204, 205, 205, 206,
    207, 208, 208, 209, 210, 210, 211, 212, 213, 213, 214, 215, 215, 216, 217, 217, 218, 219, 220,
    220, 221, 222, 222, 223, 224, 225, 225, 226, 227, 227, 228, 229, 230, 230, 231, 232, 232, 233,
    234, 234, 235, 236, 237, 237, 238, 239, 239, 240, 241, 242, 242, 243, 244, 244, 245, 246, 247,
    247, 248, 249, 249, 250, 251, 251, 252, 253, 254, 254, 255};

// HomeKit percent 0..100 (whole percent) -> led byte
static const int PCT_IN[101] = {0, 3, 5, 8, 10, 13, 15, 18, 20, 23, 26, 28, 31, 33, 36, 38, 41, 43,
    46, 48, 51, 54, 56, 59, 61, 64, 66, 69, 71, 74, 77, 79, 82, 84, 87, 89, 92, 94, 97, 99, 102,
    105, 107, 110, 112, 115, 117, 120, 122, 125, 128, 130, 133, 135, 138, 140, 143, 145, 148, 150,
    153, 156, 158, 161, 163, 166, 168, 171, 173, 176, 179, 181, 184, 186, 189, 191, 194, 196, 199,
    201, 204, 207, 209, 212, 214, 217, 219, 222, 224, 227, 230, 232, 235, 237, 240, 242, 245, 247,
    250, 252, 255};

// half-way inputs where double arithmetic would round one lower than 2.3.0's float arithmetic
struct HalfWay {
    float in;
    int out;
};
static const HalfWay HUE_HALF[] = {
    {195.5294f, 139},
    {199.7647f, 142},
    {219.5294f, 156},
    {223.7647f, 159},
    {243.5294f, 173},
    {247.7647f, 176},
    {256.2353f, 182},
    {280.2353f, 199},
    {283.0588f, 201},
    {304.2353f, 216},
    {307.0588f, 218},
    {328.2353f, 233},
    {331.0588f, 235},
    {335.2941f, 238},
    {352.2353f, 250},
    {355.0588f, 252},
    {359.2941f, 255},
};
static const HalfWay PCT_HALF[] = {
    {47.2549f, 121},
    {64.5098f, 165},
    {67.2549f, 172},
    {81.7647f, 209},
    {84.5098f, 216},
    {87.2549f, 223},
};

int main() {
    using namespace led_map;

    // led -> HomeKit, every byte
    for (int b = 0; b <= 255; ++b) {
        CHECK(hue_to_deg(static_cast<uint8_t>(b)) == HUE_DEG[b]);
        CHECK(sat_to_pct(static_cast<uint8_t>(b)) == PCT[b]);
        CHECK(bri_to_pct(static_cast<uint8_t>(b)) == PCT[b]);
    }
    CHECK(hue_to_deg(128) == 181); // 180.7
    CHECK(sat_to_pct(128) == 50);
    // brightness 0 is passed through: bytes 0 and 1 give 0 %, no clamp to HomeKit's minimum 1
    CHECK(bri_to_pct(0) == 0);
    CHECK(bri_to_pct(1) == 0);
    CHECK(bri_to_pct(2) == 1);
    CHECK(bri_to_pct(255) == 100);

    // HomeKit -> led, whole inputs
    for (int deg = 0; deg <= 360; ++deg)
        CHECK(deg_to_hue(static_cast<float>(deg)) == DEG_IN[deg]);
    for (int pct = 0; pct <= 100; ++pct)
        CHECK(pct_to_byte(static_cast<float>(pct)) == PCT_IN[pct]);
    CHECK(deg_to_hue(120.0f) == 85);
    CHECK(pct_to_byte(50.0f) == 128);
    CHECK(pct_to_byte(1.0f) == 3); // HomeKit's minimum brightness

    // HomeKit -> led, half-way inputs (float, as HomeSpan's getNewVal<float>() returns them)
    for (const HalfWay& h : HUE_HALF)
        CHECK(deg_to_hue(h.in) == h.out);
    for (const HalfWay& h : PCT_HALF)
        CHECK(pct_to_byte(h.in) == h.out);

    // out-of-range input (HomeSpan does not range-check writes) and NaN are clamped
    CHECK(deg_to_hue(-5.0f) == 0);
    CHECK(deg_to_hue(400.0f) == 255);
    CHECK(deg_to_hue(std::numeric_limits<float>::quiet_NaN()) == 0);
    CHECK(pct_to_byte(-1.0f) == 0);
    CHECK(pct_to_byte(250.0f) == 255);
    CHECK(pct_to_byte(std::numeric_limits<float>::quiet_NaN()) == 0);
    CHECK(clamp_byte(-3) == 0 && clamp_byte(300) == 255 && clamp_byte(7) == 7);

    // round trips stay within one step of the coarser scale
    for (int h = 0; h <= 255; ++h) {
        const int back = deg_to_hue(static_cast<float>(hue_to_deg(static_cast<uint8_t>(h))));
        CHECK(back - h <= 1 && h - back <= 1);
    }
    for (int p = 0; p <= 100; ++p)
        CHECK(sat_to_pct(pct_to_byte(static_cast<float>(p))) == p);
    for (int p = 1; p <= 100; ++p)
        CHECK(bri_to_pct(pct_to_byte(static_cast<float>(p))) == p);

    std::printf("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
