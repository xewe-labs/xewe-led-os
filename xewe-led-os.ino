#include "src/Modules/Module/ModuleController.h"

ModuleController * led_os = nullptr;

void setup() {
    led_os = new ModuleController();
    led_os->begin();
}

void loop() {
    led_os->loop();
}
