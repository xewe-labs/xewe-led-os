// src/Interfaces/.h
#pragma once


#include "../../Interface/Interface.h"
#include "../../../Debug.h"


struct InterfaceTemplateConfig : public ModuleConfig {
};


class InterfaceTemplate : public Interface {
public:
    explicit                    InterfaceTemplate           (SystemController& controller);

    // required implementation
    bool                        begin                       (const ModuleConfig& cfg)       override;
    void                        loop                        ()                              override;
    void                        reset                       (bool verbose=false)            override;

    void                        sync_color                  (std::array<uint8_t,3> color)   override;
    void                        sync_brightness             (uint8_t brightness)            override;
    void                        sync_state                  (uint8_t state)                 override;
    void                        sync_mode                   (uint8_t mode)                  override;
    void                        sync_length                 (uint16_t length)               override;
    void                        sync_all                    (std::array<uint8_t,3> color,
                                                             uint8_t brightness,
                                                             uint8_t state,
                                                             uint8_t mode,
                                                             uint16_t length)               override;
    // optional implementation
    // virtual bool             init_setup                  (bool verbose=false,
    //                                                       bool enable_prompt=true,
    //                                                       bool reboot_after=false)       override;
    // bool                        enable                   (bool verbose=false)            override;
    // bool                        disable                  (bool verbose=false)            override;
    // std::string                 status                   (bool verbose=false) const      override;
    // bool                        is_enabled               (bool verbose=false) const      override;
    // bool                        is_disabled              (bool verbose=false) const      override;

    // other methods
};
