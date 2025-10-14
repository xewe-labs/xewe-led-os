// src/Interfaces/InterfaceName/InterfaceName.cpp

#include "InterfaceName.h"
#include "../../../SystemController/SystemController.h"


// required
InterfaceName::InterfaceName(SystemController& controller)
      : Interface(controller,
               /* module_name         */ "",
               /* module_description  */ "",
               /* nvs_key             */ "",
               /* requires_init_setup */ false,
               /* can_be_disabled     */ false,
               /* has_cli_cmds        */ false)
{}


void InterfaceName::sync_color(std::array<uint8_t,3> color) {
    // received new value, propagate it in the module
}

void InterfaceName::sync_brightness(uint8_t brightness) {
    // received new value, propagate it in the module
}

void InterfaceName::sync_state(uint8_t state) {
    // received new value, propagate it in the module
}

void InterfaceName::sync_mode(uint8_t mode) {
    // received new value, propagate it in the module
}

void InterfaceName::sync_length(uint16_t length) {
    // received new value, propagate it in the module
}

// optional
void InterfaceName::sync_all(std::array<uint8_t,3> color,
                   uint8_t brightness,
                   uint8_t state,
                   uint8_t mode,
                   uint16_t length) {
   // some modules might need custom sequence for correct setup
}

void InterfaceName::begin_routines_required (const ModuleConfig& cfg) {
//    const auto& config = static_cast<const InterfaceNameConfig&>(cfg);
    // do your custom routines here
}

void InterfaceName::begin_routines_init (const ModuleConfig& cfg) {
//    const auto& config = static_cast<const InterfaceNameConfig&>(cfg);
    // do your custom routines here
}

void InterfaceName::begin_routines_regular (const ModuleConfig& cfg) {
//    const auto& config = static_cast<const InterfaceNameConfig&>(cfg);
    // do your custom routines here
}

void InterfaceName::begin_routines_common (const ModuleConfig& cfg) {
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

// other methods
