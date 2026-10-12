// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/tests/unit/host_alexa_map.cpp
// Host test of src/Alexa/AlexaMap.h (driven by test_alexa.py): led hsv bytes -> Espalexa colour.
#include <cstdio>

#include "../../src/Alexa/AlexaMap.h"

static int checks = 0, failures = 0;
#define CHECK(cond) \
    do { \
        ++checks; \
        if (!(cond)) { \
            ++failures; \
            std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); \
        } \
    } while (0)

int main() {
    using namespace led_map;
    CHECK(hue8_to_hue16(0) == 0);
    CHECK(hue8_to_hue16(255) == 65535); // full scale maps to full scale
    CHECK(hue8_to_hue16(1) == 0x0101);
    CHECK(hue8_to_hue16(128) == 0x8080);
    for (int h = 0; h < 255; ++h)
        CHECK(hue8_to_hue16(static_cast<uint8_t>(h)) < hue8_to_hue16(static_cast<uint8_t>(h + 1)));
    for (int h = 0; h <= 255; ++h)
        CHECK((hue8_to_hue16(static_cast<uint8_t>(h)) >> 8) == h); // high byte is the led hue
    CHECK(sat8_to_alexa(255) == 254); // Espalexa's saturation range ends at 254 (2.3.x)
    CHECK(sat8_to_alexa(254) == 254);
    CHECK(sat8_to_alexa(0) == 0);
    CHECK(sat8_to_alexa(100) == 100);
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
