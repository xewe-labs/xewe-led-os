/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/



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
//  , buttons(*this)
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
//    modules[8] = &buttons;

    interfaces[0] = &led_strip;
    interfaces[1] = &nvs;
    interfaces[2] = &web;
    interfaces[3] = &homekit;
    interfaces[4] = &alexa;
}

void SystemController::begin() {
    bool init_setup_flag = !system.init_setup_complete();

    serial_port.begin           (SerialPortConfig   {});
    nvs.begin                   (NvsConfig          {});
    system.begin                (SystemConfig       {});
    led_strip.begin             (LedStripConfig     {});
    wifi.begin                  (WifiConfig         {});
    web.add_requirement         (wifi                 );
    web.begin                   (WebConfig          {});
    homekit.add_requirement     (wifi                 );
    homekit.begin               (HomekitConfig      {});
    alexa.add_requirement       (wifi                 );
    alexa.add_requirement       (web                  );
    alexa.begin                 (AlexaConfig        {});
//    buttons.begin               (ButtonsConfig        {});

    if (init_setup_flag) {
        serial_port.print_spacer();
        serial_port.print_centered("Initial Setup Complete", 50);
        serial_port.print_spacer();
        serial_port.print_centered("Rebooting...");
        serial_port.print_spacer();
        delay(3000);
        ESP.restart();
    }

    nvs.sync_from_memory({false, false, true, true, true});

    // this can be moved inside of the module begin
    command_groups.clear();
    for (auto module : modules) {
        auto grp = module->get_commands_group();
        if (!grp.commands.empty()) {
            command_groups.push_back(grp);
        }
    }

    CommandParserConfig parser_cfg;
    parser_cfg.groups      = command_groups.data();
    parser_cfg.group_count = command_groups.size();
    // this can be moved inside of the module begin
    command_parser.begin(parser_cfg);

    serial_port.print_spacer();
    serial_port.print_centered("System Setup Complete", 50);
    serial_port.print_spacer();
}

void SystemController::loop() {
    for (size_t i = 0; i < MODULE_COUNT; ++i) {
        modules[i]->loop();
    }
    if (serial_port.has_line()) {
        command_parser.parse(serial_port.read_line());
    }
}

void SystemController::sync_color(std::array<uint8_t,3> color, const std::array<uint8_t,INTERFACE_COUNT>& sync_flags) {
    for_each_interface(sync_flags, [&](auto& interface){ interface.sync_color(color); });
}

void SystemController::sync_brightness(uint8_t brightness, const std::array<uint8_t,INTERFACE_COUNT>& sync_flags) {
    for_each_interface(sync_flags, [&](auto& interface){ interface.sync_brightness(brightness); });
}

void SystemController::sync_state(uint8_t state, const std::array<uint8_t,INTERFACE_COUNT>& sync_flags) {
    for_each_interface(sync_flags, [&](auto& interface){ interface.sync_state(state); });
}

void SystemController::sync_mode(uint8_t mode, const std::array<uint8_t,INTERFACE_COUNT>& sync_flags) {
    for_each_interface(sync_flags, [&](auto& interface){ interface.sync_mode(mode); });
}

void SystemController::sync_length(uint16_t length, const std::array<uint8_t,INTERFACE_COUNT>& sync_flags) {
    for_each_interface(sync_flags, [&](auto& interface){ interface.sync_length(length); });
}

void SystemController::sync_all(std::array<uint8_t,3> color,
                                uint8_t brightness,
                                uint8_t state,
                                uint8_t mode,
                                uint16_t length,
                                const std::array<uint8_t,INTERFACE_COUNT>& sync_flags) {
    for_each_interface(sync_flags, [&](auto& interface){ interface.sync_all(color, brightness, state, mode, length); });
}
