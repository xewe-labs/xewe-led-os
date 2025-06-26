#pragma once

#include "ControllerModule.h"

class ControllerInterface : public ControllerModule {
public:
    ControllerInterface(SystemController& controller_ref) : ControllerModule(controller_ref) {}

    virtual ~ControllerInterface()     {}

    // --- These are the original functions from ControllerInterface ---
    virtual void sync_color                 (std::array<uint8_t, 3> color)          = 0;
    virtual void sync_brightness            (uint8_t brightness)                    = 0;
    virtual void sync_state                 (bool state)                            = 0;
    virtual void sync_mode                  (uint8_t mode_id, String mode_name)     = 0;
    virtual void sync_length                (uint16_t length)                       = 0;
    virtual void sync_all                   (std::array<uint8_t, 3> color,
                                             uint8_t brightness,
                                             bool state,
                                             uint8_t mode_id,
                                             String mode_name,
                                             uint16_t length)                       = 0;

};