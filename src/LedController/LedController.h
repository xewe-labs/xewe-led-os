#ifndef LEDCONTROLLER_H
#define LEDCONTROLLER_H

#include "../../lib/Adafruit_NeoPixel/Adafruit_NeoPixel.h"
#include "AsyncTimer/AsyncTimer.h"
#include "Brightness/Brightness.h"
#include "LedModes/ColorSolid/ColorSolid.h"
#include "LedModes/ColorChanging/ColorChanging.h"

class LedController {
private:
    AsyncTimer<uint8_t>* frame_timer;       // Timer to manage frame updates
    LedMode* led_mode;
    Brightness* brightness;

    uint16_t num_led = 56;
    uint16_t color_transition_delay = 900;
    uint8_t led_controller_frame_delay = 10;
    uint16_t brightness_transition_delay = 500;
public:
    Adafruit_NeoPixel* led_strip;  // Pointer to the LED strip object

    // Constructor
    LedController(Adafruit_NeoPixel* strip, uint8_t init_r, uint8_t init_g, uint8_t init_b, uint8_t init_brightness, uint8_t init_state, uint8_t init_mode);

    // Main frame update function (called repeatedly in the loop)
    void frame();

    // Mode control
    void set_mode(uint8_t new_mode);

    // Color control
    void set_rgb(uint8_t r, uint8_t g, uint8_t b);
    void set_r(uint8_t r);
    void set_g(uint8_t g);
    void set_b(uint8_t b);
    void set_hsv(uint8_t hue, uint8_t saturation, uint8_t value);
    void set_hue(uint8_t hue);
    void set_sat(uint8_t saturation);
    void set_val(uint8_t value);

    // Brightness control
    void set_brightness(uint8_t new_brightness);

    // Power control
    void set_state(byte state);
    void turn_on();
    void turn_off();

    // Set pixels
    void fill_all(uint8_t r, uint8_t g, uint8_t b);
    void set_all_strips_pixel_color(uint16_t i, uint8_t r, uint8_t g, uint8_t b);

//    getters
    uint8_t get_r();
    uint8_t get_g();
    uint8_t get_b();
};

#endif  // LEDCONTROLLER_H
