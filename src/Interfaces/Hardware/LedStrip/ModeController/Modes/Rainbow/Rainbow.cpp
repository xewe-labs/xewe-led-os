#include "Rainbow.h"

Rainbow::Rainbow() : speed(5), current_hue(0) {}

void Rainbow::render(CRGB* buffer, uint16_t num_leds) {
    // Fill the buffer with a moving rainbow
    fill_rainbow(buffer, num_leds, current_hue, 255 / num_leds);

    // Advance the hue based on speed (scaling it so speed 1-10 is manageable)
    current_hue += speed;
}

std::vector<ModeParameter> Rainbow::get_params() const {
    return {
        {"speed", 1, 10, speed, 5}
    };
}

bool Rainbow::set_param(const std::string& name, int value) {
    if (name == "speed") {
        speed = constrain(value, 1, 10);
        return true;
    }
    return false;
}