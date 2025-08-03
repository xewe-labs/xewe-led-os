// src/SystemController.cpp
#include "SystemController.h"

SystemController::SystemController()
  : serial_port(*this)
  , command_parser(*this)
  , wifi(*this)
{
    modules[0] = &serial_port;
    modules[1] = &command_parser;
    modules[2] = &wifi;
}

void SystemController::begin() {
    // 1) Serial port (no special config)
    ModuleConfig default_cfg;
    serial_port.begin(default_cfg);

    // 2) WiFi module
    WifiConfig wifi_cfg;
    // wifi_cfg.hostname = "MyCustomHost";
    wifi.begin(wifi_cfg);

    // 3) Command parser gets the WiFi command group
    ParserConfig parser_cfg;
    parser_cfg.groups      = &wifi.get_command_group();
    parser_cfg.group_count = 1;
    command_parser.begin(parser_cfg);
}

void SystemController::loop() {
    for (auto module : modules) {
        module->loop();
    }

//    if (serial_port.has_line()) {
//        command_parser.parse(serial_port.read_line().c_str());
//    }
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

const char* SystemController::status() const {
    return enabled ? "enabled" : "disabled";
}

void SystemController::enable_module(const char* module_name) {
    if (std::strcmp(module_name, "command_parser") == 0) {
        command_parser.enable();
    } else if (std::strcmp(module_name, "wifi") == 0) {
        wifi.enable();
    }
}

void SystemController::disable_module(const char* module_name) {
    if (std::strcmp(module_name, "command_parser") == 0) {
        command_parser.disable();
    } else if (std::strcmp(module_name, "wifi") == 0) {
        wifi.disable();
    }
}

void SystemController::reset_module(const char* module_name) {
    if (std::strcmp(module_name, "command_parser") == 0) {
        command_parser.reset();
    } else if (std::strcmp(module_name, "wifi") == 0) {
        wifi.reset();
    }
}

const char* SystemController::module_status(const char* module_name) const {
    if (std::strcmp(module_name, "command_parser") == 0) {
        return command_parser.status().data();
    } else if (std::strcmp(module_name, "serial_port") == 0) {
        return serial_port.status().data();
    } else if (std::strcmp(module_name, "wifi") == 0) {
        return wifi.status().data();
    }
    return nullptr;
}

void SystemController::module_print_help(const char* module_name) {
    command_parser.print_help(module_name);
}