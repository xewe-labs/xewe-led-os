// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/src/Alexa/Alexa.cpp

#include "Alexa.h"

#include <string>

#include "AlexaMap.h"

Alexa::Alexa(xewe::Os& host, Wifi& wifi_ref, WebInterface& web_interface_ref, Led& led_ref)
    : xewe::Module(host,
          /* id                  */ "alexa",
          /* name                */ "Alexa",
          /* description         */ "It allows to control LEDs with Amazon Alexa\nspeaker and app",
          /* requires_init_setup */ true,
          /* can_be_disabled     */ true,
          /* has_cli_commands    */ true),
      web_interface(web_interface_ref), led(led_ref) {
    add_requirement(wifi_ref);
}

// LED -> Alexa
void Alexa::on_color(std::array<uint8_t, 3> rgb, const void* origin) {
    if (origin == this)
        return;
    if (is_disabled())
        return;
    if (!device)
        return;
    const std::array<uint8_t, 3> hsv = xewe::color::rgb_to_hsv({rgb[0], rgb[1], rgb[2]});
    const uint16_t hue16 = led_map::hue8_to_hue16(hsv[0]);
    const uint8_t sat8 = led_map::sat8_to_alexa(hsv[1]);
    device->setColor(hue16, sat8);
}

// Sent also while the LED is off: a non-zero value then makes Alexa report the light as on.
void Alexa::on_brightness(uint8_t brightness, const void* origin) {
    if (origin == this)
        return;
    if (is_disabled())
        return;
    if (!device)
        return;
    device->setValue(brightness);
}

void Alexa::on_state(bool on, const void* origin) {
    if (origin == this)
        return;
    if (is_disabled())
        return;
    if (!device)
        return;
    device->setState(on);
}

void Alexa::on_mode(uint8_t mode_id, const void* origin) {
    (void)mode_id; // not supported by Alexa
    (void)origin;
}

void Alexa::on_length(uint16_t length, const void* origin) {
    (void)length; // not supported by Alexa
    (void)origin;
}

void Alexa::on_all(std::array<uint8_t, 3> rgb, uint8_t brightness, bool on, uint8_t mode_id,
    uint16_t length, const void* origin) {
    (void)mode_id;
    (void)length;
    if (is_disabled())
        return;

    on_state(on, origin);
    on_brightness(brightness, origin);
    on_color(rgb, origin);
}

// Runs only when the module is enabled and wifi is enabled. The Web Fallback module, declared
// before Alexa, installs its handler on every boot; a WebServer keeps only the last not-found
// handler, so this one replaces it while Alexa is enabled.
void Alexa::begin_routines_required() {
    WebServer& server_ref = web_interface.get_server();
    server_ref.onNotFound([this, &server_ref]() {
        if (!espalexa.handleAlexaApiCall(server_ref.uri(), server_ref.arg(0))) {
            server_ref.send(404, "text/plain", "Endpoint not found.");
        }
    });
    // Espalexa adds /espalexa and /description.xml and (re)starts the server itself. Its SSDP
    // replies and description.xml always announce port 80, so Alexa needs the web interface on 80.
    espalexa.begin(&server_ref);

    device = new EspalexaDevice(
        os.system.get_device_name().c_str(), [this](EspalexaDevice* d) { this->update_event(d); },
        EspalexaDeviceType::color);

    espalexa.addDevice(device);

    if (!led.listeners.add(this)) {
        os.serial.print("! Alexa: LED listener list full; LED changes will not reach Alexa");
    }
}

void Alexa::begin_routines_init() {
    os.serial.print("\nAsk Alexa to discover new devices\nThe setup process will continue "
                    "automatically\nafter device is pared with Alexa");
    os.serial.print("TO ABORT PRESS (x): ");
    while (!espalexa.get_responded_to_search()) {
        espalexa.loop();
        os.serial.loop();
        if (os.serial.has_line()) {
            std::string input = os.serial.read_line();
            if (input[0] == 'x') {
                disable(false, true); // no confirmation, resets the module, restarts
                return;
            } else
                os.serial.print("\n(x)?: ");
        }
    }
    os.serial.print("Setting up Alexa");
    run_with_dots([this] { espalexa.loop(); }, 3000);
    os.serial.print("\nDevice successfully paired with Alexa");
}

void Alexa::loop() {
    if (is_disabled())
        return;
    espalexa.loop();
}

void Alexa::reset(const bool verbose, const bool do_restart, const bool keep_enabled) {
    os.serial.print("You also need to remove the device from the Alexa App manually.");
    xewe::Module::reset(verbose, do_restart, keep_enabled);
}

// Alexa -> LED: state, then brightness only while on (so turning off keeps the brightness), then
// colour
void Alexa::update_event(EspalexaDevice* device_ptr) {
    if (is_disabled())
        return;
    if (!device_ptr)
        return;

    bool state_from_alexa = device_ptr->getState();
    uint8_t brightness_from_alexa = device_ptr->getValue();
    uint8_t r_val = device_ptr->getR();
    uint8_t g_val = device_ptr->getG();
    uint8_t b_val = device_ptr->getB();

    led.sync_state(state_from_alexa, this);
    if (state_from_alexa)
        led.sync_brightness(brightness_from_alexa, this);
    led.sync_color({r_val, g_val, b_val}, this);
}
