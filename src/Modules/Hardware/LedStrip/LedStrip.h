// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Hardware/LedStrip/LedStrip.h
#pragma once

#include <FastLED.h>
#include <array>
#include <memory>
#include <string>
#include <sstream>

#include "../../Module/SyncModule.h"
#include "../../../Utils/XeWeTimer.h"
#include "Brightness/Brightness.h"
#include "ModeController/ModeController.h"


struct LedStripConfig : public ModuleConfig {
    uint16_t                             num_led                     = LED_STRIP_NUM_LEDS_MAX;
    uint16_t                             mode_transition_delay       = 900;
    uint16_t                             brightness_transition_delay = 500;
    uint8_t                              frame_delay                 = 20; // 1000/20 = 50fps
    uint8_t                              fps_calc_window_s           = 3; // calculate fps number every 3 seconds
};

class LedStrip : public SyncModule {
public:
    explicit                             LedStrip                    (ModuleController& controller);

    // sync logic
    void                                 sync_color                  (const std::array<uint8_t, 3> color) override;
    void                                 sync_brightness             (const uint8_t brightness)           override;
    void                                 sync_state                  (const bool state)                   override;
    void                                 sync_mode                   (const uint8_t mode)                 override;
    void                                 sync_length                 (const uint16_t length)              override;
    //     void                sync_param                  (std::string key, uint8_t value);

    // module logic
    void                                 begin_routines_required     (const ModuleConfig& cfg)            override;
    void                                 begin_routines_init         (const ModuleConfig& cfg)            override;
    void                                 begin_routines_regular      (const ModuleConfig& cfg)            override;
    void                                 begin_routines_common       (const ModuleConfig& cfg)            override;

    void                                 loop                        ()                                   override;
    void                                 reset                       (const bool verbose      = false,
                                                                      const bool do_restart   = true,
                                                                      const bool keep_enabled = true)     override;
    std::string                          status                      (const bool verbose = false)         const override;

    // custom methods
    // color
    void                                 set_rgb                     (const std::array<uint8_t, 3> new_rgb);
    void                                 set_rgb                     (const uint8_t r,
                                                                      const uint8_t g,
                                                                      const uint8_t b);
    void                                 set_r                       (const uint8_t r);
    void                                 set_g                       (const uint8_t g);
    void                                 set_b                       (const uint8_t b);

    void                                 set_hsv                     (const std::array<uint8_t, 3> new_hsv);
    void                                 set_hsv                     (const uint8_t h,
                                                                      const uint8_t s,
                                                                      const uint8_t v);
    void                                 set_h                       (const uint8_t h);
    void                                 set_s                       (const uint8_t s);
    void                                 set_v                       (const uint8_t v);

    void                                 adj_rgb                     (const std::array<int, 3> rgb_delta);
    void                                 adj_rgb                     (const uint8_t r_delta,
                                                                      const uint8_t g_delta,
                                                                      const uint8_t b_delta);
    void                                 adj_r                       (const int r_delta);
    void                                 adj_g                       (const int g_delta);
    void                                 adj_b                       (const int b_delta);

    void                                 adj_hsv                     (const std::array<int, 3> hsv_delta);
    void                                 adj_hsv                     (const uint8_t h_delta,
                                                                      const uint8_t s_delta,
                                                                      const uint8_t v_delta);
    void                                 adj_h                       (const int h_delta);
    void                                 adj_s                       (const int s_delta);
    void                                 adj_v                       (const int v_delta);

    std::array<uint8_t, 3>               get_rgb                     ()                                   const;
    uint8_t                              get_r                       ()                                   const;
    uint8_t                              get_g                       ()                                   const;
    uint8_t                              get_b                       ()                                   const;
    std::array<uint8_t, 3>               get_hsv                     ()                                   const;
    uint8_t                              get_h                       ()                                   const;
    uint8_t                              get_s                       ()                                   const;
    uint8_t                              get_v                       ()                                   const;

    // brightness
    void                                 set_brightness              (const uint8_t new_brightness);
    void                                 adj_brightness              (const int brightness_delta);
    uint8_t                              get_brightness              ()                                   const;

    // state
    void                                 set_state                   (const bool state);
    void                                 toggle_state                ();
    void                                 turn_on                     ();
    void                                 turn_off                    ();
    bool                                 get_state                   ()                                   const;

    // mode
    void                                 set_mode                    (const uint8_t new_mode);
    void                                 adj_mode                    (const int mode_delta);

    void                                 set_mode_param              (std::string_view key,
                                                                      const uint16_t   value);
    void                                 adj_mode_param              (std::string_view key,
                                                                      const long       value_delta);

    uint8_t                              get_current_mode_id         ()                                   const;
    std::string_view                     get_current_mode_name       ()                                   const;
    uint16_t                             get_current_mode_param      (std::string_view key)               const;
    void                                 reset_current_mode          ();
    std::string                          get_all_modes_json          ()                                   const;

    // length
    void                                 set_length                  (const uint16_t length);
    uint16_t                             get_length                  ()                                   const;

    // color color
    void                                 set_color_order             (std::string_view order = "");

