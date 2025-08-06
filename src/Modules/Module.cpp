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
        [this](std::string) {
            status(true);
        }
    });

    // “reset” command
    commands_storage.push_back(Command{
        "reset",
        "Reset the module",
        std::string("Sample Use: $") + module_name + " reset",
        0,
        [this](std::string) {
            reset(true);
        }
    });

    // “enable” / “disable” commands (if supported)
    if (can_be_disabled) {
        commands_storage.push_back(Command{
            "enable",
            "Enable this module",
            std::string("Sample Use: $") + module_name + " enable",
            0,
            [this](std::string) {
                enable(true);
            }
        });
        commands_storage.push_back(Command{
            "disable",
            "Disable this module",
            std::string("Sample Use: $") + module_name + " disable",
            0,
            [this](std::string) {
                disable(true);
            }
        });
    }
}

// returns success of the operation
bool Module::enable(bool verbose) {
    if (is_enabled()){
        Serial.printf("%s module already enabled\n", module_name.c_str());
        return false;
    }

    enabled = true;
    if (verbose) Serial.printf("%s module enabled\n", module_name.c_str());
    return true;
}

bool Module::disable(bool verbose) {
    if (is_disabled()){
        if (verbose) Serial.printf("%s module already disabled\n", module_name.c_str());
        return false;
    }
    if (!can_be_disabled) {
        if (verbose) Serial.printf("%s module can't be disabled\n", module_name.c_str());
        return false;
    }
    if (verbose) Serial.printf("%s module disabled\n", module_name.c_str());
    enabled = false;
    return true;
}

std::string Module::status(bool verbose) const {
    if (verbose) Serial.printf("ok\n");
    return std::string("ok");
}

// only print the debug msg if true
bool Module::is_enabled(bool verbose) const {
    if (can_be_disabled) {
        if (verbose && enabled) Serial.printf("%s module enabled\n");
        return enabled;
    }
    return true;
}

// only print the debug msg if true
bool Module::is_disabled(bool verbose) const {
    if (can_be_disabled) {
        if (verbose && !enabled) Serial.printf("%s module disabled\n", module_name.c_str());
        return !enabled;
    }
    return false;
}

CommandsGroup Module::get_commands_group() {
    commands_group.name     = module_name;
    commands_group.commands = std::span<const Command>(
        commands_storage.data(),
        commands_storage.size()
    );
    return commands_group;
}
