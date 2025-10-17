/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/



// src/Interfaces/Alexa/Alexa.cpp

#include "Alexa.h"
#include "../../../SystemController/SystemController.h"


// required
Alexa::Alexa(SystemController& controller)
      : Interface(controller,
               /* module_name         */ "Alexa",
               /* module_description  */ "It allows to control LEDs with Amazon Alexa\nspeaker and app",
               /* nvs_key             */ "alx",
               /* requires_init_setup */ true,
               /* can_be_disabled     */ true,
               /* has_cli_cmds        */ true)
{}


void Alexa::sync_color(std::array<uint8_t,3> color) {
    if (is_disabled()) return;
    if (!device) return;
    DBG_PRINTF(Alexa, "sync_color(): R=%u, G=%u, B=%u\n", color[0], color[1], color[2]);
    std::array<uint8_t, 3> hsv = LedMode::rgb_to_hsv({color[0], color[1], color[2]});
    const uint16_t hue16 = (uint16_t(hsv[0]) << 8) | hsv[0];
    const uint8_t sat8 = hsv[1] == 255 ? 254 : hsv[1];
    device->setColor(hue16, sat8);
}

void Alexa::sync_brightness(uint8_t brightness) {
    if (is_disabled()) return;
    if (!device) return;
    DBG_PRINTF(Alexa, "sync_brightness(): brightness=%u\n", brightness);
    device->setValue(brightness);
}

void Alexa::sync_state(uint8_t state) {
    if (is_disabled()) return;
    if (!device) return;
    DBG_PRINTF(Alexa, "sync_state(): state=%s\n", state ? "ON" : "OFF");
    device->setState(static_cast<bool>(state));
}

void Alexa::sync_mode(uint8_t mode) {
    if (is_disabled()) return; // not supported
}

void Alexa::sync_length(uint16_t length) {
    if (is_disabled()) return; // not supported
}

// optional
void Alexa::sync_all(std::array<uint8_t,3> color,
                   uint8_t brightness,
                   uint8_t state,
                   uint8_t mode,
                   uint16_t length) {
    if (is_disabled()) return;

    sync_state(state);
    sync_brightness(brightness);
    sync_color(color);
}

void Alexa::begin_routines_required (const ModuleConfig& cfg) {
//    const auto& config = static_cast<const AlexaConfig&>(cfg);
    WebServer& server_ref = controller.web.get_server();
    server_ref.onNotFound([this, &server_ref]() {
        if (!espalexa.handleAlexaApiCall(server_ref.uri(), server_ref.arg(0))) {
            server_ref.send(404, "text/plain", "Endpoint not found.");
        }
    });
    espalexa.begin(&server_ref);
    
    device = new EspalexaDevice(
        controller.system.get_device_name().c_str(),
        [this](EspalexaDevice* d) { this->update_event(d); },
        EspalexaDeviceType::color
    );

    espalexa.addDevice(device);
}

void Alexa::begin_routines_init (const ModuleConfig& cfg) {
//    const auto& config = static_cast<const AlexaConfig&>(cfg);
    controller.serial_port.print("\nAsk Alexa to discover new devices\nPress \"x\" after Alexa says it discovered\nand connected new device\n(x)?: ");
    bool pairing = true;
    while(pairing) {
        espalexa.loop();
        controller.serial_port.loop();
        if (controller.serial_port.has_line()){
            std::string input = controller.serial_port.read_line();
            if (input[0] == 'x')
                pairing = false;
            else
            controller.serial_port.print("\n(x)?: ");
        }
    }
    // todo is there a way to read this from the library?
    controller.serial_port.print("Setting up Alexa");
    run_with_dots([this] { espalexa.loop(); }, 1000);
    controller.serial_port.println("\nDevice successfully paired with Alexa");
}

void Alexa::loop () {
   if (is_disabled()) return;
    espalexa.loop();
}

void Alexa::reset (const bool verbose, const bool do_restart) {
    controller.serial_port.println("You also need to remove the device from the Alexa App manually.");
    Module::reset(verbose, do_restart); // this will restart the system
}

// other methods
// make sure they have
// if (is_disabled()) return;
void Alexa::update_event(EspalexaDevice* device_ptr) {
    if (is_disabled()) return;
    if (!device_ptr) return;

    bool    state_from_alexa      = device_ptr->getState();
    uint8_t brightness_from_alexa = device_ptr->getValue();
    uint8_t r_val = device_ptr->getR();
    uint8_t g_val = device_ptr->getG();
    uint8_t b_val = device_ptr->getB();

    // Preserve original propagation flags
    controller.sync_state(state_from_alexa,          {true, true, true, true, false});
    if (state_from_alexa) // avoid resetting brightness
        controller.sync_brightness(brightness_from_alexa,{true, true, true, true, false});
    controller.sync_color({r_val, g_val, b_val},       {true, true, true, true, false});
}