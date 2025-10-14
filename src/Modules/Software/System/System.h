// src/Modules/Software/System/System.h
#pragma once

#include "../../Module/Module.h"
#include "../../../Config.h"
#include "../../../Debug.h"


struct SystemConfig : public ModuleConfig {};


class System : public Module {
public:
    explicit                    System                      (SystemController& controller);

    void                        begin_routines_required     (const ModuleConfig& cfg)       override;
    void                        begin_routines_init         (const ModuleConfig& cfg)       override;

    // other methods
    std::string                 get_device_name             ();
};
