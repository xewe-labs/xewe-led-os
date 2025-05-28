#ifndef LEDSTRIP_H
#define LEDSTRIP_H

#include <FastLED.h>
#include <memory>
#include <array>
#include "AsyncTimer/AsyncTimer.h"
#include "Brightness/Brightness.h"
#include "LedModes/LedMode.h"
#include "LedModes/ColorSolid/ColorSolid.h"
#include "LedModes/ColorChanging/ColorChanging.h"

enum LedModeID : uint8_t {
    MODE_SOLID = 0,
    MODE_CHANGING = 1,
};

class LedStrip {
private:
    CRGB* leds;
    uint16_t num_led;

    std::unique_ptr<AsyncTimer<uint8_t>> frame_timer;
    std::unique_ptr<LedMode> led_mode;
    std::unique_ptr<Brightness> brightness;

    uint16_t color_transition_delay = 900;
    uint8_t  led_controller_frame_delay = 10;
    uint16_t brightness_transition_delay = 500;

public:
    explicit LedStrip(CRGB* leds_ptr);
    ~LedStrip() = default;

    void frame();

    void set_mode(uint8_t new_mode);

    void set_rgb(uint8_t r, uint8_t g, uint8_t b);
    void set_r(uint8_t r);
    void set_g(uint8_t g);
    void set_b(uint8_t b);

    void set_hsv(uint8_t hue, uint8_t saturation, uint8_t value);
    void set_hue(uint8_t hue);
    void set_sat(uint8_t saturation);
    void set_val(uint8_t value);

    void set_brightness(uint8_t new_brightness);
    void set_state(uint8_t state);
    void turn_on();
    void turn_off();

    void fill_all(uint8_t r, uint8_t g, uint8_t b);
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

    uint8_t get_brightness() const;
    bool get_state() const;

    // Disable copying
    LedStrip(const LedStrip&) = delete;
    LedStrip& operator=(const LedStrip&) = delete;
};

#endif
