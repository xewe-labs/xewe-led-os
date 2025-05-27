#ifndef BRIGHTNESS_H
#define BRIGHTNESS_H

#include <cstdint>
#include <FastLED.h>
#include "../AsyncTimer/AsyncTimer.h"
#include "../../../Debug.h"

class Brightness {
public:
    Brightness(uint16_t transition_delay,
               uint8_t initial_brightness,
               uint8_t state);

    // Called each loop/frame to advance the timer
    void frame();

    // Smoothly transition from current to new_brightness
    void set_brightness(uint8_t new_brightness);

    // Query whether a brightness transition is in progress
    bool is_changing();

    // Timer accessors
    uint8_t get_current_value() const;
    uint8_t get_target_value() const;
    uint8_t get_start_value() const;
    uint8_t get_dimmed_color(uint8_t color) const;
    uint8_t get_last_brightness() const;


    // Turn fully on (from off) or off (saving last)
    void turn_on();
    void turn_off();

    bool get_state() const;

    int max(int a, int b) const { return a >= b ? a : b; }
private:
    AsyncTimer<uint8_t>*         timer;
    bool                         state;
    uint8_t                      last_brightness;
};

// Free‐function (as defined in your .cc)

#endif // BRIGHTNESS_H
