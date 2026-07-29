// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Module/ModuleController.cpp

#include "ModuleController.h"


ModuleController::ModuleController()
  : serial_port(*this)
  , nvs(*this)
  , system(*this)
  , command_executor(*this)
  , wifi(*this)
  , web_interface(*this)
  , buttons(*this)
  , pins(*this)
{
    register_module(serial_port);
    register_module(nvs);
    register_module(system);
    register_module(command_executor);
    register_module(wifi);
    register_module(web_interface);
    register_module(buttons);
    register_module(pins);

#if COMPILE_TESTS
    owned_modules.push_back(std::make_unique<NvsTester>(*this));
    register_module(*owned_modules.back());

    owned_modules.push_back(std::make_unique<NvsFlexTester>(*this));
    register_module(*owned_modules.back());
#endif
}

void ModuleController::begin() {

    serial_port.begin               (SerialPortConfig       {});
    nvs.begin                       (NvsConfig              {});

    const bool init_setup_flag =    !nvs.read<bool>("root", "init_setup_flag");

    system.begin                    (SystemConfig           {});
    command_executor.begin          (CommandExecutorConfig  {});

    wifi.begin                      (WifiConfig             {});

    web_interface.add_requirement   (wifi);
    web_interface.begin             (WebInterfaceConfig     {});

    buttons.begin                   (ButtonsConfig          {});
    pins.begin                      (PinsConfig             {});

    if (init_setup_flag) {
        serial_port.print_header("Initial Setup Complete");
        nvs.write<bool>("root", "init_setup_flag", true);
        system.restart();
    }

    serial_port.print_header("System Setup Complete");
}

void ModuleController::loop() {
    for (auto& [id, module] : modules) {
        if (module->is_enabled()) {
            module->loop();
        }
    }
}

bool ModuleController::register_module(Module& module) {
    auto [it, inserted] = modules.emplace(
        std::string(module.get_id()),
        &module
    );

    return inserted;
}

Module* ModuleController::get_module(std::string_view id) {
    auto it = modules.find(std::string(id));

    if (it == modules.end()) return nullptr;

    return it->second;
}

void ModuleController::send_command(std::span<const std::string> recipients,
                                    std::string_view command_name,
                                    std::span<const std::string> args
                                   ) {
    for (const std::string& recipient_id : recipients) {
        Module* recipient = get_module(recipient_id);

        if (recipient == nullptr) continue;

        for (const Command& command : recipient->get_commands()) {
            if (command.name != command_name)       { continue; }
            if (!command.function)                  { continue; }
            if (args.size() != command.arg_count)   { continue; }

            command.function(args);
            break;
        }
    }
}