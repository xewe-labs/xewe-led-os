#pragma once

#include <cstdint>
#include <FastLED.h>

#include <Config.h>

namespace xewe::led {


enum class Chipset : std::uint8_t {
    APA104,
    APA106,
    GE8822,
    GS1903,
    GW6205,
    GW6205_400,
    LPD1886,
    LPD1886_8BIT,
    NEOPIXEL,
    PL9823,
    SK6812,
    SK6822,
    SM16703,
    SM16824E,
    TM1803,
    TM1804,
    TM1809,
    TM1812,
    TM1829,
    UCS1903,
    UCS1903B,
    UCS1904,
    UCS1912,
    UCS2903,
    WS2811,
    WS2811_400,
    WS2812,
    WS2812B,
    WS2813,
    WS2815,
    WS2816,
    WS2852,

    APA102,
    APA102HD,
    DOTSTAR,
    DOTSTARHD,
    HD107,
    HD107HD,
    LPD6803,
    LPD8806,
    P9813,
    SK9822,
    SK9822HD,
    SM16716,
    WS2801,
    WS2803,
};

struct ChipsetEntry {
    std::uint8_t id;
    Chipset value;
    const char* name;
};

inline constexpr ChipsetEntry kChipsetTable[] = {
    {0, Chipset::APA104, "APA104"},
    {1, Chipset::APA106, "APA106"},
    {2, Chipset::GE8822, "GE8822"},
    {3, Chipset::GS1903, "GS1903"},
    {4, Chipset::GW6205, "GW6205"},
    {5, Chipset::GW6205_400, "GW6205_400"},
    {6, Chipset::LPD1886, "LPD1886"},
    {7, Chipset::LPD1886_8BIT, "LPD1886_8BIT"},
    {8, Chipset::NEOPIXEL, "NEOPIXEL"},
    {9, Chipset::PL9823, "PL9823"},
    {10, Chipset::SK6812, "SK6812"},
    {11, Chipset::SK6822, "SK6822"},
    {12, Chipset::SM16703, "SM16703"},
    {13, Chipset::SM16824E, "SM16824E"},
    {14, Chipset::TM1803, "TM1803"},
    {15, Chipset::TM1804, "TM1804"},
    {16, Chipset::TM1809, "TM1809"},
    {17, Chipset::TM1812, "TM1812"},
    {18, Chipset::TM1829, "TM1829"},
    {19, Chipset::UCS1903, "UCS1903"},
    {20, Chipset::UCS1903B, "UCS1903B"},
    {21, Chipset::UCS1904, "UCS1904"},
    {22, Chipset::UCS1912, "UCS1912"},
    {23, Chipset::UCS2903, "UCS2903"},
    {24, Chipset::WS2811, "WS2811"},
    {25, Chipset::WS2811_400, "WS2811_400"},
    {26, Chipset::WS2812, "WS2812"},
    {27, Chipset::WS2812B, "WS2812B"},
    {28, Chipset::WS2813, "WS2813"},
    {29, Chipset::WS2815, "WS2815"},
    {30, Chipset::WS2816, "WS2816"},
    {31, Chipset::WS2852, "WS2852"},
    {32, Chipset::APA102, "APA102"},
    {33, Chipset::APA102HD, "APA102HD"},
    {34, Chipset::DOTSTAR, "DOTSTAR"},
    {35, Chipset::DOTSTARHD, "DOTSTARHD"},
    {36, Chipset::HD107, "HD107"},
    {37, Chipset::HD107HD, "HD107HD"},
    {38, Chipset::LPD6803, "LPD6803"},
    {39, Chipset::LPD8806, "LPD8806"},
    {40, Chipset::P9813, "P9813"},
    {41, Chipset::SK9822, "SK9822"},
    {42, Chipset::SK9822HD, "SK9822HD"},
    {43, Chipset::SM16716, "SM16716"},
    {44, Chipset::WS2801, "WS2801"},
    {45, Chipset::WS2803, "WS2803"},
};

inline void initFastLED_Generated(const Chipset chipset) {
    switch (chipset) {
        case Chipset::APA104:          FastLED.addLeds<APA104, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::APA106:          FastLED.addLeds<APA106, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::GE8822:          FastLED.addLeds<GE8822, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::GS1903:          FastLED.addLeds<GS1903, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::GW6205:          FastLED.addLeds<GW6205, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::GW6205_400:      FastLED.addLeds<GW6205_400, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::LPD1886:         FastLED.addLeds<LPD1886, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::LPD1886_8BIT:    FastLED.addLeds<LPD1886_8BIT, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::NEOPIXEL:        FastLED.addLeds<NEOPIXEL, LED_PIN_DATA>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::PL9823:          FastLED.addLeds<PL9823, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::SK6812:          FastLED.addLeds<SK6812, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::SK6822:          FastLED.addLeds<SK6822, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::SM16703:         FastLED.addLeds<SM16703, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::SM16824E:        FastLED.addLeds<SM16824E, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::TM1803:          FastLED.addLeds<TM1803, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::TM1804:          FastLED.addLeds<TM1804, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::TM1809:          FastLED.addLeds<TM1809, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::TM1812:          FastLED.addLeds<TM1812, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::TM1829:          FastLED.addLeds<TM1829, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::UCS1903:         FastLED.addLeds<UCS1903, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::UCS1903B:        FastLED.addLeds<UCS1903B, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::UCS1904:         FastLED.addLeds<UCS1904, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::UCS1912:         FastLED.addLeds<UCS1912, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::UCS2903:         FastLED.addLeds<UCS2903, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::WS2811:          FastLED.addLeds<WS2811, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::WS2811_400:      FastLED.addLeds<WS2811_400, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::WS2812:          FastLED.addLeds<WS2812, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::WS2812B:         FastLED.addLeds<WS2812B, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::WS2813:          FastLED.addLeds<WS2813, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::WS2815:          FastLED.addLeds<WS2815, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::WS2816:          FastLED.addLeds<WS2816, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::WS2852:          FastLED.addLeds<WS2852, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;

        case Chipset::APA102:          FastLED.addLeds<APA102, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::APA102HD:        FastLED.addLeds<APA102HD, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::DOTSTAR:         FastLED.addLeds<DOTSTAR, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::DOTSTARHD:       FastLED.addLeds<DOTSTARHD, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::HD107:           FastLED.addLeds<HD107, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::HD107HD:         FastLED.addLeds<HD107HD, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::LPD6803:         FastLED.addLeds<LPD6803, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::LPD8806:         FastLED.addLeds<LPD8806, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::P9813:           FastLED.addLeds<P9813, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::SK9822:          FastLED.addLeds<SK9822, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::SK9822HD:        FastLED.addLeds<SK9822HD, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::SM16716:         FastLED.addLeds<SM16716, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::WS2801:          FastLED.addLeds<WS2801, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;
        case Chipset::WS2803:          FastLED.addLeds<WS2803, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX); return;

        default:
            return;
    }
}

} // namespace xewe::led
