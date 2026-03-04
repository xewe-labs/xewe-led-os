#pragma once

#include "../Mode/Mode.h"

class Solid : public Mode {
public:
    Solid();

    void render(CRGB* buffer, uint16_t num_leds) override;
    std::vector<ModeParameter> get_params() const override;
    bool set_param(const std::string& name, int value) override;
    std::string get_name() const override { return "Solid"; }

private:
    int h, s, v;
};