// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Module/ModuleController.h
#pragma once

#include <map>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Module.h"
#include "../Core/SerialPort/SerialPort.h"
#include "../Core/Nvs/Nvs.h"
#include "../Core/System/System.h"
#include "../Core/CommandExecutor/CommandExecutor.h"
#include "../Hardware/LedStrip/LedStrip.h"
#include "../Software/Wifi/Wifi.h"
#include "../Software/WebInterface/WebInterface.h"
#include "../Software/Time/Time.h"
#include "../Software/Time/Scheduler/Scheduler.h"
#include "../Hardware/Buttons/Buttons.h"
#include "SyncModule.h"


#define SYNC_MODULES_COUNT 5 // [led_strip, web_interface, homeassistant, homekit, alexa]

class ModuleController {
public:
                                          ModuleController     ();

    void                                  begin                ();
    void                                  loop                 ();

    bool                                  register_module      (Module& module,
                                                                bool    is_syncable = false);
    Module*                               get_module           (std::string_view id);
    const std::map<std::string, Module*>& get_modules          () const;

    void                                  send_command         (std::span<const std::string> recipients,
                                                                std::string_view             command_name,
                                                                std::span<const std::string> args);

    void                                  sync_color           (const std::array<uint8_t, 3> color,
                                                                const std::array<uint8_t, SYNC_MODULES_COUNT>& sync_flags);
    void                                  sync_brightness      (const uint8_t brightness,
                                                                const std::array<uint8_t, SYNC_MODULES_COUNT>& sync_flags);
    void                                  sync_state           (const bool state,
                                                                const std::array<uint8_t, SYNC_MODULES_COUNT>& sync_flags);
    void                                  sync_mode            (const uint8_t mode,
                                                                const std::array<uint8_t, SYNC_MODULES_COUNT>& sync_flags);
    void                                  sync_length          (const uint16_t length,
                                                                const std::array<uint8_t, SYNC_MODULES_COUNT>& sync_flags);
    void                                  sync_all             (const std::array<uint8_t, 3> color,
                                                                const uint8_t                brightness,
                                                                const bool                   state,
                                                                const uint8_t                mode,
                                                                const uint16_t               length,
                                                                const std::array<uint8_t, SYNC_MODULES_COUNT>& sync_flags);

    SerialPort                            serial_port;
    Nvs                                   nvs;
    System                                system;
    CommandExecutor                       command_executor;

    LedStrip                              led_strip;

    Wifi                                  wifi;
    WebInterface                          web_interface;

    Time                                  time;
    Scheduler                             scheduler;

    Buttons                               buttons;

private:
    std::map<std::string, Module*>        modules              {};
    std::vector<SyncModule*>              sync_modules         {};
    std::vector<std::unique_ptr<Module>>  owned_modules        {};

    template <typename Fn>
    void                                  for_each_sync_module (const std::array<uint8_t, SYNC_MODULES_COUNT>& sync_flags,
                                                                Fn&& fn);
};

template <typename Fn>
void ModuleController::for_each_sync_module(const std::array<uint8_t, SYNC_MODULES_COUNT>& flags,
                                            Fn&& fn) {
    for (std::size_t i = 0; i < SYNC_MODULES_COUNT; ++i) {
        if (i >= 1) return;

        if (flags[i] && sync_modules[i]) {
            std::forward<Fn>(fn)(*sync_modules[i]);
        }
    }
}