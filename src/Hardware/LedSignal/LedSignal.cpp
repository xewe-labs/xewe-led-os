//#include "LedSignal.h"
//
//LedSignal::LedSignal()
//    : leds{ CRGB(255, 0, 0) },  // default to solid red
//      led_strip(std::make_unique<LedStrip>(leds))
//{
//    FastLED.addLeds<SYSTEM_STATUS_LED_TYPE,
//                    SYSTEM_STATUS_LED_PIN,
//                    SYSTEM_STATUS_COLOR_ORDER>(leds, 1);
//}
//
//void LedSignal::set_solid_rgb(uint8_t r, uint8_t g, uint8_t b) {
//    led_strip->set_rgb(r, g, b);
//}
//
//void LedSignal::set_blinking_rgb(uint8_t r, uint8_t g, uint8_t b, uint16_t interval) {
//    led_strip->set_rgb(r, g, b);
//    // Uncomment and implement mode switching when blinking is available:
//    // led_strip->set_mode(LedModeID::MODE_BLINKING, interval);
//}
