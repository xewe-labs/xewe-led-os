/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/


// THIS TEMPLATE NEEDS TO BE UPDATED
// THIS TEMPLATE NEEDS TO BE UPDATED
// THIS TEMPLATE NEEDS TO BE UPDATED
// THIS TEMPLATE NEEDS TO BE UPDATED
// THIS TEMPLATE NEEDS TO BE UPDATED
// THIS TEMPLATE NEEDS TO BE UPDATED
// THIS TEMPLATE NEEDS TO BE UPDATED
// THIS TEMPLATE NEEDS TO BE UPDATED
// THIS TEMPLATE NEEDS TO BE UPDATED
// THIS TEMPLATE NEEDS TO BE UPDATED
// THIS TEMPLATE NEEDS TO BE UPDATED
// THIS TEMPLATE NEEDS TO BE UPDATED
// THIS TEMPLATE NEEDS TO BE UPDATED
// THIS TEMPLATE NEEDS TO BE UPDATED
// THIS TEMPLATE NEEDS TO BE UPDATED
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
    if (is_disabled()) return;
    // received new value, propagate it in the module
}

void InterfaceName::sync_brightness(uint8_t brightness) {
    if (is_disabled()) return;
    // received new value, propagate it in the module
}

void InterfaceName::sync_state(uint8_t state) {
    if (is_disabled()) return;
    // received new value, propagate it in the module
}

void InterfaceName::sync_mode(uint8_t mode) {
    if (is_disabled()) return;
    // received new value, propagate it in the module
}

void InterfaceName::sync_length(uint16_t length) {
    if (is_disabled()) return;
    // received new value, propagate it in the module
}

// optional
void InterfaceName::sync_all(std::array<uint8_t,3> color,
                   uint8_t brightness,
                   uint8_t state,
                   uint8_t mode,
                   uint16_t length) {
    if (is_disabled()) return;

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
   if (is_disabled()) return;
    // do your custom routines here
}

void InterfaceName::reset (const bool verbose) {
    // do your custom routines here
    Module::reset(verbose);  // this will restart the system
}

void InterfaceName::enable (const bool verbose) {
    // do your custom routines here
    Module::enable(verbose);  // this will restart the system
}

void InterfaceName::disable (const bool verbose) {
    // do your custom routines here
    Module::disable(verbose); // this will restart the system
}

std::string InterfaceName::status (const bool verbose) const {
    return Module::status(verbose);
    // do your custom routines here
}

// other methods
// make sure they have
// if (is_disabled()) return;
