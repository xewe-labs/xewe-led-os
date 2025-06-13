#include <FastLED.h>
#include "src/Config.h"
#include "src/SystemController/SystemController.h"

// CRGB leds[LED_STRIP_NUM_LEDS_MAX];
// CRGB system_led_indicator[1];

SystemController * led_os = nullptr;

void setup() {
//     FastLED.addLeds<LED_STRIP_TYPE, PIN_LED_STRIP, LED_STRIP_COLOR_ORDER>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection( TypicalLEDStrip );
//     FastLED.addLeds<LED_INDICATOR_TYPE, PIN_LED_INDICATOR, LED_INDICATOR_COLOR_ORDER>(system_led_indicator, 1).setCorrection( TypicalLEDStrip );
//     FastLED.setBrightness(255);

    led_os = new SystemController();
    led_os->begin();
}


// long ahan = 0;
// bool ahan_2 = false;

void loop() {
    led_os->loop();

//     if(millis() - ahan > 500){
//         ahan = millis();
//         if(ahan_2){
//             system_led_indicator[0] = CRGB::Red;
//         } else {
//             system_led_indicator[0] = CRGB::Green;
//         }
//         ahan_2 = !ahan_2;
//         FastLED.show();
//     }
}
