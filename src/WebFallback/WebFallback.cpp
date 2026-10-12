// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/src/WebFallback/WebFallback.cpp

#include "WebFallback.h"

#include <WebServer.h>

WebFallback::WebFallback(xewe::Os& host, WebInterface& web_interface_ref)
    : xewe::Module(host,
          /* id                  */ "web_fallback",
          /* name                */ "Web Fallback",
          /* description         */ "Answers CORS preflights and unknown paths of the web server",
          /* requires_init_setup */ false,
          /* can_be_disabled     */ false,
          /* has_cli_cmds        */ false),
      web_interface(web_interface_ref) {}

void WebFallback::begin_routines_required() {
    web_interface.get_server().onNotFound([this]() { handle_not_found(); });
}

void WebFallback::handle_not_found() {
    WebServer& server = web_interface.get_server();
    for (const auto& header : web_fallback_api::CORS_HEADERS)
        server.sendHeader(header[0], header[1]);

    const web_fallback_api::Answer a = web_fallback_api::answer(server.method() == HTTP_OPTIONS);
    if (a.type == nullptr)
        server.send(a.code);
    else
        server.send(a.code, a.type, a.body);
}
