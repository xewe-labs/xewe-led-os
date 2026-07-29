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

#include "../Software/Wifi/Wifi.h"
#include "../Software/WebInterface/WebInterface.h"

#include "../Hardware/Buttons/Buttons.h"
#include "../Hardware/Pins/Pins.h"

// tests
#if COMPILE_TESTS
    #include "../../Tests/Nvs/NvsTester.h"
    #include "../../Tests/NvsFlex/NvsFlexTester.h"
#endif


class ModuleController {
public:
    ModuleController();

    void                                    begin               ();
    void                                    loop                ();

    bool                                    register_module     (Module& module);
    Module*                                 get_module          (std::string_view id);
    const std::map<std::string, Module*>&   get_modules         ()                              const { return modules; }

    void send_command                                           (std::span<const std::string>   recipients,
                                                                std::string_view                command_name,
                                                                std::span<const                 std::string> args);

    SerialPort                              serial_port;
    Nvs                                     nvs;
    System                                  system;
    CommandExecutor                         command_executor;

    Wifi                                    wifi;
    WebInterface                            web_interface;

    Buttons                                 buttons;
    Pins                                    pins;

private:
    std::map<std::string, Module*>          modules             {};
    std::vector<std::unique_ptr<Module>>    owned_modules       {};
};