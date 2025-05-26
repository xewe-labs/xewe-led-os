// LedStrip.cpp
#include "LedStrip.h"

LedStrip::LedStrip(CRGB* leds_ptr,
                             uint16_t init_length,
                             uint8_t init_r,
                             uint8_t init_g,
                             uint8_t init_b,
                             uint8_t init_brightness,
                             uint8_t init_state,
                             uint8_t init_mode)
    : leds(leds_ptr), num_led(init_length) {
    // Clear any residual data
//    FastLED.clear();

    frame_timer = new AsyncTimer<uint8_t>(led_controller_frame_delay);
    if (init_mode == 0) {
        led_mode = new ColorSolid(this, init_r, init_g, init_b);
    } else {
        // to be implemented for other modes
        led_mode = new ColorSolid(this, init_r, init_g, init_b);
    }
    brightness = new Brightness(brightness_transition_delay, init_brightness, init_state);

    DBG_PRINTLN(LedStrip, "LedStrip: Constructor called");
}

void LedStrip::frame() {
    if (frame_timer->is_active())
        return;
    frame_timer->reset();

    brightness->frame();
    led_mode->frame();

    switch (led_mode->get_mode_id()) {
        case 1:
            break;

        case 2:
            if (led_mode->is_done()) {
                led_mode->frame();
                delete led_mode;
                led_mode = new ColorSolid(this);
            }
            break;

        case 3:
            DBG_PRINTLN(LedStrip, "Mode: DIRECTIONAL_FILLER");
            break;

        case 4:
            DBG_PRINTLN(LedStrip, "Mode: PERLIN");
            break;

        case 0:
            DBG_PRINTLN(LedStrip, "Mode: OFF");
            break;

        default:
            DBG_PRINTLN(LedStrip, "Unknown mode");
            break;
    }
}

void LedStrip::set_mode(uint8_t new_mode) {
    DBG_PRINTF(LedStrip,
               "LedStrip: Function: set_mode, mode = %d\n",
               new_mode);
    if (new_mode == 0) {
        delete led_mode;
        led_mode = new ColorSolid(this);
    }
    // Extend for other modes
}

void LedStrip::set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    DBG_PRINTF(LedStrip,
               "LedStrip: Function: set_rgb, R = %d, G = %d, B = %d\n",
               r, g, b);
    if (!led_mode || !led_mode->is_done()) {
        DBG_PRINTLN(LedStrip,
                    "LedStrip: Still changing previous color or led_mode is null");
        return;
    }
    if (r == led_mode->get_r() &&
        g == led_mode->get_g() &&
        b == led_mode->get_b()) {
        DBG_PRINTLN(LedStrip, "LedStrip: Color already set");
        return;
    }
    uint8_t old_r = led_mode->get_r();
    uint8_t old_g = led_mode->get_g();
    uint8_t old_b = led_mode->get_b();
    delete led_mode;
    led_mode = new ColorChanging(this,
                                 old_r, old_g, old_b,
                                 r, g, b,
                                 'r',
                                 color_transition_delay);
}

void LedStrip::set_r(uint8_t r) {
    DBG_PRINTF(LedStrip,
               "LedStrip: Function: set_r, R = %d\n",
               r);
    set_rgb(r, led_mode->get_g(), led_mode->get_b());
}

void LedStrip::set_g(uint8_t g) {
    DBG_PRINTF(LedStrip,
               "LedStrip: Function: set_g, G = %d\n",
               g);
    set_rgb(led_mode->get_r(), g, led_mode->get_b());
}

void LedStrip::set_b(uint8_t b) {
    DBG_PRINTF(LedStrip,
               "LedStrip: Function: set_b, B = %d\n",
               b);
    set_rgb(led_mode->get_r(), led_mode->get_g(), b);
}

uint8_t LedStrip::get_r() const { return led_mode->get_r(); }
uint8_t LedStrip::get_g() const { return led_mode->get_g(); }
uint8_t LedStrip::get_b() const { return led_mode->get_b(); }

void LedStrip::set_hsv(uint8_t hue, uint8_t saturation, uint8_t value) {
    DBG_PRINTF(LedStrip,
               "LedStrip: Function: set_hsv, H = %d, S = %d, V = %d\n",
               hue, saturation, value);
    if (!led_mode->is_done()) {
        DBG_PRINTLN(LedStrip, "LedStrip: Still changing previous color");
        return;
    }
    if (hue == led_mode->get_hue() &&
        saturation == led_mode->get_sat() &&
        value == led_mode->get_val()) {
        DBG_PRINTLN(LedStrip, "LedStrip: HSV already set");
        return;
    }
    uint8_t old_r = led_mode->get_r();
    uint8_t old_g = led_mode->get_g();
    uint8_t old_b = led_mode->get_b();
    delete led_mode;
    led_mode = new ColorChanging(this,
                                 old_r, old_g, old_b,
                                 hue, saturation, value,
                                 'h',
                                 color_transition_delay);
}

void LedStrip::set_hue(uint8_t hue) {
    set_hsv(hue, led_mode->get_sat(), led_mode->get_val());
}

void LedStrip::set_sat(uint8_t saturation) {
    set_hsv(led_mode->get_hue(), saturation, led_mode->get_val());
}

void LedStrip::set_val(uint8_t value) {
    set_hsv(led_mode->get_hue(), led_mode->get_sat(), value);
}

void LedStrip::set_brightness(uint8_t new_brightness) {
    DBG_PRINTF(LedStrip,
               "LedStrip: Function: set_brightness, brightness = %d\n",
               new_brightness);
    if (brightness->get_target_value() == new_brightness) {
        DBG_PRINTLN(LedStrip,
                    "LedStrip: Function: set_brightness: Already set to this value.");
        return;
    }
    brightness->set_brightness(new_brightness);
}

void LedStrip::set_state(byte target_state) {
    if (target_state)
        turn_on();
    else
        turn_off();
}

void LedStrip::turn_on() {
    DBG_PRINTLN(LedStrip, "LedStrip: Function: turn_on");
    brightness->turn_on();
}

void LedStrip::turn_off() {
    DBG_PRINTLN(LedStrip, "LedStrip: Function: turn_off");
    brightness->turn_off();
}

void LedStrip::fill_all(uint8_t r, uint8_t g, uint8_t b) {
    //DBG_PRINTF(LedStrip,
    //           "LedStrip: Function: fill_all, R = %d, G = %d, B = %d\n",
    //           r, g, b);
    for (int i = 0; i < num_led; i++) {
        set_all_strips_pixel_color(i, r, g, b);
    }
    FastLED.show();
}

void LedStrip::set_all_strips_pixel_color(uint16_t i,
                                               uint8_t r,
                                               uint8_t g,
                                               uint8_t b) {
    uint8_t dr = brightness->get_dimmed_color(r);
    uint8_t dg = brightness->get_dimmed_color(g);
    uint8_t db = brightness->get_dimmed_color(b);
    leds[i] = CRGB(dr, dg, db);
}

void LedStrip::set_length(uint16_t length) {
    DBG_PRINTF(LedStrip,
               "Function: set_length, length = %d\n",
               length);
//    if (length > NUM_LEDS_MAX){
//
//    }
    fill_all(0, 0, 0);
    num_led = length;
}

uint8_t LedStrip::get_brightness() const {
    DBG_PRINTLN(LedStrip, "uint8_t LedStrip::get_brightness() const {");
    return brightness->get_target_value();
}

bool LedStrip::get_state() const {
    brightness->get_state();
}

