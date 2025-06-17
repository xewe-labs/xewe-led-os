#ifndef LEDSTRIP_H
#define LEDSTRIP_H

#include <FastLED.h>
#include <memory>
#include <array>
#include "../../Debug.h" // Assuming this path is correct for Debug.h
#include "AsyncTimer/AsyncTimer.h" // Assuming this path is correct
#include "Brightness/Brightness.h" // Assuming this path is correct and Brightness is now thread-safe
#include "LedModes/LedMode.h"      // Assuming this path is correct
#include "LedModes/ColorSolid/ColorSolid.h" // Assuming this path is correct
#include "LedModes/ColorChanging/ColorChanging.h" // Assuming this path is correct

// Required for FreeRTOS mutexes
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

enum LedModeID : uint8_t {
    MODE_SOLID = 0,
    MODE_CHANGING = 1,
};

class LedStrip {
private:
    uint16_t                                                color_transition_delay = 900;
    uint8_t                                                 led_controller_frame_delay = 10;
    uint16_t                                                brightness_transition_delay = 500;

    CRGB*                                                   leds;
    uint16_t                                                num_led = 0;

    std::unique_ptr<AsyncTimer<uint8_t>>                    frame_timer;
    std::unique_ptr<LedMode>                                led_mode;
    std::unique_ptr<Brightness>                             brightness;

    SemaphoreHandle_t                                       led_mode_mutex;
    SemaphoreHandle_t                                       led_data_mutex;

public:
    explicit                LedStrip                        ();
                            ~LedStrip                       ();
    void                    begin                           (CRGB* leds_ptr, uint16_t count);

    void                    loop                            ();

    void                    set_mode                        (uint8_t new_mode);

    void                    set_rgb                         (std::array<uint8_t, 3> new_rgb);
    void                    set_r                           (uint8_t r);
    void                    set_g                           (uint8_t g);
    void                    set_b                           (uint8_t b);

    void                    set_hsv                         (std::array<uint8_t, 3> new_hsv);
    void                    set_h                           (uint8_t h);
    void                    set_s                           (uint8_t s);
    void                    set_v                           (uint8_t v);

    void                    set_brightness                  (uint8_t new_brightness);
    void                    set_state                       (uint8_t state);
    void                    turn_on                         ();
    void                    turn_off                        ();

    void                    fill_all                        (std::array<uint8_t, 3> color_rgb);
    void                    set_pixel                       (uint16_t i, std::array<uint8_t, 3> color_rgb);
    void                    set_length                      (uint16_t length);
    uint16_t                get_length                      () const;

    std::array<uint8_t, 3>  get_rgb                         () const;
    uint8_t                 get_r                           () const;
    uint8_t                 get_g                           () const;
    uint8_t                 get_b                           () const;

    std::array<uint8_t, 3>  get_target_rgb                  () const;
    uint8_t                 get_target_r                    () const;
    uint8_t                 get_target_g                    () const;
    uint8_t                 get_target_b                    () const;

    std::array<uint8_t, 3>  get_hsv                         () const;
    uint8_t                 get_h                           () const;
    uint8_t                 get_s                           () const;
    uint8_t                 get_v                           () const;

    std::array<uint8_t, 3>  get_target_hsv                  () const;
    uint8_t                 get_target_h                    () const;
    uint8_t                 get_target_s                    () const;
    uint8_t                 get_target_v                    () const;

    uint8_t                 get_brightness                  () const;
    bool                    get_state                       () const;
    String                  get_mode_name                   () const;
    uint8_t                 get_mode_id                     () const;

    uint8_t                 get_target_brightness           () const;
    bool                    get_target_state                () const;
    uint8_t                 get_target_mode_id              () const;
    String                  get_target_mode_name            () const;

    // Disable copying
    LedStrip                                                (const LedStrip&) = delete;
    LedStrip&               operator=                       (const LedStrip&) = delete;
};

#endif // LEDSTRIP_H