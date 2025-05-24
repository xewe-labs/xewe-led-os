// LedSignal.cpp
#include "LedSignal.h"

LedSignal::LedSignal()
    : leds{ CRGB(255, 0, 0) }  // default to solid red
    , led_strip(
          leds,    // pointer to LED array
          1,       // length
          255,     // initial R
          0,       // initial G
          0,       // initial B
          255,     // initial brightness
          1,       // initial on/off state
          0        // initial mode (solid)
      )
{
    FastLED.addLeds<SYSTEM_STATUS_LED_TYPE,
                    SYSTEM_STATUS_LED_PIN,
                    SYSTEM_STATUS_COLOR_ORDER>(leds, 1);
}

void LedSignal::set_solid_rgb(uint8_t r, uint8_t g, uint8_t b) {
    led_strip.set_rgb(r, g, b);
}

void LedSignal::set_blinking_rgb(uint8_t r, uint8_t g, uint8_t b, uint16_t interval) {
    led_strip.set_rgb(r, g, b);
    led_strip.set_mode(LedStrip::BLINKING, interval);
}
