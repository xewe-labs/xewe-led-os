// src/SystemController.cpp

#include "SystemController.h"

SystemController::SystemController()
  : serial_port(*this)
  , command_parser(*this)
  , wifi(*this)
{
    // Register modules
    modules[0] = &serial_port;
    modules[1] = &command_parser;
    modules[2] = &wifi;

    // Route serial input lines to command parser
    serial_port.set_line_callback([this](std::string_view line) {
        command_parser.parse(line);
    });
}

void SystemController::begin() {
    // 1) Serial port (no special config)
    ModuleConfig default_cfg;
    serial_port.begin(default_cfg);

    // 2) WiFi module
    WifiConfig wifi_cfg;
    wifi.begin(wifi_cfg);

    static CommandsGroup groups[] = {
        wifi.get_command_group()
    };

    ParserConfig parser_cfg;
    parser_cfg.groups      = groups;
    parser_cfg.group_count = 1;
    command_parser.begin(parser_cfg);
}

void SystemController::loop() {
    for (auto module : modules) {
        module->loop();
    }
}

void SystemController::enable() {
    enabled = true;
    for (auto module : modules) {
        module->enable();
    }
}

void SystemController::disable() {
    enabled = false;
    for (auto module : modules) {
        module->disable();
    }
}

void SystemController::reset() {
    for (auto module : modules) {
        module->reset();
    }
}

std::string_view SystemController::status() const {
    return enabled ? "enabled" : "disabled";
}

void SystemController::enable_module(std::string_view module_name) {
    if (module_name == "command_parser") {
        command_parser.enable();
    } else if (module_name == "wifi") {
        wifi.enable();
    }
}

void SystemController::disable_module(std::string_view module_name) {
    if (module_name == "command_parser") {
        command_parser.disable();
    } else if (module_name == "wifi") {
        wifi.disable();
    }
}

void SystemController::reset_module(std::string_view module_name) {
    if (module_name == "command_parser") {
        command_parser.reset();
    } else if (module_name == "wifi") {
        wifi.reset();
    }
}

std::string_view SystemController::module_status(std::string_view module_name) const {
    if (module_name == "command_parser") {
        return command_parser.status();
    } else if (module_name == "serial_port") {
        return serial_port.status();
    } else if (module_name == "wifi") {
        return wifi.status();
    }
    return {};
}

void SystemController::module_print_help(std::string_view module_name) {
    command_parser.print_help(std::string(module_name));
}