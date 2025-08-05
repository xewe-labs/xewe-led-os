// src/SystemController.cpp
#include "SystemController.h"
#include "Interfaces/Hardware/Nvs/Nvs.h"
#include <Arduino.h>
#include <string>

SystemController::SystemController()
  : nvs(*this)
  , serial_port(*this)
  , command_parser(*this)
  , wifi(*this)
{
    modules[0] = &nvs;
    modules[1] = &serial_port;
    modules[2] = &command_parser;
    modules[3] = &wifi;

    serial_port.set_line_callback([this](std::string_view line) {
        command_parser.parse(line);
    });
}

void SystemController::begin() {
    // Initialize NVS namespace first
    NvsConfig nvs_cfg;
    nvs.begin(nvs_cfg);

    // Then serial port
    SerialPortConfig serial_cfg;
    serial_port.begin(serial_cfg);

    // Then Wi-Fi
    WifiConfig wifi_cfg;
    wifi.begin(wifi_cfg);

    // Build CLI command groups...
    command_groups.clear();
    for (auto module : modules) {
        auto grp = module->get_commands_group();
        if (!grp.commands.empty()) {
            command_groups.push_back(grp);
        }
    }

    // Hand off to the command parser
    ParserConfig parser_cfg;
    parser_cfg.groups      = command_groups.data();
    parser_cfg.group_count = command_groups.size();
    command_parser.begin(parser_cfg);
}

// -----------------------------------------------------------------------------
// Missing definitions for SystemController declared methods
// -----------------------------------------------------------------------------

void SystemController::loop() {
    for (size_t i = 0; i < MODULE_COUNT; ++i) {
        modules[i]->loop();
    }
}

void SystemController::reset() {
    for (size_t i = 0; i < MODULE_COUNT; ++i) {
        modules[i]->reset();
    }
}

std::string_view SystemController::status() const {
    return enabled ? "enabled" : "disabled";
}

void SystemController::module_enable(std::string_view module_name) {
    for (auto module : modules) {
        if (module->get_commands_group().name == module_name) {
            module->enable();
            return;
        }
    }
    Serial.printf("Error: Module '%s' not found\n", module_name.data());
}

void SystemController::module_disable(std::string_view module_name) {
    for (auto module : modules) {
        if (module->get_commands_group().name == module_name) {
            module->disable();
            return;
        }
    }
    Serial.printf("Error: Module '%s' not found\n", module_name.data());
}

void SystemController::module_reset(std::string_view module_name) {
    for (auto module : modules) {
        if (module->get_commands_group().name == module_name) {
            module->reset();
            return;
        }
    }
    Serial.printf("Error: Module '%s' not found\n", module_name.data());
}

std::string_view SystemController::module_status(std::string_view module_name) const {
    for (auto module : modules) {
        if (module->get_commands_group().name == module_name) {
            return module->status();
        }
    }
    return "unknown";
}

void SystemController::module_print_help(std::string_view module_name) {
    // Delegate to the command parser
    command_parser.print_help(std::string(module_name));
}
