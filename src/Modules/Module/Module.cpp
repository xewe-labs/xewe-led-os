// src/Modules/Module.cpp

#include "Module.h"
#include "../../SystemController/SystemController.h"
#include <Arduino.h>
#include <string>


void Module::begin (const ModuleConfig& cfg) {
    if(requires_init_setup && !init_setup_complete()) {
        init_setup();
        controller.nvs.write_bool(nvs_key, "isc", false);
    }
}

void Module::register_generic_commands() {
    // “status” command
    commands_storage.push_back(Command{
        "status",
        "Get module status",
        std::string("Sample Use: $") + lower(module_name) + " status",
        0,
        [this](std::string) {
            status(true);
        }
    });

    // “reset” command
    commands_storage.push_back(Command{
        "reset",
        "Reset the module",
        std::string("Sample Use: $") + lower(module_name) + " reset",
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
            std::string("Sample Use: $") + lower(module_name) + " enable",
            0,
            [this](std::string) {
                enable(true);
            }
        });
        commands_storage.push_back(Command{
            "disable",
            "Disable this module",
            std::string("Sample Use: $") + lower(module_name) + " disable",
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
    controller.nvs.write_bool(nvs_key, "is_en", true);
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
    controller.nvs.write_bool(nvs_key, "is_en", false);
    return true;
}

std::string Module::status(bool verbose) const {
    std::string status_str = (module_name + " module " + (controller.nvs.read_bool(nvs_key, "is_en") ? "enabled" : "disabled"));
    if (verbose) Serial.printf("%s\n", status_str.c_str());
    return status_str;
}

// only print the debug msg if true
bool Module::is_enabled(bool verbose) const {
    if (can_be_disabled) {
        bool enabled = controller.nvs.read_bool(nvs_key, "is_en");
        if (verbose && enabled) Serial.printf("%s module enabled\n");
        return enabled;
    }
    return true;
}

// only print the debug msg if true
bool Module::is_disabled(bool verbose) const {
    if (can_be_disabled) {
        bool disabled = !controller.nvs.read_bool(nvs_key, "is_en");
        if (verbose && disabled) Serial.printf("%s module disabled; use $%s enable\n", module_name.c_str(), lower(module_name).c_str());
        return disabled;
    }
    return false;
}

bool Module::init_setup_complete (bool verbose) const {
    return !requires_init_setup || !controller.nvs.read_bool(nvs_key, "isc");
}


CommandsGroup Module::get_commands_group() {
    commands_group.name     = module_name;
    commands_group.group     = lower(module_name);
    commands_group.commands = std::span<const Command>(
        commands_storage.data(),
        commands_storage.size()
    );
    return commands_group;
}