#pragma once
#include "../../Hardware/LedStrip/LedModes/LedMode.h" // for hsv to rgb
#include "../../Interface/Interface.h"
#include "../../../Debug.h"
#include "HomeSpan.h"
#include <array>
#include <string>

// Forward declaration
class SystemController;

/** Optional configuration for the Homekit interface */
struct HomekitConfig : public ModuleConfig {
    std::string device_name      = "LED Strip";
    uint16_t    port             = 1201;   // default different than 80
    int         log_level        = -1;     // -1 = quiet (as in old code)
    bool        serial_input_off = true;   // disable serial input
};

/**
 * Homekit interface (new Interface pattern).
 * Exposes a LightBulb to Homekit (Hue/Saturation/Brightness/On).
 */
class Homekit : public Interface {
public:
    explicit            Homekit                 (SystemController& controller_ref);

    // Module lifecycle
    void                begin                   (const ModuleConfig& cfg)       override;
    void                loop                    ()                              override;
    void                reset                   (bool verbose=false)            override;

    // Interface sync (System -> Homekit)
    void                sync_color              (std::array<uint8_t,3> color)   override; // HSV; H,S used
    void                sync_brightness         (uint8_t brightness)            override; // 0..255 -> 0..100%
    void                sync_state              (uint8_t state)                 override; // 0=off, !0=on
    void                sync_mode               (uint8_t mode)                  override; // ignored
    void                sync_length             (uint16_t length)               override; // ignored
    void                sync_all                (std::array<uint8_t,3> color,
                                                 uint8_t brightness,
                                                 uint8_t state,
                                                 uint8_t mode,
                                                 uint16_t length)               override;

    // Extra helper (not part of Interface, preserved from old code)
    void                status                  ();

private:
    // HomeSpan LightBulb service wired to SystemController
    struct NeoPixel_RGB : Service::LightBulb {
        Characteristic::On          power   {0,   true};
        Characteristic::Hue         H       {0,   true};
        Characteristic::Saturation  S       {0,   true};
        Characteristic::Brightness  V       {100, true};

        SystemController* controller;

        explicit NeoPixel_RGB(SystemController* ctrl);
        boolean update() override; // Homekit -> System
    };

    NeoPixel_RGB*       device                  = nullptr;
    std::string         accessory_name          = "LED Strip";
};
