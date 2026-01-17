#include "src/SystemController/SystemController.h"


SystemController * led_os = nullptr;


void setup() {
    led_os = new SystemController();
    led_os->begin();
}


void loop() {
    led_os->loop();
}
