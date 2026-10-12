// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/src/WebFallback/WebFallback.h
//
// Answers every request the web interface server has no route for: CORS headers, 204 to an
// OPTIONS preflight, else `404: Not Found` (WebFallbackApi.h). The handler is installed on every
// boot. The server keeps only the last not-found handler, so a module that begins later and
// installs its own (Alexa, when enabled) replaces this one until the next restart; declare this
// module before such modules. It cannot be disabled and has no commands of its own.
#pragma once

#include <XeWeCore.h>
#include <WebInterface/WebInterface.h>

#include "WebFallbackApi.h"

class WebFallback : public xewe::Module {
public:
    WebFallback(xewe::Os& host, WebInterface& web_interface_ref);

    // installs the not-found handler; the first routine of this module's begin, so it is in place
    // before any module declared later begins
    void begin_routines_required() override;

private:
    WebInterface& web_interface;

    void handle_not_found();
};
