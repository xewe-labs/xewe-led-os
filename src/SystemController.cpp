// src/SystemController.cpp

#include "SystemController.h"

#include <span>
#include <vector>
#include <functional>

namespace {
    // will be set in ctor
    SystemController* g_self = nullptr;

    // Single no-arg command that prints every command
    Command help_commands[] = {
        {
            /* name        */ "",
            /* desc        */ "Print all commands",
            /* usage       */ "",
            /* arg_count   */ 0,
            /* function    */ [](std::string_view) {
                if (g_self) g_self->print_all_commands();
            }
        }
    };
    CommandsGroup help_group = {
        /* name     */ "help",
        /* commands */ std::span<const Command>(help_commands, 1)
    };
}

SystemController::SystemController()
  : serial_port(*this)
  , command_parser(*this)
  , wifi(*this)
{
    // allow help-lambda to call back into us
    g_self = this;

    // Register modules
    modules[0] = &serial_port;
    modules[1] = &command_parser;
    modules[2] = &wifi;

    // Route serial input lines to command parser
    serial_port.set_line_callback([this](std::string_view line) {
        // Pass C-string view directly into parser
        command_parser.parse(line);
    });
}

void SystemController::begin() {
    // 1) Serial port
    ModuleConfig default_cfg;
    serial_port.begin(default_cfg);

    // 2) WiFi
    WifiConfig wifi_cfg;
    wifi.begin(wifi_cfg);

    // 3) Command parser: help + wifi
    static CommandsGroup groups[2];
    groups[0] = help_group;
    groups[1] = wifi.get_command_group();

    ParserConfig parser_cfg;
    parser_cfg.groups      = groups;
    parser_cfg.group_count = 2;
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

void SystemController::print_all_commands() {
    command_parser.print_all_commands();
}
