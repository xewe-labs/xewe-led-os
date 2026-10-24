// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/xewe-led-os.ino

// The Os begins and loops the modules in the order they are declared here.

// The project's values, first so a plain Arduino IDE build reads them too.
#include "Config.h"

#include <XeWeCore.h>

// The Os, before every module: modules register with it from their constructors.
XeWeOs os({.url = "https://github.com/xewe-labs/xewe-led-os"});

// src/Modules.h is generated: it declares led, wifi, web_interface, led_web, time_module,
// scheduler and buttons, dependencies first.
#if __has_include("src/Modules.h")
#include "src/Modules.h"
#endif

// The product's modules, declared after the generated ones (they use led, wifi, web_interface,
// scheduler). The Os begins them in this order: SchedulePage, HomeAssistant, HomeKit, Alexa.
// - SchedulePage: the 2.3.0 schedule page and its JSON routes (requires led_web).
// - HomeAssistant before HomeKit: HA ends its first-boot mDNS responder (MDNS.end()) before
//   HomeSpan starts its own.
// - Alexa after web_interface: web_interface installs the 2.3.0 catch-all (CORS headers, OPTIONS
//   204, else 404) on every boot; the web server keeps only the last not-found handler, so an
//   enabled Alexa replaces it with its own and a disabled Alexa leaves it in place.
// Each integration registers its led listener in begin_routines_required, so the led notifies
// led_web, Home Assistant, HomeKit, Alexa in that order and a disabled module receives nothing.
// Do not add listener registrations here.
#include "src/SchedulePage/SchedulePage.h"
SchedulePage schedule_page(os, web_interface, scheduler);
#include "src/HomeAssistant/HomeAssistant.h"
HomeAssistant homeassistant(os, wifi, web_interface, led);
#include "src/HomeKit/HomeKit.h"
HomeKit homekit(os, wifi, led);
#include "src/Alexa/Alexa.h"
Alexa alexa(os, wifi, web_interface, led);

void setup() {
    os.begin();
}

void loop() {
    os.loop();
}
