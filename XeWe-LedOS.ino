#include <Adafruit_NeoPixel.h>
#define LED_PIN 2
#define NUM_LEDS 56

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);


#include "src/SystemController/SystemController.h"

SystemController * led_os = nullptr;

void setup() {
  led_os = new SystemController(&strip);
  led_os->init_system_setup();
}

void loop() {
  led_os->update();
}
