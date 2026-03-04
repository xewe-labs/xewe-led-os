#pragma once

#include <FastLED.h>
#include <memory>
#include <vector>
#include "Modes/Mode/Mode.h"

// Forward declarations for modes to avoid circular dependencies if needed
#include "Modes/Solid/Solid.h"
#include "Modes/Rainbow/Rainbow.h"

enum class ModeID : uint8_t {
    SOLID = 0,
    RAINBOW = 1
};

class ModeController {
public:
    ModeController(uint16_t num_leds, uint16_t transition_delay_ms);
    ~ModeController() = default;

    // Core loop to be called by LedStrip::loop()
    void update(CRGB* output_buffer);

    // Dynamic mode switching
    void set_mode(ModeID new_mode_id);

    // Interface helpers
    std::string get_current_mode_name() const;
    std::vector<ModeParameter> get_current_mode_params() const;
    bool set_current_mode_param(const std::string& name, int value);

    static std::array<uint8_t, 3> hsv_to_rgb                (const std::array<uint8_t, 3> hsv);
    static std::array<uint8_t, 3> rgb_to_hsv                (const std::array<uint8_t, 3> rgb);
private:
    uint16_t num_leds;
    uint16_t transition_delay_ms;

    // State
    bool is_transitioning;
    bool using_snapshot;
    uint32_t transition_start_time;

    // Smart pointers to manage mode lifecycles elegantly
    std::unique_ptr<Mode> current_mode;
    std::unique_ptr<Mode> old_mode;

    // Buffers for transition interpolation
    std::vector<CRGB> buffer_current;
    std::vector<CRGB> buffer_old;
    std::vector<CRGB> buffer_snapshot;

    // Factory method for dynamic initialization
    std::unique_ptr<Mode> create_mode(ModeID id);
};