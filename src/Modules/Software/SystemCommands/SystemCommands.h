// src/Modules/.h
#pragma once

#include "../../Module/Module.h"
#include "../../../Debug.h"


struct SystemCommandsConfig : public ModuleConfig {
};


class SystemCommands : public Module {
public:
    explicit                    SystemCommands              (SystemController& controller);

    // required implementation
    void                        begin                       (const ModuleConfig& cfg)       override;
    void                        loop                        ()                              override;
    void                        reset                       (bool verbose=false)            override;

    // optional implementation
    // bool                     enable                      (bool verbose=false)            override;
    // bool                     disable                     (bool verbose=false)            override;
    // std::string              status                      (bool verbose=false) const      override;
    // bool                     is_enabled                  (bool verbose=false) const      override;
    // bool                     is_disabled                 (bool verbose=false) const      override;

    // other methods

private:

};
