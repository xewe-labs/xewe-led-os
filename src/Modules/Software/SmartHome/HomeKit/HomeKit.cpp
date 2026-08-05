// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Software/SmartHome/HomeKit/HomeKit.cpp

#include "HomeKit.h"
#include "../../../Module/ModuleController.h"
#include "../../../../Utils/XeWeColor.h"

using namespace xewe::color;

// required
HomeKit::HomeKit(ModuleController& controller)
      : SyncModule(controller,
               /* id                  */ "homekit",
               /* name                */ "HomeKit",
               /* description         */ "Allows to control the LED via Apple Home App.\nREQUIRES Apple Hub (Speaker/Apple TV)",
               /* requires_init_setup */ true,
               /* can_be_disabled     */ true,
               /* has_cli_cmds        */ true)
{}

void HomeKit::sync_color(std::array<uint8_t,3> color) {
    if (is_disabled()) return;
    if (!device) return;

    std::array<uint8_t, 3> hsv = rgb_to_hsv({color[0], color[1], color[2]});
    const float hue_deg = std::round((hsv[0] / 255.0f) * 360.0f);
    const float sat_pct = std::round((hsv[1] / 255.0f) * 100.0f);

    device->H.setVal(hue_deg);   // does NOT trigger update()
    device->S.setVal(sat_pct);

    DBG_PRINTF(HomeKit, "sync_color(): H=%.0f°, S=%.0f%%.\n", hue_deg, sat_pct);
}

void HomeKit::sync_brightness(uint8_t brightness) {
    if (is_disabled()) return;
    if (!device) return;

    const float bri_pct = std::round((brightness / 255.0f) * 100.0f);
    device->V.setVal(bri_pct);
}

void HomeKit::sync_state(bool state) {
    if (is_disabled()) return;
    if (!device) return;

    device->power.setVal(state);
}

void HomeKit::sync_mode(uint8_t mode) {
    if (is_disabled()) return;
    if (!mode_selector) return;

    mode_selector->input.setVal(mode);   // does NOT trigger update()
}

void HomeKit::sync_length(uint16_t length) {
    if (is_disabled()) return; // not supported
}

// optional
void HomeKit::sync_all(std::array<uint8_t,3> color,
                   uint8_t brightness,
                   bool state,
                   uint8_t mode,
                   uint16_t length) {

    if (is_disabled()) return;
    if (!device) {
        DBG_PRINTLN(HomeKit, "sync_all(): FAILED - device not initialized.");
        return;
    }
    DBG_PRINTLN(HomeKit, "sync_all(): Full sync to HomeKit.");
    sync_state(state);
    sync_brightness(brightness);
    sync_color(color);
    sync_mode(mode);
}

void HomeKit::begin_routines_required (const ModuleConfig& cfg) {
    instance = this;

    homeSpan.setPortNum(1201);
    homeSpan.setSerialInputDisable(true);
    homeSpan.setLogLevel(-1);
    homeSpan.begin(Category::Lighting, controller.system.get_device_name().c_str());

    SPAN_ACCESSORY();
    SPAN_ACCESSORY(controller.system.get_device_name().c_str());

    device = new NeoPixel_RGB(&controller);

    SPAN_ACCESSORY("LED Mode");
    mode_selector = new ModeSelector(&controller,
                                     controller.led_strip.get_all_modes(),
                                     controller.led_strip.get_current_mode_id());
}

