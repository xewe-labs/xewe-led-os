// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/src/SchedulePage/SchedulePage.h
//
// The schedule editor in a browser: the page, its stylesheet and scripts and the /schedule/json,
// /schedule/set and /schedule/delete routes, served by the web interface module's server. The
// blocks live in the Scheduler module; this module only reads and edits them. Route table and
// answers: SchedulePageApi.h.
//
// Requires Led Web (the page's Home link is `/`, the LED page). It does not require the scheduler:
// with the scheduler disabled the page still loads, the list is empty and every save is rejected.
// It cannot be disabled and has no commands of its own.
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
    void send_response(const schedule_page_api::Response& response);
    void apply_cors();
};
