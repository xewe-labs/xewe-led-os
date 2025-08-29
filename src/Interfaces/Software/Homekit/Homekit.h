// Software/Homekit/Homekit.h
#pragma once

#include "../../Interface/Interface.h"
#include "../../../Debug.h"

#include <array>
#include <cstdint>

class SystemController;

struct HomekitConfig : public ModuleConfig {};
/**
 * Homekit interface using HomeSpan. Bridges HomeKit Hue/Sat/Brightness/On
 * to controller.sync_color/sync_brightness/sync_state.
 */
class Homekit : public Interface {
public:
    explicit Homekit(SystemController& controller);

    // Module interface
    void begin(const ModuleConfig& cfg) override;
    void loop() override;
    void reset(bool verbose=false) override;

    // Interface sync: System -> HomeKit
    void sync_color(std::array<uint8_t,3> color) override;
    void sync_brightness(uint8_t brightness) override;
    void sync_state(uint8_t state) override;
    void sync_mode(uint8_t mode) override;
    void sync_length(uint16_t length) override;
    void sync_all(std::array<uint8_t,3> color,
                  uint8_t brightness,
                  uint8_t state,
                  uint8_t mode,
                  uint16_t length) override;

private:
    struct NeoPixel_RGB;   // forward (defined in .cpp)
    NeoPixel_RGB* device_ = nullptr;

    // helpers
    static void rgb_to_hs(std::array<uint8_t,3> rgb, float& h_deg, float& s_pct);
};
