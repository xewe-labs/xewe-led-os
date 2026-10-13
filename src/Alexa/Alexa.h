// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/src/Alexa/Alexa.h
//
// Amazon Alexa control of the LED through Espalexa (a Philips Hue bridge emulation) on the web
// interface's server. When enabled it owns the server's not-found handler: Espalexa's API calls
// are answered there, everything else gets `404 Endpoint not found.`. The first boot waits until
// an Echo device has discovered the light (serial `x` aborts and disables the module).
#pragma once

#include <Espalexa.h>
#include <WebServer.h>

#include <array>
#include <cstdint>

#include <XeWeCore.h>
#include <Led/Led.h>
#include <WebInterface/WebInterface.h>
#include <Wifi/Wifi.h>

class Alexa : public xewe::Module, public LedListener {
public:
    // requires wifi (`$wifi disable` disables Alexa too); uses the web interface's server, which is
    // not a requirement
    Alexa(xewe::Os& host, Wifi& wifi_ref, WebInterface& web_interface_ref, Led& led_ref);

    // installs the not-found handler, starts Espalexa on the web interface's server, adds the
    // colour device and registers the LED listener
    void begin_routines_required() override;
    // blocking discovery wait (first boot, or the boot after an enable or reset)
    void begin_routines_init() override;
    void loop() override;
    // prints the reminder to remove the device from the Alexa app (also when not verbose)
    void reset(const bool verbose = false, const bool do_restart = true,
        const bool keep_enabled = true) override;

    // LED -> Alexa; events this module caused itself are ignored
    void on_color(std::array<uint8_t, 3> rgb, const void* origin) override;
    void on_brightness(uint8_t brightness, const void* origin) override;
    void on_state(bool on, const void* origin) override;
    void on_mode(uint8_t mode_id, const void* origin) override;
    void on_length(uint16_t length, const void* origin) override;
    // state, then brightness, then colour
    void on_all(std::array<uint8_t, 3> rgb, uint8_t brightness, bool on, uint8_t mode_id,
        uint16_t length, const void* origin) override;

private:
    // Alexa -> LED
    void update_event(EspalexaDevice* device_ptr);

    WebInterface& web_interface;
    Led& led;
    Espalexa espalexa;
    EspalexaDevice* device = nullptr;
};