    // led lights
    void                                 set_pixel                   (uint16_t               i,
                                                                      std::array<uint8_t, 3> color_rgb);
    void                                 set_all                     (CRGB* new_leds);
    void                                 set_all                     (const uint8_t r,
                                                                      const uint8_t g,
                                                                      const uint8_t b);
    void                                 set_black                   ();

private:
    void                                 update_nvs_color_params     (const std::array<uint8_t, 3> new_color,
                                                                      bool is_rgb);

    CRGB                                 leds                        [LED_STRIP_NUM_LEDS_MAX];

    uint16_t                             num_led;
    uint8_t                              color_order_index           = 0;
    std::unique_ptr<AsyncTimer<uint8_t>> frame_timer;
    std::unique_ptr<AsyncTimer<uint8_t>> fps_timer;
    std::unique_ptr<ModeController>      mode_controller;
    std::unique_ptr<Brightness>          brightness;

    uint16_t                             fps_counter                 = 0;
    uint16_t                             fps_calculated              = 0;
    uint8_t                              fps_calc_window_s           = 1;

    enum class LEDChipset : std::uint8_t {
        APA102,
        APA102HD,
        APA104,
        APA106,
        DOTSTAR,
        DOTSTARHD,
        GE8822,
        GS1903,
        GW6205,
        GW6205_400KHZ,
        HD107,
        HD107HD,
        LPD1886,
        LPD1886_8BIT,
        LPD6803,
        LPD8806,
        NEOPIXEL,
        P9813,
        PL9823,
        SK6812,
        SK6822,
        SK9822,
        SK9822HD,
        SM16703,
        SM16716,
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
        WS2801,
        WS2803,
        WS2811,
        WS2811_400KHZ,
        WS2812,
        WS2812B,
        WS2813,
        WS2815,
        WS2816,
        WS2852,
    };

    struct LEDChipsetEntry {
        std::uint8_t                     id;
        LEDChipset                       value;
        const char*                      name;
    };

    inline static constexpr LEDChipsetEntry LED_CHIPSET_TABLE[] = {
        { 0, LEDChipset::APA102       , "APA102"},
        { 1, LEDChipset::APA102HD     , "APA102HD"},
        { 2, LEDChipset::APA104       , "APA104"},
        { 3, LEDChipset::APA106       , "APA106"},
        { 4, LEDChipset::DOTSTAR      , "DOTSTAR"},
        { 5, LEDChipset::DOTSTARHD    , "DOTSTARHD"},
        { 6, LEDChipset::GE8822       , "GE8822"},
        { 7, LEDChipset::GS1903       , "GS1903"},
        { 8, LEDChipset::GW6205       , "GW6205"},
        { 9, LEDChipset::GW6205_400KHZ, "GW6205_400KHZ"},
        {10, LEDChipset::HD107        , "HD107"},
        {11, LEDChipset::HD107HD      , "HD107HD"},
        {12, LEDChipset::LPD1886      , "LPD1886"},
        {13, LEDChipset::LPD1886_8BIT , "LPD1886_8BIT"},
        {14, LEDChipset::LPD6803      , "LPD6803"},
        {15, LEDChipset::LPD8806      , "LPD8806"},
        {16, LEDChipset::NEOPIXEL     , "NEOPIXEL"},
        {17, LEDChipset::P9813        , "P9813"},
        {18, LEDChipset::PL9823       , "PL9823"},
        {19, LEDChipset::SK6812       , "SK6812"},
        {20, LEDChipset::SK6822       , "SK6822"},
        {21, LEDChipset::SK9822       , "SK9822"},
        {22, LEDChipset::SK9822HD     , "SK9822HD"},
        {23, LEDChipset::SM16703      , "SM16703"},
        {24, LEDChipset::SM16716      , "SM16716"},
        {25, LEDChipset::SM16824E     , "SM16824E"},
        {26, LEDChipset::TM1803       , "TM1803"},
        {27, LEDChipset::TM1804       , "TM1804"},
        {28, LEDChipset::TM1809       , "TM1809"},
        {29, LEDChipset::TM1812       , "TM1812"},
        {30, LEDChipset::TM1829       , "TM1829"},
        {31, LEDChipset::UCS1903      , "UCS1903"},
        {32, LEDChipset::UCS1903B     , "UCS1903B"},
        {33, LEDChipset::UCS1904      , "UCS1904"},
        {34, LEDChipset::UCS1912      , "UCS1912"},
        {35, LEDChipset::UCS2903      , "UCS2903"},
        {36, LEDChipset::WS2801       , "WS2801"},
        {37, LEDChipset::WS2803       , "WS2803"},
        {38, LEDChipset::WS2811       , "WS2811"},
        {39, LEDChipset::WS2811_400KHZ, "WS2811_400KHZ"},
        {40, LEDChipset::WS2812       , "WS2812"},
        {41, LEDChipset::WS2812B      , "WS2812B"},
        {42, LEDChipset::WS2813       , "WS2813"},
        {43, LEDChipset::WS2815       , "WS2815"},
        {44, LEDChipset::WS2816       , "WS2816"},
        {45, LEDChipset::WS2852       , "WS2852"},
    };

    bool                                 set_leds_chipset            (const LEDChipset chipset);
};