#include "LedStrip.h"

LedStrip::LedStrip(CRGB* leds_ptr)
    : leds(leds_ptr),
      num_led(0),
      frame_timer(std::make_unique<AsyncTimer<uint8_t>>(led_controller_frame_delay)),
      brightness(std::make_unique<Brightness>(brightness_transition_delay, 0, 0)),
      led_mode(std::make_unique<ColorSolid>(this, 0, 0, 0))
{
    DBG_PRINTLN(LedStrip, "LedStrip: Constructor called");
    frame_timer->initiate();
}

void LedStrip::frame() {
    if (frame_timer->is_active())
        return;
    frame_timer->reset();
    frame_timer->initiate();

    led_mode->frame();

    switch (led_mode->get_mode_id()) {
        case 0:
            break;

        case 1:
            if (led_mode->is_done()) {
                led_mode->frame();
                std::array<uint8_t, 3> rgb_temp = led_mode->get_rgb();
                led_mode = std::make_unique<ColorSolid>(this, rgb_temp[0], rgb_temp[1], rgb_temp[2]);
            }
            break;

        case 3:
            DBG_PRINTLN(LedStrip, "Mode: DIRECTIONAL_FILLER");
            break;

        case 4:
            DBG_PRINTLN(LedStrip, "Mode: PERLIN");
            break;

        default:
            DBG_PRINTLN(LedStrip, "Unknown mode");
            break;
    }
}

void LedStrip::set_mode(uint8_t new_mode) {
    DBG_PRINTF(LedStrip, "set_mode: %d\n", new_mode);
    //todo
}

void LedStrip::set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    DBG_PRINTF(LedStrip, "set_rgb: R=%d G=%d B=%d\n", r, g, b);

    std::array<uint8_t, 3> old_rgb = led_mode->get_rgb();
    if (old_rgb == std::array<uint8_t,3>{r,g,b}) {
        DBG_PRINTLN(LedStrip, "Color already set");
        return;
    }

    led_mode = std::make_unique<ColorChanging>(
        this,
        old_rgb[0], old_rgb[1], old_rgb[2],
        r, g, b,
        'r',
        color_transition_delay);
}

void LedStrip::set_r(uint8_t r) { set_rgb(r, get_g(), get_b()); }
void LedStrip::set_g(uint8_t g) { set_rgb(get_r(), g, get_b()); }
void LedStrip::set_b(uint8_t b) { set_rgb(get_r(), get_g(), b); }

void LedStrip::set_hsv(uint8_t h, uint8_t s, uint8_t v) {
    DBG_PRINTF(LedStrip, "set_hsv: H=%d S=%d V=%d\n", h, s, v);
    if (led_mode->get_hsv() == std::array<uint8_t,3>{h,s,v}) {
        DBG_PRINTLN(LedStrip, "Color already set");
        return;
    }

    std::array<uint8_t, 3> old_rgb = led_mode->get_rgb();

    led_mode = std::make_unique<ColorChanging>(
        this,
        old_rgb[0], old_rgb[1], old_rgb[2],
        h, s, v,
        'h',
        color_transition_delay);
}

void LedStrip::set_h(uint8_t h) { set_hsv(h, led_mode->get_s(), led_mode->get_v()); }
void LedStrip::set_s(uint8_t s) { set_hsv(led_mode->get_h(), s, led_mode->get_v()); }
void LedStrip::set_v(uint8_t v) { set_hsv(led_mode->get_h(), led_mode->get_s(), v); }

void LedStrip::set_brightness(uint8_t new_brightness) {
    brightness->set_brightness(new_brightness);
}

void LedStrip::set_state(uint8_t state) {
    if (state)
        turn_on();
    else
        turn_off();
}

void LedStrip::turn_on() { brightness->turn_on(); }
void LedStrip::turn_off() { brightness->turn_off(); }

void LedStrip::fill_all(uint8_t r, uint8_t g, uint8_t b) {
    for (uint16_t i = 0; i < num_led; i++)
        set_all_strips_pixel_color(i, r, g, b);
    FastLED.show();
}

void LedStrip::set_all_strips_pixel_color(uint16_t i, uint8_t r, uint8_t g, uint8_t b) {
    leds[i] = CRGB(brightness->get_dimmed_color(r),
                   brightness->get_dimmed_color(g),
                   brightness->get_dimmed_color(b));
}

void LedStrip::set_length(uint16_t length) {
    fill_all(0, 0, 0);
    num_led = length;
}

std::array<uint8_t,3>   LedStrip::get_rgb() const { return led_mode->get_rgb(); }
uint8_t                 LedStrip::get_r() const { return led_mode->get_r(); }
uint8_t                 LedStrip::get_g() const { return led_mode->get_g(); }
uint8_t                 LedStrip::get_b() const { return led_mode->get_b(); }

std::array<uint8_t,3>   LedStrip::get_target_rgb() const { return led_mode->get_target_rgb(); }
uint8_t                 LedStrip::get_target_r() const { return led_mode->get_target_r(); }
uint8_t                 LedStrip::get_target_g() const { return led_mode->get_target_g(); }
uint8_t                 LedStrip::get_target_b() const { return led_mode->get_target_b(); }

std::array<uint8_t, 3>  LedStrip::get_hsv() const { return led_mode->get_hsv(); }
uint8_t                 LedStrip::get_h() const { return led_mode->get_h(); }
uint8_t                 LedStrip::get_s() const { return led_mode->get_s(); }
uint8_t                 LedStrip::get_v() const { return led_mode->get_v(); }

std::array<uint8_t, 3>  LedStrip::get_target_hsv() const { return led_mode->get_target_hsv(); }
uint8_t                 LedStrip::get_target_h() const { return led_mode->get_target_h(); }
uint8_t                 LedStrip::get_target_s() const { return led_mode->get_target_s(); }
uint8_t                 LedStrip::get_target_v() const { return led_mode->get_target_v(); }

uint8_t LedStrip::get_brightness() const { return brightness->get_last_brightness(); }
bool LedStrip::get_state() const { return brightness->get_state(); }
