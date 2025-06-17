#pragma once

#include <cstdint>
#include <array>

class SystemController;

class ControllerModule {
public:
    ControllerModule(SystemController& controller_ref) : controller(controller_ref) {}

    virtual ~ControllerModule()     {}

    virtual void begin              ()                                      = 0;
    virtual void loop               ()                                      = 0;
    virtual void reset              ()                                      = 0;

    virtual void sync_rgb           (std::array<uint8_t, 3> rgb)            = 0;
    virtual void sync_brightness    (uint8_t brightness)                    = 0;
    virtual void sync_state         (bool state)                            = 0;
    virtual void sync_mode          (uint8_t mode_id, String mode_name)     = 0;
    virtual void sync_length        (uint16_t length)                       = 0;
    virtual void sync_all           (std::array<uint8_t, 3> rgb,
                                     uint8_t brightness,
                                     bool state,
                                     uint8_t mode_id,
                                     String mode_name,
                                     uint16_t length)                       = 0;

protected:
    SystemController&               controller;
};
