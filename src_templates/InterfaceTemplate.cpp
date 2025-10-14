// src/Interfaces/InterfaceName/InterfaceName.cpp

#include "InterfaceName.h"
#include "../../../SystemController/SystemController.h"


InterfaceName::InterfaceName(SystemController& controller)
      : Interface(controller,
               /* module_name         */ "",
               /* module_description  */ "",
               /* nvs_key             */ "",
               /* requires_init_setup */ false,
               /* can_be_disabled     */ false,
               /* has_cli_cmds        */ false)
{}


void Nvs::sync_color(std::array<uint8_t,3> color) {
    // received new value, propagate it in the module
}

void Nvs::sync_brightness(uint8_t brightness) {
    // received new value, propagate it in the module
}

void Nvs::sync_state(uint8_t state) {
    // received new value, propagate it in the module
}

void Nvs::sync_mode(uint8_t mode) {
    // received new value, propagate it in the module
}

void Nvs::sync_length(uint16_t length) {
    // received new value, propagate it in the module
}

void InterfaceName::begin_routines_required (const InterfaceConfig& cfg) {
//    const auto& config = static_cast<const InterfaceNameConfig&>(cfg);
    // do your custom routines here
}

void InterfaceName::begin_routines_init (const InterfaceConfig& cfg) {
//    const auto& config = static_cast<const InterfaceNameConfig&>(cfg);
    // do your custom routines here
}

void InterfaceName::begin_routines_regular (const InterfaceConfig& cfg) {
//    const auto& config = static_cast<const InterfaceNameConfig&>(cfg);
    // do your custom routines here
}

void InterfaceName::begin_routines_common (const InterfaceConfig& cfg) {
//    const auto& config = static_cast<const InterfaceNameConfig&>(cfg);
    // do your custom routines here
}

void InterfaceName::loop () {
    // do your custom routines here
}

void InterfaceName::reset (const bool verbose) {
    Module::reset(verbose);
    // do your custom routines here
}

bool InterfaceName::enable (const bool verbose) {
    return Module::enable(verbose);
    // do your custom routines here
}

bool InterfaceName::disable (const bool verbose) {
    return Module::disable(verbose);
    // do your custom routines here
}

std::string InterfaceName::status (const bool verbose) const {
    return Module::status(verbose);
    // do your custom routines here
}
