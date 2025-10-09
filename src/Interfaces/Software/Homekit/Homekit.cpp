#include "Homekit.h"
#include "../../../SystemController/SystemController.h"
#include <cmath>

// ===================== NeoPixel_RGB Service =====================

Homekit::NeoPixel_RGB::NeoPixel_RGB(SystemController* ctrl)
: Service::LightBulb(), controller(ctrl) {
    V.setRange(1, 100, 1);     // match old behavior
}

boolean Homekit::NeoPixel_RGB::update() {
    if (!controller) return false;

    // Home client -> get new values
    const bool  state       = power.getNewVal();
    const float hue_deg     = H.getNewVal<float>();     // 0..360
    const float sat_pct     = S.getNewVal<float>();     // 0..100
    const float bri_pct     = V.getNewVal<float>();     // 0..100

    // Convert to system byte ranges (H,S 0..255, V 0..255)
    const uint8_t h_byte    = static_cast<uint8_t>(std::round((hue_deg / 360.0f) * 255.0f));
    const uint8_t s_byte    = static_cast<uint8_t>(std::round((sat_pct / 100.0f) * 255.0f));
    const uint8_t bri_byte  = static_cast<uint8_t>(std::round((bri_pct / 100.0f) * 255.0f));

    // Sync flags: notify other interfaces but avoid echo storms.
    // Keep aligned with your project’s 5-flag convention.
    constexpr std::array<uint8_t,5> SYNC_FLAGS = {1,1,1,0,1};

    std::array<uint8_t, 3> rgb = LedMode::hsv_to_rgb({h_byte, s_byte, 255});
    controller->sync_state(state ? 1 : 0, SYNC_FLAGS);
    controller->sync_brightness(bri_byte, SYNC_FLAGS);
    controller->sync_color(rgb, SYNC_FLAGS);  // V fixed at 255; brightness handled separately

    return true;
}

// ========================= Homekit Class =========================

Homekit::Homekit(SystemController& controller_ref)
: Interface(controller_ref, "homekit", "homekit", /*requires_init_setup*/true,
            /*can_be_disabled*/true, /*has_cli_commands*/true) {
    DBG_PRINTLN(Homekit, "Constructor called.");
}

void Homekit::begin(const ModuleConfig& cfg) {
    DBG_PRINTLN(Homekit, "begin(): Initializing HomeSpan (Interface pattern).");

    // Always call base begin
    Module::begin(cfg);

    // Optional, best-effort config extraction
    const auto& c = static_cast<const HomekitConfig&>(cfg);
    const uint16_t port     = c.port             ;
    const int      ll       = c.log_level        ;
    const bool     noSerial = c.serial_input_off ;


    // HomeSpan setup
    homeSpan.setPortNum(port);
    homeSpan.setSerialInputDisable(noSerial);
    homeSpan.setLogLevel(ll);
    homeSpan.begin(Category::Lighting, controller.system.get_device_name().c_str());

    SPAN_ACCESSORY();
    SPAN_ACCESSORY(controller.system.get_device_name().c_str());

    device = new NeoPixel_RGB(&controller);

    DBG_PRINTF(Homekit, "begin(): HomeSpan ready (name=\"%s\", port=%u, log=%d).\n",
               accessory_name.c_str(), static_cast<unsigned>(port), ll);
}

void Homekit::loop() {
    homeSpan.poll();
}

void Homekit::reset(bool /*verbose*/) {
    // Factory reset (pairings & WiFi, as per old behavior)
    homeSpan.setLogLevel(0);
    homeSpan.processSerialCommand("F");
    homeSpan.setLogLevel(-1);
    DBG_PRINTLN(Homekit, "reset(): HomeSpan factory reset command issued.");
}

void Homekit::status() {
    homeSpan.setLogLevel(0);
    homeSpan.processSerialCommand("s");
    homeSpan.setLogLevel(-1);
}

// -------------------- System -> Homekit sync ---------------------

void Homekit::sync_color(std::array<uint8_t,3> color /* HSV */) {
    if (!device) return;

    std::array<uint8_t, 3> hsv = LedMode::rgb_to_hsv({color[0], color[1], color[2]});
    const float hue_deg = std::round((hsv[0] / 255.0f) * 360.0f);
    const float sat_pct = std::round((hsv[1] / 255.0f) * 100.0f);

    device->H.setVal(hue_deg);   // does NOT trigger update()
    device->S.setVal(sat_pct);

    DBG_PRINTF(Homekit, "sync_color(): H=%.0f°, S=%.0f%%.\n", hue_deg, sat_pct);
}

void Homekit::sync_brightness(uint8_t brightness) {
    if (!device) return;

    const float bri_pct = std::round((brightness / 255.0f) * 100.0f);
    device->V.setVal(bri_pct);

    DBG_PRINTF(Homekit, "sync_brightness(): V=%.0f%% (src=%u).\n", bri_pct, brightness);
}

void Homekit::sync_state(uint8_t state) {
    if (!device) return;

    const bool on = static_cast<bool>(state);
    device->power.setVal(on);

    DBG_PRINTF(Homekit, "sync_state(): %s.\n", on ? "ON" : "OFF");
}

void Homekit::sync_mode(uint8_t /*mode*/) {
    // Homekit has no "mode" concept; noop
}

void Homekit::sync_length(uint16_t /*length*/) {
    // Not needed for Homekit; noop
}

void Homekit::sync_all(std::array<uint8_t,3> color,
                       uint8_t brightness,
                       uint8_t state,
                       uint8_t mode,
                       uint16_t length) {
    if (!device) {
        DBG_PRINTLN(Homekit, "sync_all(): FAILED - device not initialized.");
        return;
    }
    DBG_PRINTLN(Homekit, "sync_all(): Full sync to Homekit.");
    sync_state(state);
    sync_brightness(brightness);
    sync_color(color);
    sync_mode(mode);     // noop
    sync_length(length); // noop
}
