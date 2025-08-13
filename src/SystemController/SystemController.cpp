// src/SystemController.cpp
#include "SystemController.h"


SystemController::SystemController()
  : nvs(*this)
  , serial_port(*this)
  , command_parser(*this)
  , led_strip(*this)
  , wifi(*this)
{
    modules[0] = &serial_port;
    modules[1] = &nvs;
    modules[2] = &command_parser;
    modules[3] = &led_strip;
    modules[4] = &wifi;
}

void SystemController::begin() {
    SerialPortConfig serial_cfg;
    serial_port.begin(serial_cfg);

    NvsConfig nvs_cfg;
    nvs.begin(nvs_cfg);

    LedStripConfig led_strip_cfg;
    led_strip.begin(led_strip_cfg);

    WifiConfig wifi_cfg;
    wifi.begin(wifi_cfg);

    command_groups.clear();
    for (auto module : modules) {
        auto grp = module->get_commands_group();
        if (!grp.commands.empty()) {
            command_groups.push_back(grp);
        }
    }

    ParserConfig parser_cfg;
    parser_cfg.groups      = command_groups.data();
    parser_cfg.group_count = command_groups.size();
    command_parser.begin(parser_cfg);
}

void SystemController::loop() {
    for (size_t i = 0; i < MODULE_COUNT; ++i) {
        modules[i]->loop();
    }
    if (serial_port.has_line()) {
        command_parser.parse(serial_port.read_line());
    }
}

void SystemController::reset() {
    for (size_t i = 0; i < MODULE_COUNT; ++i) {
        modules[i]->reset();
    }
}

std::string SystemController::status() const {
    return std::string("ok");
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

std::string SystemController::module_status(std::string_view module_name) const {
    for (auto module : modules) {
        if (module->get_commands_group().name == module_name) {
            return module->status();
        }
    }
    return std::string("unknown");
}

void SystemController::module_print_help(std::string_view module_name) {
    command_parser.print_help(std::string(module_name));
}
