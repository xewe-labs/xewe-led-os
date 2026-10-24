// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/src/SchedulePage/SchedulePage.h
#pragma once

#include <WebServer.h>

#include <XeWeCore.h>
#include <Scheduler/Scheduler.h>
#include <WebInterface/WebInterface.h>

#include "SchedulePageApi.h"

class SchedulePage : public xewe::Module {
public:
    // Led Web is found by its id ("led_web"); declare this module after it.
    SchedulePage(xewe::Os& host, WebInterface& web_interface_ref, Scheduler& scheduler_ref);

    // the routes on the web interface server (every boot: this module has no first-boot setup)
    void begin_routines_regular() override;

private:
    WebInterface& web_interface;
    Scheduler& scheduler;

    bool registered{false};

    void serve(const schedule_page_api::Route& route);
    std::string request_body();
    void send_response(const schedule_page_api::Response& response);
    void apply_cors();
};
