// src/Modules/Module.cpp

#include "Module.h"
#include "../SystemController.h"
#include <Arduino.h>
#include <string>

void Module::register_generic_commands() {
    // “status” command
    commands_storage.push_back(Command{
        "status",
        "Get module status",
        std::string("Sample Use: $") + module_name + " status",
        0,
        [this](std::string_view) {
            status();
        }
    });

    // “reset” command
    commands_storage.push_back(Command{
        "reset",
        "Reset the module",
        std::string("Sample Use: $") + module_name + " reset",
        0,
        [this](std::string_view) {
            reset();
        }
    });

    // “enable” / “disable” commands (if supported)
    if (can_be_disabled) {
        commands_storage.push_back(Command{
            "enable",
            "Enable this module",
            std::string("Sample Use: $") + module_name + " enable",
            0,
            [this](std::string_view) {
                enable();
            }
        });
        commands_storage.push_back(Command{
            "disable",
            "Disable this module",
            std::string("Sample Use: $") + module_name + " disable",
            0,
            [this](std::string_view) {
                disable();
            }
        });
    }
}

// returns success of the operation
bool Module::enable(bool verbose) {
    if (is_enabled()) return false;
    if (!can_be_disabled) return false;

    enabled = true;
    Serial.printf("Enabled %s module", module_name.c_str());
    return true;
}

// returns success of the operation
bool Module::disable(bool verbose) {
    if (is_disabled()) return false;
    if (!can_be_disabled) return false;

    enabled = false;
    Serial.printf("Disabled %s module", module_name.c_str());
    return true;
}


std::string_view Module::status(bool print) const {
    if (print) {
        Serial.printf("ok");
    }
    return "ok";
}

bool Module::is_enabled(bool verbose) const {
    if (can_be_disabled) {
        if (verbose) {
            Serial.printf("%s module %s\n", module_name.c_str(), (enabled ? "enabled" : "disabled"));
        }
        return enabled;
    } else {
        if (verbose) {
            Serial.printf("%s module always enabled\n", module_name.c_str());
        }
        return true;
    }
}

bool Module::is_disabled(bool verbose) const {
    if (can_be_disabled) {
        if (verbose) {
            Serial.printf("%s module %s\n", module_name.c_str(), enabled ? "enabled" : "disabled");
        }
        return !enabled;
    } else {
        if (verbose) {
            Serial.printf("%s module always enabled\n", module_name.c_str());
        }
        return false;
    }
}

CommandsGroup Module::get_commands_group() {
    commands_group.name     = module_name;
    commands_group.commands = std::span<const Command>(
        commands_storage.data(),
        commands_storage.size()
    );
    return commands_group;
}
