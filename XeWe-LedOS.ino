#include <FastLED.h>
#include "src/SystemController/SystemController.h"

#define LED_PIN     0
#define LED_TYPE    WS2815
// #define COLOR_ORDER GRB
#define COLOR_ORDER BRG

#define NUM_LEDS_MAX    1000
#define BRIGHTNESS_MAX  255
CRGB leds[NUM_LEDS_MAX];

SystemController * led_os = nullptr;

void setup() {
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS_MAX).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(  BRIGHTNESS_MAX );

    led_os = new SystemController(leds);
}

void loop() {
  led_os->update();
}
