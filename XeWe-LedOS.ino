#include <FastLED.h>
#include "src/Config.h"
#include "src/SystemController/SystemController.h"

CRGB leds[LED_STRIP_NUM_LEDS_MAX];
// CRGB system_led_indicator[1];

SystemController * led_os = nullptr;

void setup() {
    FastLED.addLeds<LED_STRIP_TYPE, PIN_LED_STRIP, LED_STRIP_COLOR_ORDER>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection( TypicalLEDStrip );
//     FastLED.addLeds<LED_INDICATOR_TYPE, PIN_LED_INDICATOR, LED_INDICATOR_COLOR_ORDER>(system_led_indicator, 1).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(255);

    led_os = new SystemController(leds);
}

void loop() {
  led_os->update();
}
