// src/Modules/Module.cpp

#include "Module.h"
#include "../SystemController.h"
#include <Arduino.h>  // for Serial.printf

void Module::add_generic_commands() {
    commands_storage.push_back({
        "help",
        "Show this module’s help",
        "",
        0,
        [this](std::string_view) {
            this->controller.module_print_help(module_name);
        }
    });
    commands_storage.push_back({
        "status",
        "Get module status",
        "",
        0,
        [this](std::string_view) {
            Serial.printf("%s status: %s\n",
                          module_name.c_str(),
                          status().data());
        }
    });
    commands_storage.push_back({
        "enable",
        "Enable this module",
        "",
        0,
        [this](std::string_view) {
            enable();
            Serial.printf("%s enabled\n", module_name.c_str());
        }
    });
    commands_storage.push_back({
        "disable",
        "Disable this module",
        "",
        0,
        [this](std::string_view) {
            disable();
            Serial.printf("%s disabled\n", module_name.c_str());
        }
    });
    commands_storage.push_back({
        "reset",
        "Reset this module",
        "",
        0,
        [this](std::string_view) {
            reset();
            Serial.printf("%s reset\n", module_name.c_str());
        }
    });
}
