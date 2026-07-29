// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Software/WebInterface/WebInterface.h
#pragma once

#include <WebServer.h>
#include <string>
#include <sstream>
#include <iomanip>

#include "../../Module/Module.h"
#include "../Wifi/Wifi.h"

struct WebInterfaceConfig : public ModuleConfig {};


class WebInterface : public Module {
public:
    explicit                    WebInterface                (ModuleController& controller);

    void                        begin_routines_regular      (const ModuleConfig& cfg)       override;

    void                        loop                        ()                              override;
    std::string                 status                      (const bool verbose=false)      const override;

    WebServer&                  get_server                  ()                              { return http_server; }
private:
    WebServer                   http_server                  {80};

    void                        serve_main_page               ();
    void                        handle_command_request        ();

    static const char           INDEX_HTML                  [] PROGMEM;
};
