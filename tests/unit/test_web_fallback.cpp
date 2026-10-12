// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/tests/unit/test_web_fallback.cpp
//
// Host test of src/WebFallback/WebFallbackApi.h: the CORS headers and the two answers of the
// catch-all. Built by tests/unit/test_web_fallback.py with g++ -Wall -Wextra -Werror.

#include <cstdio>
#include <cstring>

#include "WebFallbackApi.h"

namespace api = web_fallback_api;

static int checks = 0;
static int failures = 0;

#define CHECK(cond) \
    do { \
        ++checks; \
        if (!(cond)) { \
            ++failures; \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        } \
    } while (0)

static bool eq(const char* a, const char* b) {
    return a != nullptr && b != nullptr && std::strcmp(a, b) == 0;
}

int main() {
    CHECK(eq(api::CORS_HEADERS[0][0], "Access-Control-Allow-Origin"));
    CHECK(eq(api::CORS_HEADERS[0][1], "*"));
    CHECK(eq(api::CORS_HEADERS[1][0], "Access-Control-Allow-Methods"));
    CHECK(eq(api::CORS_HEADERS[1][1], "POST, GET, OPTIONS, DELETE, PUT"));
    CHECK(eq(api::CORS_HEADERS[2][0], "Access-Control-Allow-Headers"));
    CHECK(eq(api::CORS_HEADERS[2][1], "Content-Type"));

    // OPTIONS: 204, the server's default type (text/html), empty body
    constexpr api::Answer preflight = api::answer(true);
    CHECK(preflight.code == 204);
    CHECK(preflight.type == nullptr);
    CHECK(eq(preflight.body, ""));

    // any other method: 404 text/plain "404: Not Found"
    constexpr api::Answer missing = api::answer(false);
    CHECK(missing.code == 404);
    CHECK(eq(missing.type, "text/plain"));
    CHECK(eq(missing.body, "404: Not Found"));

    std::printf(
        "%s: %d check(s), %d failure(s)\n", failures ? "FAILED" : "PASSED", checks, failures);
    return failures ? 1 : 0;
}
