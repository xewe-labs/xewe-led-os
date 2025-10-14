// src/Modules/ModuleName/ModuleName.cpp

#include "ModuleName.h"
#include "../../../SystemController/SystemController.h"


ModuleName::ModuleName(SystemController& controller)
      : Module(controller,
               /* module_name         */ "",
               /* module_description  */ "",
               /* nvs_key             */ "",
               /* requires_init_setup */ false,
               /* can_be_disabled     */ false,
               /* has_cli_cmds        */ false)
{}

//  begin(cfg):
//      begin_routines_required(cfg)
//      if first_time_startup
//           begin_routines_init(cfg)
//      else
//          begin_routines_regular(cfg)
//      begin_routines_common(cfg)

void ModuleName::begin_routines_required (const ModuleConfig& cfg) {
//    const auto& config = static_cast<const ModuleNameConfig&>(cfg);
    // do your custom routines here
}

void ModuleName::begin_routines_init (const ModuleConfig& cfg) {
//    const auto& config = static_cast<const ModuleNameConfig&>(cfg);
    // do your custom routines here
}

void ModuleName::begin_routines_regular (const ModuleConfig& cfg) {
//    const auto& config = static_cast<const ModuleNameConfig&>(cfg);
    // do your custom routines here
}

void ModuleName::begin_routines_common (const ModuleConfig& cfg) {
//    const auto& config = static_cast<const ModuleNameConfig&>(cfg);
    // do your custom routines here
}

void ModuleName::loop () {
    // do your custom routines here
}

void ModuleName::reset (const bool verbose) {
    Module::reset(verbose);
    // do your custom routines here
}

bool ModuleName::enable (const bool verbose) {
    return Module::enable(verbose);
    // do your custom routines here
}

bool ModuleName::disable (const bool verbose) {
    return Module::disable(verbose);
    // do your custom routines here
}

std::string ModuleName::status (const bool verbose) const {
    return Module::status(verbose);
    // do your custom routines here
}
