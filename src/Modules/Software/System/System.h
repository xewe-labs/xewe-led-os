// src/Modules/Software/System/System.h
#pragma once

#include "../../Module/Module.h"
#include "../../../Debug.h"


struct SystemConfig : public ModuleConfig {};


class System : public Module {
public:
    explicit                    System              (SystemController& controller);

    void                begin_routines_required     (const ModuleConfig& cfg)       override;
    void                begin_routines_init         (const ModuleConfig& cfg)       override;
//    void                begin_routines_regular      (const ModuleConfig& cfg)       override;
//    void                begin_routines_common       (const ModuleConfig& cfg)       override;
//
//    void                loop                        ()                              override;
//
//    void                reset                       (const bool verbose=false)      override;
//
//    bool                enable                      (const bool verbose=false)      override;
//    bool                disable                     (const bool verbose=false)      override;
//
//    std::string         status                      (const bool verbose=false)      const override;
//    bool                is_enabled                  (const bool verbose=false)      const override;
//    bool                is_disabled                 (const bool verbose=false)      const override;
//    bool                init_setup_complete         (const bool verbose=false)      const override;

    // other methods
    std::string                 get_device_name             ();
};
