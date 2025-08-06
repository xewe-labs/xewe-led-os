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
            Serial.printf("%s status:\n", module_name.c_str());
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
            Serial.printf("%s reset\n", module_name.c_str());
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
                Serial.printf("%s enabled\n", module_name.c_str());
                enable();
            }
        });
        commands_storage.push_back(Command{
            "disable",
            "Disable this module",
            std::string("Sample Use: $") + module_name + " disable",
            0,
            [this](std::string_view) {
                Serial.printf("%s disabled\n", module_name.c_str());
                disable();
            }
        });
    }
}

std::string_view Module::status(bool print) const {
    if (print) {
        Serial.printf("ok");
    }
    return "ok";
}

bool Module::is_enabled(bool verbose) const {
    if (verbose) {
        if (can_be_disabled) { Serial.printf("%s module %s", module_name.c_str(), enabled ? "enabled" : "disabled"); }
        else { Serial.printf("%s module always enabled", module_name.c_str()); }
    }
    return !can_be_disabled || enabled;
}

bool Module::is_disabled(bool verbose) const {
    if (verbose) {
        if (can_be_disabled) { Serial.printf("%s module %s", module_name.c_str(), enabled ? "enabled" : "disabled"); }
        else { Serial.printf("%s module always enabled", module_name.c_str()); }
    }
    return can_be_disabled || !enabled;
}
