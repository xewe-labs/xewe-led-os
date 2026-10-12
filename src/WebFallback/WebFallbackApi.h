// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/src/WebFallback/WebFallbackApi.h
//
// The answer of the web server's catch-all (a request no route matched), without the server:
// the three CORS headers on every answer, an empty 204 to a CORS preflight (OPTIONS), else
// `404: Not Found` as plain text. Standard library only, so host tests check the same values the
// firmware sends.
#pragma once

namespace web_fallback_api {

// Sent before every answer.
inline constexpr const char* CORS_HEADERS[3][2] = {
    {"Access-Control-Allow-Origin", "*"},
    {"Access-Control-Allow-Methods", "POST, GET, OPTIONS, DELETE, PUT"},
    {"Access-Control-Allow-Headers", "Content-Type"},
};

struct Answer {
    int code;
    // nullptr: the server's default type and an empty body (WebServer::send(code) sends
    // Content-Type: text/html)
    const char* type;
    const char* body;
};

inline constexpr Answer PREFLIGHT = {204, nullptr, ""};
inline constexpr Answer NOT_FOUND = {404, "text/plain", "404: Not Found"};

inline constexpr Answer answer(bool is_options) {
    return is_options ? PREFLIGHT : NOT_FOUND;
}

} // namespace web_fallback_api
