// src/SystemController.cpp
#include "SystemController.h"
#include "Interfaces/Hardware/Nvs/Nvs.h"

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
    if (module_name == "nvs") {
        nvs.enable();
    } else if (module_name == "command_parser") {
        command_parser.enable();
    } else if (module_name == "wifi") {
        wifi.enable();
    }
}

void SystemController::disable_module(std::string_view module_name) {
    if (module_name == "nvs") {
        nvs.disable();
    } else if (module_name == "command_parser") {
        command_parser.disable();
    } else if (module_name == "wifi") {
        wifi.disable();
    }
}

void SystemController::reset_module(std::string_view module_name) {
    if (module_name == "nvs") {
        nvs.reset();
    } else if (module_name == "command_parser") {
        command_parser.reset();
    } else if (module_name == "wifi") {
        wifi.reset();
    }
}

std::string_view SystemController::module_status(std::string_view module_name) const {
    if (module_name == "nvs") {
        // Nvs::status() currently prints via DBG_PRINTLN;
        // returning empty or could wrap in a string if implemented.
        return "";
    } else if (module_name == "command_parser") {
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
