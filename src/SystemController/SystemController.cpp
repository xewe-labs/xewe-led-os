// src/SystemController.cpp
#include "SystemController.h"


SystemController::SystemController()
  : serial_port(*this)
  , nvs(*this)
  , system(*this)
  , command_parser(*this)
  , led_strip(*this)
  , wifi(*this)
  , web(*this)
//  , homekit(*this)
//  , alexa(*this)
{
    modules[0] = &serial_port;
    modules[1] = &nvs;
    modules[3] = &system;
    modules[2] = &command_parser;
    modules[4] = &led_strip;
    modules[5] = &wifi;
    modules[6] = &web;
//    modules[7] = &homekit;
//    modules[8] = &alexa;

    interfaces[0] = &led_strip;
    interfaces[1] = &nvs;
    interfaces[2] = &web;
//    interfaces[3] = homekit;
//    interfaces[4] = alexa;
}

void SystemController::begin() {
    SerialPortConfig serial_cfg;
    serial_port.begin(serial_cfg);

    NvsConfig nvs_cfg;
    nvs.begin(nvs_cfg);

    SystemConfig system_cfg;
    system.begin(system_cfg);

    LedStripConfig led_strip_cfg;
    led_strip.begin(led_strip_cfg);

    WifiConfig wifi_cfg;
    wifi.begin(wifi_cfg);

    WebConfig web_cfg;
    // add here a config element that tells if wifi is enabled
    web.begin(web_cfg);
//
//    HomekitConfig homekit_cfg;
    // add here a config element that tells if wifi is enabled
//    homekit.begin(homekit_cfg);

    // after all interfaces begin() complete, we can sync
//    nvs.sync_from_memory();

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

void SystemController::sync_color(std::array<uint8_t,3> color, std::array<uint8_t,INTERFACE_COUNT> sync_flags) {
    for (int i = 0; i < INTERFACE_COUNT; i++) {

        if (i > 2) return; // temp way to prevent interfaces that are not ready yet

        if (sync_flags[i])
            interfaces[i]->sync_color(color);
    }
}

void SystemController::sync_brightness(uint8_t brightness, std::array<uint8_t,INTERFACE_COUNT> sync_flags) {
    for (int i = 0; i < INTERFACE_COUNT; i++) {

        if (i > 2) return; // temp way to prevent interfaces that are not ready yet

        if (sync_flags[i])
            interfaces[i]->sync_brightness(brightness);
    }
}

void SystemController::sync_state(uint8_t state, std::array<uint8_t,INTERFACE_COUNT> sync_flags) {
    for (int i = 0; i < INTERFACE_COUNT; i++) {

        if (i > 2) return; // temp way to prevent interfaces that are not ready yet

        if (sync_flags[i])
            interfaces[i]->sync_state(state);
    }
}

void SystemController::sync_mode(uint8_t mode, std::array<uint8_t,INTERFACE_COUNT> sync_flags) {
    for (int i = 0; i < INTERFACE_COUNT; i++) {

        if (i > 2) return; // temp way to prevent interfaces that are not ready yet

        if (sync_flags[i])
            interfaces[i]->sync_mode(mode);
    }
}

void SystemController::sync_length(uint16_t length, std::array<uint8_t,INTERFACE_COUNT> sync_flags) {
    for (int i = 0; i < INTERFACE_COUNT; i++) {

        if (i > 2) return; // temp way to prevent interfaces that are not ready yet

        if (sync_flags[i])
            interfaces[i]->sync_length(length);
    }
}

void SystemController::sync_all(std::array<uint8_t,3> color, uint8_t brightness, uint8_t state, uint8_t mode, uint16_t length, std::array<uint8_t,INTERFACE_COUNT> sync_flags) {
    for (int i = 0; i < INTERFACE_COUNT; i++) {

        if (i > 2) return; // temp way to prevent interfaces that are not ready yet

        if (sync_flags[i])
            interfaces[i]->sync_all(color, brightness, state, mode, length);
    }
}

std::string SystemController::get_name() {
    return std::string("Test Lights");
}