void HomeKit::begin_routines_init (const ModuleConfig& cfg) {
    homeSpan.setStatusCallback(&HomeKit::status_callback);
    controller.serial_port.print("\nOpen the link with the Setup QR below and scan it\nwith your iPhone/iPad");
    controller.serial_port.print("https://github.com/maxdokukin/xewe-led-os/blob/main/static/media/resources/HomeKit_Connect_QR.png");
    controller.serial_port.print("If using Mac, go to the Home App and add device\nusing code 4663-7726");
    controller.serial_port.print("\nThe setup process will continue automatically\nafter device is pared with HomeKit");

    controller.serial_port.print("TO ABORT PRESS (x): ");
    while(hs_status != 3) {
        homeSpan.poll();
        controller.serial_port.loop();
        if (controller.serial_port.has_line()){
            std::string input = controller.serial_port.read_line();
            if (input[0] == 'x') {
                disable(false, true); // reset with no verbose and restart
                return;
            }
            else
                controller.serial_port.print("\n(x)?: ");
        }
    }

    controller.serial_port.print("Setting up HomeKit");
    run_with_dots([this] { homeSpan.poll(); }, 3000);
    controller.serial_port.print("\nDevice successfully paired with HomeKit.\nNote, it will stop working with HomeKit App if you dont have a hub");
}

void HomeKit::loop () {
   if (is_disabled()) return;
    homeSpan.poll();
}

void HomeKit::reset (const bool verbose, const bool do_restart, const bool keep_enabled) {
    if (verbose) controller.serial_port.print("You also need to remove the device from the Home App manually");
    homeSpan.processSerialCommand("F");
    delay(100);
    Module::reset(verbose, do_restart, keep_enabled);
}

std::string HomeKit::status (const bool verbose) const {
   if (is_disabled()) return std::string("HomeKit module disabled");
    homeSpan.setLogLevel(2);
    homeSpan.processSerialCommand("s");
    homeSpan.processSerialCommand("i");
    homeSpan.setLogLevel(-1);
    return Module::status(verbose);
}

// other methods
HomeKit* HomeKit::instance = nullptr;
void HomeKit::status_callback(HS_STATUS s) {
    if (instance) {
        instance->hs_status = static_cast<uint8_t>(s);
    }
}
HomeKit::NeoPixel_RGB::NeoPixel_RGB(ModuleController* ctrl)
: Service::LightBulb(), controller(ctrl) {
    V.setRange(1, 100, 1);
}

boolean HomeKit::NeoPixel_RGB::update() {
    if (!controller) return false;

    const bool  state       = power.getNewVal();
    const float hue_deg     = H.getNewVal<float>();     // 0..360
    const float sat_pct     = S.getNewVal<float>();     // 0..100
    const float bri_pct     = V.getNewVal<float>();     // 0..100

    const uint8_t h_byte    = static_cast<uint8_t>(std::round((hue_deg / 360.0f) * 255.0f));
    const uint8_t s_byte    = static_cast<uint8_t>(std::round((sat_pct / 100.0f) * 255.0f));
    const uint8_t bri_byte  = static_cast<uint8_t>(std::round((bri_pct / 100.0f) * 255.0f));

    std::array<uint8_t, 3> rgb = hsv_to_rgb({h_byte, s_byte, 255});
    controller->sync_color(rgb, {1,1,1,0,1});  // V fixed at 255; brightness handled separately
    controller->sync_brightness(bri_byte, {1,1,1,0,1});
    controller->sync_state(state ? 1 : 0, {1,1,1,0,1});

    return true;
}

HomeKit::ModeSelector::ModeSelector(ModuleController*                                    ctrl,
                                    const std::vector<std::pair<uint8_t, std::string>>& modes,
                                    uint8_t                                             current_mode_id)
: Service::Television(), controller(ctrl) {
    input.setVal(current_mode_id);

    for (const auto& [id, name] : modes) {
        SpanService* source = new Service::InputSource();
        new Characteristic::ConfiguredName(name.c_str());
        new Characteristic::Identifier(id);
        new Characteristic::IsConfigured(1);
        new Characteristic::CurrentVisibilityState(0);   // 0 = shown in Home App
        addLink(source);
    }
}

boolean HomeKit::ModeSelector::update() {
    if (!controller) return false;

    if (input.updated()) {
        const uint8_t mode = input.getNewVal<uint8_t>();
        controller->sync_mode(mode, {1,1,1,0,1});   // skip idx 3 (self)
        DBG_PRINTF(HomeKit, "ModeSelector::update(): mode=%u\n", mode);
    }

    return true;
}