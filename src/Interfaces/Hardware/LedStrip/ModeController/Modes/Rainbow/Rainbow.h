#pragma once

#include "../Mode/Mode.h"

class Rainbow : public Mode {
public:
    Rainbow();

    void render(CRGB* buffer, uint16_t num_leds) override;
    std::vector<ModeParameter> get_params() const override;
    bool set_param(const std::string& name, int value) override;
    std::string get_name() const override { return "Rainbow"; }

private:
    int speed;
    uint8_t current_hue;
};