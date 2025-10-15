// src/Modules/ModuleName/ModuleName.h
#pragma once

#include "../../Module/Module.h"
#include "../../../Config.h"
#include "../../../Debug.h"


struct ModuleNameConfig : public ModuleConfig {};


class ModuleName : public Module {
public:
    explicit                    ModuleName              (SystemController& controller);

    // optional implementation
//    void                begin_routines_required     (const ModuleConfig& cfg)       override;
//    void                begin_routines_init         (const ModuleConfig& cfg)       override;
//    void                begin_routines_regular      (const ModuleConfig& cfg)       override;
//    void                begin_routines_common       (const ModuleConfig& cfg)       override;
//
//    void                loop                        ()                              override;
//
//    void                reset                       (const bool verbose=false)      override;
//
//    void                enable                      (const bool verbose=false)      override;
//    void                disable                     (const bool verbose=false)      override;
//
//    std::string         status                      (const bool verbose=false)      const override;
//    bool                is_enabled                  (const bool verbose=false)      const override;
//    bool                is_disabled                 (const bool verbose=false)      const override;
//    bool                init_setup_complete         (const bool verbose=false)      const override;

    // other methods

private:

};
