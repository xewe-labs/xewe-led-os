// src/Interfaces/Homekit/Homekit.cpp

#include "Homekit.h"
#include "../../../SystemController/SystemController.h"


// required
Homekit::Homekit(SystemController& controller)
      : Interface(controller,
               /* module_name         */ "Homekit",
               /* module_description  */ "Allows to control the LED via Apple Home App. REQUIRES Apple Hub (Speaker/Apple TV)",
               /* nvs_key             */ "hkt",
               /* requires_init_setup */ true,
               /* can_be_disabled     */ true,
               /* has_cli_cmds        */ true)
{}


void Homekit::sync_color(std::array<uint8_t,3> color) {
    if (is_disabled()) return;
    if (!device) return;

    std::array<uint8_t, 3> hsv = LedMode::rgb_to_hsv({color[0], color[1], color[2]});
    const float hue_deg = std::round((hsv[0] / 255.0f) * 360.0f);
    const float sat_pct = std::round((hsv[1] / 255.0f) * 100.0f);

    device->H.setVal(hue_deg);   // does NOT trigger update()
    device->S.setVal(sat_pct);

    DBG_PRINTF(Homekit, "sync_color(): H=%.0f°, S=%.0f%%.\n", hue_deg, sat_pct);
}

void Homekit::sync_brightness(uint8_t brightness) {
    if (is_disabled()) return;
    if (!device) return;

    const float bri_pct = std::round((brightness / 255.0f) * 100.0f);
    device->V.setVal(bri_pct);
}

void Homekit::sync_state(uint8_t state) {
    if (is_disabled()) return;
    if (!device) return;

    const bool on = static_cast<bool>(state);
    device->power.setVal(on);
}

void Homekit::sync_mode(uint8_t mode) {
    if (is_disabled()) return; // not supported
}

void Homekit::sync_length(uint16_t length) {
    if (is_disabled()) return; // not supported
}

// optional
void Homekit::sync_all(std::array<uint8_t,3> color,
                   uint8_t brightness,
                   uint8_t state,
                   uint8_t mode,
                   uint16_t length) {

    if (is_disabled()) return;
    if (!device) {
        DBG_PRINTLN(Homekit, "sync_all(): FAILED - device not initialized.");
        return;
    }
    DBG_PRINTLN(Homekit, "sync_all(): Full sync to Homekit.");
    sync_state(state);
    sync_brightness(brightness);
    sync_color(color);
}

void Homekit::begin_routines_required (const ModuleConfig& cfg) {
//    const auto& config = static_cast<const HomekitConfig&>(cfg);
    instance = this;

    homeSpan.setPortNum(1201);
    homeSpan.setSerialInputDisable(true);
    homeSpan.setLogLevel(-1);
    homeSpan.setStatusCallback(&Homekit::status_callback);
    homeSpan.begin(Category::Lighting, controller.system.get_device_name().c_str());

    SPAN_ACCESSORY();
    SPAN_ACCESSORY(controller.system.get_device_name().c_str());

    device = new NeoPixel_RGB(&controller);
}



void Homekit::begin_routines_init (const ModuleConfig& cfg) {
//    const auto& config = static_cast<const HomekitConfig&>(cfg);
    controller.serial_port.println("Open the link with the Setup QR below and scan it with your iPhone/iPad");
    controller.serial_port.println("https://github.com/maxdokukin/XeWe-LED-OS/blob/main/doc/HomeKit_Connect_QR.png");
    controller.serial_port.println("If using Mac, go to the Home App and add device using code 4663-7729");

//    homeSpan.setLogLevel(0);
    // 3 means device paired
    while(hs_status != 3) {
        homeSpan.poll();
    }
    // extra couple seconds for pairing processing
    run_with_dots([this] { homeSpan.poll(); }, 3000);
//    homeSpan.setLogLevel(-1);
    controller.serial_port.println("Device successfully paired with HomeKit.\nNote, it will stop working if you dont have a hub.\nPress Enter to Continue");

    // setup routines here
}

void Homekit::loop () {
   if (is_disabled()) return;
    homeSpan.poll();
}

void Homekit::reset (const bool verbose) {
    homeSpan.setLogLevel(2);
    homeSpan.processSerialCommand("F");
    homeSpan.setLogLevel(-1);
    Module::reset(verbose);  // this will restart the system
}

std::string Homekit::status (const bool verbose) const {
    homeSpan.setLogLevel(2);
    homeSpan.processSerialCommand("s");
    homeSpan.setLogLevel(-1);
    return Module::status(verbose);
    // do your custom routines here
}

// other methods
Homekit* Homekit::instance = nullptr;
void Homekit::status_callback(HS_STATUS s) {
    Serial.printf("\n*** HOMESPAN STATUS CHANGE: %s\n",homeSpan.statusString(s));
    Serial.println(s);
  if (instance) {
    instance->hs_status = static_cast<uint8_t>(s);
    Serial.println(instance->hs_status);
  }
}
// make sure they have
// if (is_disabled()) return;
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
    std::array<uint8_t, 3> rgb = LedMode::hsv_to_rgb({h_byte, s_byte, 255});
    controller->sync_color(rgb, {1,1,1,0,1});  // V fixed at 255; brightness handled separately
    controller->sync_brightness(bri_byte, {1,1,1,0,1});
    controller->sync_state(state ? 1 : 0, {1,1,1,0,1});

    return true;
}