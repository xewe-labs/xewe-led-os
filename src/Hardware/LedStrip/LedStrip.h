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
    // Add other mode IDs if they exist
};

class LedStrip {
private:
    uint16_t color_transition_delay = 900;
    uint8_t  led_controller_frame_delay = 10;
    uint16_t brightness_transition_delay = 500;

    CRGB* leds;
    uint16_t num_led = 0;

    std::unique_ptr<AsyncTimer<uint8_t>> frame_timer;
    std::unique_ptr<LedMode> led_mode;
    std::unique_ptr<Brightness> brightness; // Assumed to be an internally thread-safe class

    // Mutexes for thread safety
    SemaphoreHandle_t led_mode_mutex;
    SemaphoreHandle_t led_data_mutex;


public:
    explicit LedStrip(CRGB* leds_ptr);
    ~LedStrip(); // Modified to non-default for mutex deletion

    void frame();

    void set_mode(uint8_t new_mode);

    void set_rgb(uint8_t r, uint8_t g, uint8_t b);
    void set_r(uint8_t r);
    void set_g(uint8_t g);
    void set_b(uint8_t b);

    void set_hsv(uint8_t h, uint8_t s, uint8_t v);
    void set_h(uint8_t h);
    void set_s(uint8_t s);
    void set_v(uint8_t v);

    void set_brightness(uint8_t new_brightness);
    void set_state(uint8_t state);
    void turn_on();
    void turn_off();

    void fill_all(uint8_t r, uint8_t g, uint8_t b);
    // set_all_strips_pixel_color is internal helper, usually called when data_mutex is held
    void set_all_strips_pixel_color(uint16_t i, uint8_t r, uint8_t g, uint8_t b);

    void set_length(uint16_t length);

    std::array<uint8_t, 3> get_rgb() const;
    uint8_t get_r() const;
    uint8_t get_g() const;
    uint8_t get_b() const;

    std::array<uint8_t, 3> get_target_rgb() const;
    uint8_t get_target_r() const;
    uint8_t get_target_g() const;
    uint8_t get_target_b() const;

    std::array<uint8_t, 3> get_hsv() const;
    uint8_t get_h() const;
    uint8_t get_s() const;
    uint8_t get_v() const;

    std::array<uint8_t, 3> get_target_hsv() const;
    uint8_t get_target_h() const;
    uint8_t get_target_s() const;
    uint8_t get_target_v() const;

    uint8_t get_brightness() const; // Relies on Brightness class being thread-safe
    bool get_state() const;       // Relies on Brightness class being thread-safe

    // Disable copying
    LedStrip(const LedStrip&) = delete;
    LedStrip& operator=(const LedStrip&) = delete;
};

#endif // LEDSTRIP_H