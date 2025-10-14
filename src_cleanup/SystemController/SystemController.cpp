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
  , homekit(*this)
  , alexa(*this)
{
    modules[0] = &serial_port;
    modules[1] = &nvs;
    modules[2] = &system;
    modules[3] = &command_parser;
    modules[4] = &led_strip;
    modules[5] = &wifi;
    modules[6] = &web;
    modules[7] = &homekit;
    modules[8] = &alexa;

    interfaces[0] = &led_strip;
    interfaces[1] = &nvs;
    interfaces[2] = &web;
    interfaces[3] = &homekit;
    interfaces[4] = &alexa;
}

bool SystemController::begin() {
    SerialPortConfig serial_cfg;
    serial_port.begin(serial_cfg);

    NvsConfig nvs_cfg;
    nvs.begin(nvs_cfg);

    bool init_setup_flag = !system.init_setup_complete();
    SystemConfig system_cfg;
    system.begin(system_cfg);

    LedStripConfig led_strip_cfg;
    led_strip.begin(led_strip_cfg);

    WifiConfig wifi_cfg;
    wifi.begin(wifi_cfg);

    WebConfig web_cfg;
    web_cfg.requirements[0] = &wifi;
    web.begin(web_cfg);

    HomekitConfig homekit_cfg;
    homekit_cfg.requirements[0] = &wifi;
    homekit.begin(homekit_cfg);

    AlexaConfig alexa_cfg;
    alexa_cfg.requirements[0] = &wifi;
    alexa_cfg.requirements[1] = &web;
    alexa_cfg.server = web.get_server();
    alexa.begin(alexa_cfg);

    if (init_setup_flag) {
        serial_port.println(xewe::str::generate_split_line(50, '-', "+"));
        serial_port.println(xewe::str::center_text("Initial Setup Complete!", 50));
        serial_port.println(xewe::str::generate_split_line(50, '-', "+"));
        serial_port.println(xewe::str::center_text("Rebooting...", 50));
        serial_port.println(xewe::str::generate_split_line(50, '-', "+"));
        delay(3000);
        ESP.restart();
    }

    web.begin_server();
    nvs.sync_from_memory({true, false, true, true, true});

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

void SystemController::sync_color(std::array<uint8_t,3> color, std::array<uint8_t,INTERFACE_COUNT>& sync_flags) {
    for_each_interface(sync_flags, [&](auto& interface){ interface.sync_color(color); });
}

void SystemController::sync_brightness(uint8_t brightness, std::array<uint8_t,INTERFACE_COUNT>& sync_flags) {
    for_each_interface(sync_flags, [&](auto& interface){ interface.sync_brightness(brightness); });
}

void SystemController::sync_state(uint8_t state, std::array<uint8_t,INTERFACE_COUNT>& sync_flags) {
    for_each_interface(sync_flags, [&](auto& interface){ interface.sync_state(state); });
}

void SystemController::sync_mode(uint8_t mode, std::array<uint8_t,INTERFACE_COUNT>& sync_flags) {
    for_each_interface(sync_flags, [&](auto& interface){ interface.sync_mode(mode); });
}

void SystemController::sync_length(uint16_t length, std::array<uint8_t,INTERFACE_COUNT>& sync_flags) {
    for_each_interface(sync_flags, [&](auto& interface){ interface.sync_length(length); });
}

void SystemController::sync_all(std::array<uint8_t,3> color,
                                uint8_t brightness,
                                uint8_t state, 
                                uint8_t mode, 
                                uint16_t length,
                                std::array<uint8_t,INTERFACE_COUNT>& sync_flags) {
    for_each_interface(sync_flags, [&](auto& interface){ interface.sync_all(color, brightness, state, mode, length); });
}

template <typename Fn>
void SystemController::for_each_interface(const std::array<uint8_t, INTERFACE_COUNT>& sync_flags, Fn&& fn) {
    for (std::size_t i = 0; i < INTERFACE_COUNT; ++i) {
        if (sync_flags[i] && interfaces[i]) {
            std::forward<Fn>(fn)(*interfaces[i]);
        }
    }
}


