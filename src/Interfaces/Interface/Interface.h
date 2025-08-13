// src/Interfaces/Interface/Interface.h
#pragma once


#include "../../Modules/Module/Module.h"
#include <array>
#include <cstdint>


class Interface : public Module {
public:
    Interface(SystemController&   controller,
              std::string         module_name,
              std::string         nvs_key,
              bool                can_be_disabled,
              bool                has_cli_commands) :
        Module(controller,
               module_name,
               nvs_key,
               can_be_disabled,
               has_cli_commands)
    {}

    // required implementation
    virtual void                sync_color                  (std::array<uint8_t,3> color)   =   0;
    virtual void                sync_brightness             (uint8_t brightness)            =   0;
    virtual void                sync_state                  (uint8_t state)                 =   0;
    virtual void                sync_mode                   (uint8_t mode)                  =   0;
    virtual void                sync_length                 (uint16_t length)               =   0;
    virtual void                sync_all                    (std::array<uint8_t,3> color,
                                                             uint8_t brightness,
                                                             uint8_t state,
                                                             uint8_t mode,
                                                             uint16_t length)               =   0;
};
