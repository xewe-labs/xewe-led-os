// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/src/SchedulePage/SchedulePage.cpp

#include "SchedulePage.h"

#include <cstring>

#include "pages/schedule_actions_js.h"
#include "pages/schedule_core_js.h"
#include "pages/schedule_html.h"
#include "pages/schedule_interactions_js.h"
#include "pages/schedule_style_css.h"
#include "pages/schedule_ui_js.h"

SchedulePage::SchedulePage(
    xewe::Os& host, WebInterface& web_interface_ref, Scheduler& scheduler_ref)
    : xewe::Module(host,
          /* id                  */ "schedule_page",
          /* name                */ "Schedule Page",
          /* description         */ "Allows to edit the schedules in web browser",
          /* requires_init_setup */ false,
          /* can_be_disabled     */ false,
          /* has_cli_cmds        */ false),
      web_interface(web_interface_ref), scheduler(scheduler_ref) {
    xewe::Module* led_web = host.get_module("led_web");
    if (led_web != nullptr)
        add_requirement(*led_web);
    else
        host.report_error("! Schedule Page: module led_web not declared before it; no requirement");
}

void SchedulePage::begin_routines_regular() {
    if (registered)
        return;
    registered = true;

    WebServer& server = web_interface.get_server();
    for (const schedule_page_api::Route& route : schedule_page_api::ROUTES) {
        const HTTPMethod method = std::strcmp(route.method, "POST") == 0 ? HTTP_POST : HTTP_GET;
        server.on(route.path, method, [this, &route]() { serve(route); });
    }
}

void SchedulePage::serve(const schedule_page_api::Route& route) {
    if (is_disabled())
        return;
    WebServer& server = web_interface.get_server();
    apply_cors();

    const char* type = schedule_page_api::content_type(route.content);
    switch (route.content) {
    case schedule_page_api::Content::Page:
        server.send_P(200, type, SCHEDULE_HTML);
        break;
    case schedule_page_api::Content::Style:
        server.send_P(200, type, SCHEDULE_STYLE_CSS);
        break;
    case schedule_page_api::Content::CoreJs:
        server.send_P(200, type, SCHEDULE_CORE_JS);
        break;
    case schedule_page_api::Content::ActionsJs:
        server.send_P(200, type, SCHEDULE_ACTIONS_JS);
        break;
    case schedule_page_api::Content::InteractionsJs:
        server.send_P(200, type, SCHEDULE_INTERACTIONS_JS);
        break;
    case schedule_page_api::Content::UiJs:
        server.send_P(200, type, SCHEDULE_UI_JS);
        break;
    case schedule_page_api::Content::UtilsJs:
        server.send(200, type, "");
        break;
    case schedule_page_api::Content::List:
        send_response(schedule_page_api::list_schedules(scheduler));
        break;
    case schedule_page_api::Content::Set:
        send_response(
            schedule_page_api::set_schedule(server.hasArg("plain"), request_body(), scheduler));
        break;
    case schedule_page_api::Content::Delete:
        send_response(
            schedule_page_api::delete_schedule(server.hasArg("plain"), request_body(), scheduler));
        break;
    }
}

// the request body (the "plain" argument; empty when absent), byte for byte
std::string SchedulePage::request_body() {
    const String plain = web_interface.get_server().arg("plain");
    return std::string(plain.c_str(), plain.length());
}

void SchedulePage::send_response(const schedule_page_api::Response& response) {
    web_interface.get_server().send(response.code, response.type, response.body.c_str());
}

void SchedulePage::apply_cors() {
    WebServer& server = web_interface.get_server();
    for (const auto& header : schedule_page_api::CORS_HEADERS)
        server.sendHeader(header[0], header[1]);
}
