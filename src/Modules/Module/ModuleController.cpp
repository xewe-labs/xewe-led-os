// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Module/ModuleController.cpp

#include "ModuleController.h"


ModuleController::ModuleController()
    : serial_port(*this)
    , nvs(*this)
    , system(*this)
    , command_executor(*this)
    , led_strip(*this)
    , wifi(*this)
    , web_interface(*this)
    , homeassistant(*this)
    , homekit(*this)
    , alexa(*this)
    , time(*this)
    , scheduler(*this)
    , buttons(*this)
{
    register_module(serial_port);
    register_module(nvs);
    register_module(system);
    register_module(command_executor);
    register_module(led_strip, true);       // sync idx 0
    register_module(wifi);
    register_module(web_interface, true);    // sync idx 1
    register_module(homeassistant, true);    // sync idx 2
    register_module(homekit, true);          // sync idx 3
    register_module(alexa, true);            // sync idx 4
    register_module(time);
    register_module(scheduler);
    register_module(buttons);
}

void ModuleController::begin() {
    serial_port.begin(SerialPortConfig{});
    nvs.begin(NvsConfig{});

    const bool init_setup_flag = !nvs.read<bool>("root", "init_setup_flag");

    system.begin(SystemConfig{});
    command_executor.begin(CommandExecutorConfig{});

    led_strip.begin(LedStripConfig{});

    wifi.begin(WifiConfig{});

    web_interface.add_requirement(wifi);
    web_interface.begin(WebInterfaceConfig{});

    homeassistant.add_requirement(web_interface);
    homeassistant.begin(HomeAssistantConfig{});

    homekit.add_requirement(wifi);
    homekit.begin(HomeKitConfig{});

    alexa.add_requirement(wifi);
    alexa.begin(AlexaConfig{});

    time.add_requirement(wifi);
    time.begin(TimeConfig{});
    scheduler.add_requirement(time);
    scheduler.begin(SchedulerConfig{});

    buttons.begin(ButtonsConfig{});

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

bool ModuleController::register_module(Module& module,
                                       bool is_syncable) {
    auto [it, inserted] = modules.emplace(
        std::string(module.get_id()),
        &module
    );

    if (is_syncable) {
        sync_modules.push_back(static_cast<SyncModule*>(&module));
    }

    return inserted;
}

Module* ModuleController::get_module(std::string_view id) {
    auto it = modules.find(std::string(id));

    if (it == modules.end()) return nullptr;

    return it->second;
}

const std::map<std::string, Module*>& ModuleController::get_modules() const { return modules; }

void ModuleController::send_command(std::span<const std::string> recipients,
                                    std::string_view command_name,
                                    std::span<const std::string> args) {
    for (const std::string& recipient_id : recipients) {
        Module* recipient = get_module(recipient_id);

        if (recipient == nullptr) continue;

        for (const Command& command : recipient->get_commands()) {
            if (command.name != command_name) {
                continue;
            }
            if (!command.function) {
                continue;
            }
            if (args.size() != command.arg_count) {
                continue;
            }

            command.function(args);
            break;
        }
    }
}

void ModuleController::sync_color(std::array<uint8_t, 3> color,
                                  const std::array<uint8_t, SYNC_MODULES_COUNT>& sync_flags) {
    for_each_sync_module(sync_flags, [&](auto& interface) { interface.sync_color(color); });
}

void ModuleController::sync_brightness(uint8_t brightness,
                                       const std::array<uint8_t, SYNC_MODULES_COUNT>& sync_flags) {
    for_each_sync_module(sync_flags, [&](auto& interface) { interface.sync_brightness(brightness); });
}

void ModuleController::sync_state(bool state,
                                  const std::array<uint8_t, SYNC_MODULES_COUNT>& sync_flags) {
    for_each_sync_module(sync_flags, [&](auto& interface) { interface.sync_state(state); });
}

void ModuleController::sync_mode(uint8_t mode,
                                 const std::array<uint8_t, SYNC_MODULES_COUNT>& sync_flags) {
    for_each_sync_module(sync_flags, [&](auto& interface) { interface.sync_mode(mode); });
}

void ModuleController::sync_length(uint16_t length,
                                   const std::array<uint8_t, SYNC_MODULES_COUNT>& sync_flags) {
    for_each_sync_module(sync_flags, [&](auto& interface) { interface.sync_length(length); });
}

void ModuleController::sync_all(std::array<uint8_t, 3> color,
                                uint8_t brightness,
                                bool state,
                                uint8_t mode,
                                uint16_t length,
                                const std::array<uint8_t, SYNC_MODULES_COUNT>& sync_flags) {
    for_each_sync_module(sync_flags, [&](auto& interface) { interface.sync_all(color, brightness, state, mode, length); });
}
