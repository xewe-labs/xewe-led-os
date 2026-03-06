/*********************************************************************************
 * SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 * Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 * See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 * Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 * https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Interfaces/Hardware/LedStrip/LedStrip.h

#pragma once

#include "../../Interface/Interface.h"

#include "AsyncTimer/AsyncTimer.h"
#include "Brightness/Brightness.h"
#include "ModeController/ModeController.h"

#include <FastLED.h>
#include <array>
#include <memory>
#include <string>

struct LedStripConfig : public ModuleConfig {
    uint16_t                    num_led                     = LED_STRIP_NUM_LEDS_MAX;
    uint16_t                    mode_transition_delay       = 900;
    uint16_t                    brightness_transition_delay = 500;
    uint8_t                     frame_delay                 = 20; // 1000/20 = 50fps max
};


class LedStrip : public Interface {
public:
    explicit                    LedStrip                    (SystemController& controller);

    // interface sync
    void                        sync_color                  (const array<uint8_t,3> color)  override;
    void                        sync_brightness             (const uint8_t brightness)      override;
    void                        sync_state                  (const uint8_t state)           override;
    void                        sync_mode                   (const uint8_t mode)            override;
    void                        sync_length                 (const uint16_t length)         override;

    // module logic
    void                        begin_routines_required     (const ModuleConfig& cfg)       override;
    void                        begin_routines_init         (const ModuleConfig& cfg)       override;
    void                        begin_routines_regular      (const ModuleConfig& cfg)       override;
    void                        begin_routines_common       (const ModuleConfig& cfg)       override;

    void                        loop                        ()                              override;
    void                        reset                       (const bool verbose=false,
                                                             const bool do_restart=true,
                                                             const bool keep_enabled=true)  override;
    string                      status                      (const bool verbose=false)      const override;

    // custom methods
    // color
    void                        set_rgb                     (const array<uint8_t, 3> new_rgb);
    void                        set_r                       (const uint8_t r);
    void                        set_g                       (const uint8_t g);
    void                        set_b                       (const uint8_t b);
    void                        set_hsv                     (const array<uint8_t, 3> new_hsv);
    void                        set_h                       (const uint8_t h);
    void                        set_s                       (const uint8_t s);
    void                        set_v                       (const uint8_t v);

    array<uint8_t, 3>           get_rgb                     () const;
    uint8_t                     get_r                       () const;
    uint8_t                     get_g                       () const;
    uint8_t                     get_b                       () const;

    array<uint8_t, 3>           get_hsv                     () const;
    uint8_t                     get_h                       () const;
    uint8_t                     get_s                       () const;
    uint8_t                     get_v                       () const;

    // brightness
    void                        set_brightness              (const uint8_t new_brightness);
    uint8_t                     get_brightness              () const;

    // state
    void                        set_state                   (const uint8_t state);
    void                        toggle_state                ();
    void                        turn_on                     ();
    void                        turn_off                    ();
    bool                        get_state                   () const;

    // mode
    void                        set_mode                    (const uint8_t new_mode);
    void                        set_mode_param              (string_view key, const uint16_t value);

    uint8_t                     get_current_mode_id         () const;
    string_view                 get_current_mode_name       () const;
    uint16_t                    get_current_mode_param      (string_view key) const;

    std::string                 get_all_modes_json          () const;

    // length
    void                        set_length                  (const uint16_t length);
    uint16_t                    get_length                  () const;

    // led lights
    void                        set_pixel                   (uint16_t i, array<uint8_t, 3> color_rgb);
    void                        set_all                     (CRGB* new_leds);
    void                        set_all                     (const uint8_t r, const uint8_t g, const uint8_t b);
    void                        set_black                   ();

private:
    void                        update_nvs_color_params     (const array<uint8_t, 3> new_color, bool is_rgb);

    CRGB                        leds                        [LED_STRIP_NUM_LEDS_MAX];

    uint16_t                    num_led;

    unique_ptr                  <AsyncTimer<uint8_t>>       frame_timer;
    unique_ptr                  <ModeController>            mode_controller;
    unique_ptr                  <Brightness>                brightness;

    uint32_t                    fps_counter                 =1;
};