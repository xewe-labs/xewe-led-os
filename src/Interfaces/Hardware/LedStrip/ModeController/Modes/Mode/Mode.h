#pragma once

#include <FastLED.h>
#include <string>
#include <vector>

// Struct to represent mode parameters for the Web UI and CLI
struct ModeParameter {
    std::string name;
    int min_val;
    int max_val;
    int value;
    int default_val;
};

class Mode {
public:
    virtual ~Mode() = default;

    // Renders the mode's current state to the provided buffer
    virtual void render(CRGB* buffer, uint16_t num_leds) = 0;

    // Returns a list of parameters for UI rendering
    virtual std::vector<ModeParameter> get_params() const = 0;

    // Sets a parameter dynamically (returns true if parameter exists)
    virtual bool set_param(const std::string& name, int value) = 0;

    // Returns the mode name
    virtual std::string get_name() const = 0;
};