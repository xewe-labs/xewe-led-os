// src/Modules/.h
#pragma once

#include "../../Module/Module.h"
#include "../../../Debug.h"


struct ModuleTemplateConfig : public ModuleConfig {
};


class ModuleTemplate : public Module {
public:
    explicit                    ModuleTemplate              (SystemController& controller);

    // required implementation
    void                        begin                       (const ModuleConfig& cfg)       override;
    void                        loop                        ()                              override;
    void                        reset                       (bool verbose=false)            override;

    // optional implementation
    // virtual bool             init_setup                  (bool verbose=false,
    //                                                       bool enable_prompt=true,
    //                                                       bool reboot_after=false)       override;
    // bool                     enable                      (bool verbose=false)            override;
    // bool                     disable                     (bool verbose=false)            override;
    // std::string              status                      (bool verbose=false) const      override;

    // other methods

private:

};
