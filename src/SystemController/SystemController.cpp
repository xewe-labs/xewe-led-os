/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
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
  , time(*this)
  , scheduler(*this)
  , web_interface(*this)
  , homekit(*this)
  , alexa(*this)
  , buttons(*this)
{
    modules.push_back(&serial_port);
    modules.push_back(&nvs);
    modules.push_back(&system);
    modules.push_back(&command_parser);
    modules.push_back(&led_strip);
    modules.push_back(&wifi);
    modules.push_back(&time);
    modules.push_back(&scheduler);
    modules.push_back(&web_interface);
    modules.push_back(&homekit);
    modules.push_back(&alexa);
    modules.push_back(&buttons);

    interfaces.push_back(&led_strip);
    interfaces.push_back(&nvs);
    interfaces.push_back(&web_interface);
    interfaces.push_back(&homekit);
    interfaces.push_back(&alexa);
}

void SystemController::begin() {
    bool init_setup_flag = !nvs.read_bool("root", "init_setup_flag");

    serial_port.begin               (SerialPortConfig       {});
    nvs.begin                       (NvsConfig              {});
    system.begin                    (SystemConfig           {});
    led_strip.begin                 (LedStripConfig         {});
    wifi.begin                      (WifiConfig             {});
    scheduler.add_requirement       (wifi);
    time.begin                      (TimeConfig             {});
    scheduler.add_requirement       (time);
    scheduler.add_requirement       (wifi);
    scheduler.begin                 (SchedulerConfig        {});

    web_interface.add_requirement   (wifi);
    web_interface.begin             (WebInterfaceConfig     {});
    homekit.add_requirement         (wifi);
    homekit.begin                   (HomeKitConfig          {});
    alexa.add_requirement           (wifi);
    alexa.add_requirement           (web_interface);
    alexa.begin                     (HomeKitConfig          {});
    buttons.begin                   (ButtonsConfig          {});

    command_parser.begin            (CommandParserConfig    {});

    if (init_setup_flag) {
        serial_port.print_header("Initial Setup Complete");
        nvs.write_bool("root", "init_setup_flag", true);
        system.restart();
    }
    nvs.sync_from_memory({false, false, true, true, true});

    serial_port.print_header("System Setup Complete");
}

void SystemController::loop() {
    for (Module* m : modules) {
        if (m && m->is_enabled())
            m->loop();
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
