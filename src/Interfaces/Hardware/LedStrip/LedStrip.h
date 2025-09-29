// src/Interfaces/Hardware/LedStrip/LedStrip.h
#pragma once


#include "../../Interface/Interface.h"
#include "../../../Debug.h"
#include "../../../Config.h"
#include <FastLED.h>
#include <memory>
#include <array>
#include <string>
#include <sstream>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "AsyncTimer/AsyncTimer.h"
#include "Brightness/Brightness.h"
#include "LedModes/LedMode.h"
#include "LedModes/ColorSolid/ColorSolid.h"
#include "LedModes/ColorChanging/ColorChanging.h"


enum LedModeID : uint8_t {
    MODE_SOLID = 0,
    MODE_CHANGING = 1,
};


struct LedStripConfig : public ModuleConfig {
    uint16_t                    num_led                     = LED_STRIP_NUM_LEDS_MAX;
    uint16_t                    color_transition_delay      = 900;
    uint8_t                     led_controller_frame_delay  = 10;
    uint16_t                    brightness_transition_delay = 500;
};


class LedStrip : public Interface {
public:
    explicit                    LedStrip                    (SystemController& controller);
                                ~LedStrip                   ();

    // required implementation
    void                        begin                       (const ModuleConfig& cfg)       override;
    void                        loop                        ()                              override;
    void                        reset                       (bool verbose=false)            override;

    void                        sync_color                  (std::array<uint8_t,3> color)   override;
    void                        sync_brightness             (uint8_t brightness)            override;
    void                        sync_state                  (uint8_t state)                 override;
    void                        sync_mode                   (uint8_t mode)                  override;
    void                        sync_length                 (uint16_t length)               override;
    void                        sync_all                    (std::array<uint8_t,3> color,
                                                             uint8_t brightness,
                                                             uint8_t state,
                                                             uint8_t mode,
                                                             uint16_t length)               override;
    // optional implementation
    bool                        init_setup                  (bool verbose=false,
                                                             bool enable_prompt=true,
                                                             bool reboot_after=false)       override;
    // bool                        enable                      (bool verbose=false)            override;
    // bool                        disable                     (bool verbose=false)            override;
     std::string                 status                     (bool verbose=false) const      override;
    // bool                        is_enabled                  (bool verbose=false) const      override;
    // bool                        is_disabled                 (bool verbose=false) const      override;

    // other methods
    void                        set_mode                    (uint8_t new_mode);

    void                        set_rgb                     (std::array<uint8_t, 3> new_rgb);
    void                        set_r                       (uint8_t r);
    void                        set_g                       (uint8_t g);
    void                        set_b                       (uint8_t b);

    void                        set_hsv                     (std::array<uint8_t, 3> new_hsv);
    void                        set_h                       (uint8_t h);
    void                        set_s                       (uint8_t s);
    void                        set_v                       (uint8_t v);

    void                        set_brightness              (uint8_t new_brightness);
    void                        set_state                   (uint8_t state);
    void                        toggle_state                ();
    void                        turn_on                     ();
    void                        turn_off                    ();

    void                        fill_all                    (std::array<uint8_t, 3> color_rgb);
    void                        set_pixel                   (uint16_t i, std::array<uint8_t, 3> color_rgb);
    void                        set_length                  (uint16_t length);
    uint16_t                    get_length                  () const;

    std::array<uint8_t, 3>      get_rgb                     () const;
    uint8_t                     get_r                       () const;
    uint8_t                     get_g                       () const;
    uint8_t                     get_b                       () const;

    std::array<uint8_t, 3>      get_target_rgb              () const;
    uint8_t                     get_target_r                () const;
    uint8_t                     get_target_g                () const;
    uint8_t                     get_target_b                () const;

    std::array<uint8_t, 3>      get_hsv                     () const;
    uint8_t                     get_h                       () const;
    uint8_t                     get_s                       () const;
    uint8_t                     get_v                       () const;

    std::array<uint8_t, 3>      get_target_hsv              () const;
    uint8_t                     get_target_h                () const;
    uint8_t                     get_target_s                () const;
    uint8_t                     get_target_v                () const;

    uint8_t                     get_brightness              () const;
    bool                        get_state                   () const;
    String                      get_mode_name               () const;
    uint8_t                     get_mode_id                 () const;

    uint8_t                     get_target_brightness       () const;
    bool                        get_target_state            () const;
    uint8_t                     get_target_mode_id          () const;
    String                      get_target_mode_name        () const;

private:
    CRGB                        leds                        [LED_STRIP_NUM_LEDS_MAX];
    uint16_t                    num_led                     = LED_STRIP_NUM_LEDS_MAX;
    uint16_t                    color_transition_delay      = 900;
    uint8_t                     led_controller_frame_delay  = 10;
    uint16_t                    brightness_transition_delay = 500;

    SemaphoreHandle_t           led_mode_mutex;
    SemaphoreHandle_t           led_data_mutex;
    // CLI callbacks
    void                        set_rgb_cli                 (std::string_view args);
    void                        set_r_cli                   (std::string_view args);
    void                        set_g_cli                   (std::string_view args);
    void                        set_b_cli                   (std::string_view args);
    void                        set_hsv_cli                 (std::string_view args);
    void                        set_hue_cli                 (std::string_view args);
    void                        set_sat_cli                 (std::string_view args);
    void                        set_val_cli                 (std::string_view args);
    void                        set_brightness_cli          (std::string_view args);
    void                        set_state_cli               (std::string_view args);
    void                        toggle_state_cli            ();
    void                        turn_on_cli                 ();
    void                        turn_off_cli                ();
    void                        set_mode_cli                (std::string_view args);
    void                        set_length_cli              (std::string_view args);

    std::unique_ptr             <AsyncTimer<uint8_t>>       frame_timer;
    std::unique_ptr             <LedMode>                   led_mode;
    std::unique_ptr             <Brightness>                brightness;

    uint32_t                    fps_counter                         =1;
};