// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// <filepath from project root>

#include "ModuleName.h"
#include "../Module/ModuleController.h" // adjust this path if needed


ModuleName::ModuleName(ModuleController& controller)
      : Module(controller,
               /* id                  */ "",
               /* name                */ "",
               /* description         */ "",
               /* requires_init_setup */ false,
               /* can_be_disabled     */ false,
               /* has_cli_cmds        */ false)
{}

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

void ModuleName::enable (const bool verbose, const bool do_restart) {
    // do your custom routines here
    // IMPORTANT after, call the parent method
    return Module::enable(verbose, do_restart);
}

void ModuleName::disable (const bool verbose, const bool do_restart) {
    // do your custom routines here
    // IMPORTANT after, call the parent method
    Module::disable(verbose, do_restart);
}

void ModuleName::reset (const bool verbose, const bool do_restart, const bool keep_enabled) {
    // do your custom routines here
    // IMPORTANT after, call the parent method
    Module::reset(verbose, do_restart, keep_enabled);
}

std::string ModuleName::status (const bool verbose) const {
    // do your custom routines here
    string status = "custom status";

    if (verbose)
        controller.serial_port.print(status);

    return status;
}

void ModuleName::custom_function () {
    // IMPORTANT: sure to have this line, otherwise if other modules call it when disabled, this will lead to undesired bugs.
    if (is_disabled()) return;

    // do your custom routines here
}
