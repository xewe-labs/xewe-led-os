<!-- SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com) -->
<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Web Fallback (`web_fallback`)

Answers every request the web interface server has no route for.

| | |
|---|---|
| id / name | `web_fallback` / `Web Fallback` |
| declare | `WebFallback web_fallback(os, web_interface);` after `src/Modules.h`, **before** `alexa` |
| requires | nothing |
| first-boot setup | none, no prompt |
| can be disabled | no |
| commands | none (state in `$system status`) |
| stored data | none |

## Answer

Every answer carries `Access-Control-Allow-Origin: *`,
`Access-Control-Allow-Methods: POST, GET, OPTIONS, DELETE, PUT` and
`Access-Control-Allow-Headers: Content-Type`. Then:

- `OPTIONS` (a CORS preflight, to any path, routed or not, since every route answers only GET or
  POST): `204`, empty body, `Content-Type: text/html` (the server's default)
- anything else: `404`, `text/plain`, `404: Not Found`

## When it is in force

The handler is installed with `onNotFound` in `begin_routines_required` on every boot, first
thing in this module's begin. Modules begin one after the other in declaration order, so the
handler is in place before any module declared later begins, and before the HTTP server first
answers in the main loop (Home Assistant starts the server during its first-boot wait, which also
comes later).

The server keeps one not-found handler: the last `onNotFound` wins. Alexa, declared after this
module, installs its own in its `begin_routines_required` when it is enabled, which replaces this
one (no CORS headers, `404 text/plain Endpoint not found.`, and the Alexa API paths). So the
catch-all in force is decided by Alexa's state at boot and changes only across a restart; enabling
or disabling Alexa always restarts. The module needs no reference to Alexa.

The answer lives in `WebFallbackApi.h` (no server, host-tested); `WebFallback.cpp` is the glue.

## Known issues

- With WiFi disabled the web interface does not start its server; the handler is installed but
  never called.
