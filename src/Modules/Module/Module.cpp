#include "Module.h"
#include "../../SystemController/SystemController.h"
#include <Arduino.h>
#include <string>


bool Module::init_setup (bool verbose, bool enable_prompt, bool reboot_after) { return true; }

void Module::begin (const ModuleConfig& cfg) {
    DBG_PRINTF(Module, "'%s'->begin(): Called.\n", module_name.c_str());
    if(requires_init_setup && !init_setup_complete()) {
        DBG_PRINTLN(Module, "begin(): Module requires initial setup and it is not yet complete. Calling init_setup().");

        bool enabled = true;
        if (can_be_disabled) {
            enabled = controller.serial_port.prompt_user_yn(std::string("Would you like to enable ") + lower(module_name) + " module?");
        }
        controller.nvs.write_bool(nvs_key, "is_en", enabled);

        if (enabled) {
            init_setup();
        } else {
            controller.serial_port.println(std::string("You can enable it later using $") + lower(module_name) + " enable");
        }
        controller.nvs.write_bool(nvs_key, "isc", true);
        DBG_PRINTLN(Module, "begin(): init_setup() finished. Setting 'isc' flag to false in NVS.");
    }
}

void Module::register_generic_commands() {
    DBG_PRINTF(Module, "'%s'->register_generic_commands(): Called.\n", module_name.c_str());
    // “status” command
    DBG_PRINTLN(Module, "register_generic_commands(): Registering 'status' command.");
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
    DBG_PRINTLN(Module, "register_generic_commands(): Registering 'reset' command.");
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
        DBG_PRINTLN(Module, "register_generic_commands(): Module can be disabled, registering 'enable'/'disable' commands.");
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
    DBG_PRINTF(Module, "'%s'->enable(verbose=%s): Called.\n", module_name.c_str(), verbose ? "true" : "false");
    if (is_enabled()){
        DBG_PRINTLN(Module, "enable(): Module is already enabled.");
        Serial.printf("%s module already enabled\n", module_name.c_str());
        return false;
    }
    DBG_PRINTLN(Module, "enable(): Writing 'is_en'=true to NVS.");
    controller.nvs.write_bool(nvs_key, "is_en", true);
    if (verbose) Serial.printf("%s module enabled\n", module_name.c_str());
    return true;
}

bool Module::disable(bool verbose) {
    DBG_PRINTF(Module, "'%s'->disable(verbose=%s): Called.\n", module_name.c_str(), verbose ? "true" : "false");
    if (is_disabled()){
        DBG_PRINTLN(Module, "disable(): Module is already disabled.");
        if (verbose) Serial.printf("%s module already disabled\n", module_name.c_str());
        return false;
    }
    if (!can_be_disabled) {
        DBG_PRINTLN(Module, "disable(): Module cannot be disabled.");
        if (verbose) Serial.printf("%s module can't be disabled\n", module_name.c_str());
        return false;
    }
    if (verbose) Serial.printf("%s module disabled\n", module_name.c_str());
    DBG_PRINTLN(Module, "disable(): Writing 'is_en'=false to NVS.");
    controller.nvs.write_bool(nvs_key, "is_en", false);
    return true;
}

std::string Module::status(bool verbose) const {
    DBG_PRINTF(Module, "'%s'->status(verbose=%s): Called.\n", module_name.c_str(), verbose ? "true" : "false");
    std::string status_str = (module_name + " module " + (controller.nvs.read_bool(nvs_key, "is_en") ? "enabled" : "disabled"));
    DBG_PRINTF(Module, "status(): Generated status string: '%s'.\n", status_str.c_str());
    if (verbose) Serial.printf("%s\n", status_str.c_str());
    return status_str;
}

// only print the debug msg if true
bool Module::is_enabled(bool verbose) const {
    DBG_PRINTF(Module, "'%s'->is_enabled(verbose=%s): Called.\n", module_name.c_str(), verbose ? "true" : "false");
    if (can_be_disabled) {
        bool enabled = controller.nvs.read_bool(nvs_key, "is_en");
        DBG_PRINTF(Module, "is_enabled(): Module can be disabled, read NVS 'is_en' flag as %s.\n", enabled ? "true" : "false");
        if (verbose && enabled) Serial.printf("%s module enabled\n");
        return enabled;
    }
    DBG_PRINTLN(Module, "is_enabled(): Module cannot be disabled, returning true by default.");
    return true;
}

// only print the debug msg if true
bool Module::is_disabled(bool verbose) const {
    DBG_PRINTF(Module, "'%s'->is_disabled(verbose=%s): Called.\n", module_name.c_str(), verbose ? "true" : "false");
    if (can_be_disabled) {
        bool disabled = !controller.nvs.read_bool(nvs_key, "is_en");
        DBG_PRINTF(Module, "is_disabled(): Module can be disabled, read NVS 'is_en' flag as %s, so disabled state is %s.\n", !disabled ? "true" : "false", disabled ? "true" : "false");
        if (verbose && disabled) Serial.printf("%s module disabled; use $%s enable\n", module_name.c_str(), lower(module_name).c_str());
        return disabled;
    }
    DBG_PRINTLN(Module, "is_disabled(): Module cannot be disabled, returning false by default.");
    return false;
}

bool Module::init_setup_complete (bool verbose) const {
    DBG_PRINTF(Module, "'%s'->init_setup_complete(verbose=%s): Called.\n", module_name.c_str(), verbose ? "true" : "false");
    bool isc_flag = controller.nvs.read_bool(nvs_key, "isc");
    bool result = !requires_init_setup || isc_flag;
    DBG_PRINTF(Module, "init_setup_complete(): requires_init_setup=%s, nvs 'isc' flag=%s. Final result=%s\n",
        requires_init_setup ? "true" : "false",
        isc_flag ? "true" : "false",
        result ? "true" : "false"
    );
    return result;
}

CommandsGroup Module::get_commands_group() {
    DBG_PRINTF(Module, "'%s'->get_commands_group(): Called.\n", module_name.c_str());
    commands_group.name     = module_name;
    commands_group.group     = lower(module_name);
    commands_group.commands = std::span<const Command>(
        commands_storage.data(),
        commands_storage.size()
    );
    DBG_PRINTF(Module, "get_commands_group(): Returning command group '%s' with %zu commands.\n", commands_group.name.c_str(), commands_storage.size());
    return commands_group;
}
