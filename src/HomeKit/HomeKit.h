// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/src/HomeKit/HomeKit.h
//
// Apple Home control of the LED strip through HomeSpan: one Lighting accessory (On, Hue,
// Saturation, Brightness) on HAP port 1201. The first boot waits until the device is paired
// (serial `x` aborts and disables the module). Home app writes set colour, brightness and state on
// the led; every led change made elsewhere is mirrored back to the Home app.
#pragma once

#include <HomeSpan.h>

#include <array>
#include <cstdint>
#include <string>

#include <XeWeCore.h>

#include <Led/Led.h>
#include <Wifi/Wifi.h>

// The setup QR code image printed on the first boot (a link: the serial console cannot show it).
// The QR encodes HomeSpan's default setup code 466-37-726 and setup ID HSPN.
#ifndef HOMEKIT_QR_URL
#define HOMEKIT_QR_URL \
    "https://github.com/xewe-labs/xewe-led-os/raw/v2.3.0/static/media/resources/" \
    "HomeKit_Connect_QR.png"
#endif

class HomeKit : public xewe::Module, public LedListener {
public:
    HomeKit(xewe::Os& host, Wifi& wifi, Led& led);

    // HomeSpan setup, the accessory and the led listener
    void begin_routines_required() override;
    // the pairing text and the blocking wait for the pairing
    void begin_routines_init() override;
    // HomeSpan poll (HAP requests, Home app writes)
    void loop() override;
    // erases the HomeKit pairings (HomeSpan `U`, or the stored list when HomeSpan never started
    // on this boot), then the core reset
    void reset(const bool verbose = false, const bool do_restart = true,
        const bool keep_enabled = true) override;
    // HomeSpan status and info dumps, then `HomeKit module enabled`; nothing when disabled
    std::string status(const bool verbose = false) const override;

    // led -> Home app; changes this module made itself are not sent back
    void on_color(std::array<uint8_t, 3> rgb, const void* origin) override;
    void on_brightness(uint8_t brightness, const void* origin) override;
    void on_state(bool on, const void* origin) override;
    void on_mode(uint8_t mode_id, const void* origin) override;
    // `$led reset`: state, brightness, colour
    void on_all(std::array<uint8_t, 3> rgb, uint8_t brightness, bool on, uint8_t mode_id,
        uint16_t length, const void* origin) override;

private:
    static void status_callback(HS_STATUS s);

    struct NeoPixel_RGB : Service::LightBulb {
        Characteristic::On power{0, true};
        Characteristic::Hue H{0, true};
        Characteristic::Saturation S{0, true};
        Characteristic::Brightness V{100, true};

        HomeKit* owner;

        explicit NeoPixel_RGB(HomeKit* owner_module);
        // Home app -> led: colour, brightness and state, on every write
        boolean update() override;
    };

    Led& led;
    NeoPixel_RGB* device = nullptr;
    static HomeKit* instance;
    uint8_t hs_status = 0;
};
