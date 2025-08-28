// src/SystemController.cpp
#include "SystemController.h"


SystemController::SystemController()
  : nvs(*this)
  , serial_port(*this)
  , command_parser(*this)
  , system(*this)
  , led_strip(*this)
  , wifi(*this)
  , web(*this)
{
    modules[0] = &serial_port;
    modules[1] = &nvs;
    modules[2] = &command_parser;
    modules[3] = &system;
    modules[4] = &led_strip;
    modules[5] = &wifi;
    modules[6] = &web;
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
    wifi.connect(true);

    WebConfig web_cfg;
    web.begin(web_cfg);

    // after all interfaces begin() complete, we can sync
    nvs.sync_from_memory();

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

void SystemController::sync_color(std::array<uint8_t,3> color, std::array<uint8_t,5> sync_flags) {
    if (sync_flags[0]) led_strip.sync_color(color);
    if (sync_flags[1]) nvs.sync_color(color);
    if (sync_flags[2]) web.sync_color(color);

//    if (sync_flags[3]) homekit.sync_color(color);
//    if (sync_flags[4]) alexa.sync_color(color);
}

void SystemController::sync_brightness(uint8_t brightness, std::array<uint8_t,5> sync_flags) {
    if (sync_flags[0]) led_strip.sync_brightness(brightness);
    if (sync_flags[1]) nvs.sync_brightness(brightness);
    if (sync_flags[2]) web.sync_brightness(brightness);
//    if (sync_flags[3]) homekit.sync_brightness(brightness);
//    if (sync_flags[4]) alexa.sync_brightness(brightness);
}

void SystemController::sync_state(uint8_t state, std::array<uint8_t,5> sync_flags) {
    if (sync_flags[0]) led_strip.sync_state(state);
    if (sync_flags[1]) nvs.sync_state(state);
    if (sync_flags[2]) web.sync_state(state);
//    if (sync_flags[3]) homekit.sync_state(state);
//    if (sync_flags[4]) alexa.sync_state(state);
}

void SystemController::sync_mode(uint8_t mode, std::array<uint8_t,5> sync_flags) {
    if (sync_flags[0]) led_strip.sync_mode(mode);
    if (sync_flags[1]) nvs.sync_mode(mode);
    if (sync_flags[2]) web.sync_mode(mode);
//    if (sync_flags[3]) homekit.sync_mode(mode);
//    if (sync_flags[4]) alexa.sync_mode(mode);
}

void SystemController::sync_length(uint16_t length, std::array<uint8_t,5> sync_flags) {
    if (sync_flags[0]) led_strip.sync_length(length);
    if (sync_flags[1]) nvs.sync_length(length);
    if (sync_flags[2]) web.sync_length(length);
//    if (sync_flags[3]) homekit.sync_length(length);
//    if (sync_flags[4]) alexa.sync_length(length);
}

void SystemController::sync_all(std::array<uint8_t,3> color, uint8_t brightness, uint8_t state, uint8_t mode, uint16_t length, std::array<uint8_t,5> sync_flags) {
    sync_mode           (mode, sync_flags);
    sync_color          (color, sync_flags);
    sync_brightness     (brightness, sync_flags);
    sync_state          (state, sync_flags);
    sync_length         (length, sync_flags);
}