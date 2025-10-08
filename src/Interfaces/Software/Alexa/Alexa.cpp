//#include "Alexa.h"
//#include "../../../SystemController/SystemController.h"
//
//Alexa::Alexa(SystemController& controller_ref)
//: Interface(controller_ref, "alexa", "alexa", /*requires_init_setup*/false,
//            /*can_be_disabled*/true, /*has_cli_commands*/true)
//{
//    DBG_PRINTLN(Alexa, "Constructor called.");
//}
//
//void Alexa::begin(const ModuleConfig& cfg) {
//    DBG_PRINTLN(Alexa, "begin(): Initializing Espalexa...");
//    Module::begin(cfg);
//
//    // Expect an AlexaConfig here
//    auto alexa_cfg = dynamic_cast<const AlexaConfig*>(&cfg);
//    if (!alexa_cfg) {
//        DBG_PRINTLN(Alexa, "begin(): ERROR - invalid ModuleConfig type (expected AlexaConfig).");
//        return;
//    }
//
//    WebServer* server_instance = alexa_cfg->server;
//    configured_device_name     = alexa_cfg->device_name.empty()
//                               ? "Smart Light"
//                               : alexa_cfg->device_name;
//
//    if (!server_instance) {
//        DBG_PRINTLN(Alexa, "begin(): ERROR - WebServer* is null! Cannot start Espalexa.");
//        return;
//    }
//
//    // Start Espalexa on the provided server
//    espalexa.begin(server_instance);
//    DBG_PRINTF(Alexa, "begin(): Espalexa WebServer initialized with instance at %p.\n",
//               (void*)server_instance);
//
//    // Recreate device if needed
//    if (device) {
//        delete device;
//        device = nullptr;
//    }
//
//    device = new EspalexaDevice(
//        configured_device_name.c_str(),
//        [this](EspalexaDevice* d) { this->change_event(d); },
//        EspalexaDeviceType::color
//    );
//
//    if (device) {
//        espalexa.addDevice(device);
//        DBG_PRINTF(Alexa, "begin(): Device '%s' added. Device ID: %d.\n",
//                   configured_device_name.c_str(), device->getId());
//    } else {
//        DBG_PRINTLN(Alexa, "begin(): ERROR - Failed to create Espalexa device!");
//    }
//}
//
//void Alexa::loop() {
//    // Must be called frequently to handle Alexa traffic
//    espalexa.loop();
//}
//
//void Alexa::reset(bool verbose) {
//    // The previous implementation had "nothing to do"; keep behavior.
//    // If desired, you could delete the device here and re-add on next begin().
//    (void)verbose;
//    DBG_PRINTLN(Alexa, "reset(): Nothing to reset for Alexa module.");
//}
//
//// ~~~~~~~~~~~~~~~~~~ Espalexa -> System (callback) ~~~~~~~~~~~~~~~~~~
//
//void Alexa::change_event(EspalexaDevice* device_ptr) {
//    if (!device_ptr) {
//        DBG_PRINTLN(Alexa, "change_event(): FAILED - Null device pointer.");
//        return;
//    }
//
//    bool    state_from_alexa      = device_ptr->getState();
//    uint8_t brightness_from_alexa = device_ptr->getValue();
//    uint8_t r_val = device_ptr->getR();
//    uint8_t g_val = device_ptr->getG();
//    uint8_t b_val = device_ptr->getB();
//
//    // Preserve original propagation flags
//    controller.led_strip_set_state(state_from_alexa,          {true, true, false, true});
//    controller.led_strip_set_brightness(brightness_from_alexa,{true, true, false, true});
//    controller.led_strip_set_rgb({r_val, g_val, b_val},       {true, true, false, true});
//}
//
//// ~~~~~~~~~~~~~~~~~~ System -> Alexa (syncs) ~~~~~~~~~~~~~~~~~~
//
//void Alexa::sync_color(std::array<uint8_t,3> color) {
//    if (!device) return;
//    DBG_PRINTF(Alexa, "sync_color(): R=%u, G=%u, B=%u\n", color[0], color[1], color[2]);
//    device->setColor(color[0], color[1], color[2]);
//}
//
//void Alexa::sync_brightness(uint8_t brightness) {
//    if (!device) return;
//    DBG_PRINTF(Alexa, "sync_brightness(): brightness=%u\n", brightness);
//    device->setValue(brightness);
//}
//
//void Alexa::sync_state(uint8_t state) {
//    if (!device) return;
//    DBG_PRINTF(Alexa, "sync_state(): state=%s\n", state ? "ON" : "OFF");
//    device->setState(static_cast<bool>(state));
//}
//
//void Alexa::sync_mode(uint8_t mode) {
//    // Not exposed via Espalexa; intentionally no‑op
//    DBG_PRINTF(Alexa, "sync_mode(): (ignored) mode=%u\n", mode);
//    (void)mode;
//}
//
//void Alexa::sync_length(uint16_t length) {
//    // Not exposed via Espalexa; intentionally no‑op
//    DBG_PRINTF(Alexa, "sync_length(): (ignored) length=%u\n", length);
//    (void)length;
//}
//
//void Alexa::sync_all(std::array<uint8_t,3> color,
//                     uint8_t brightness,
//                     uint8_t state,
//                     uint8_t mode,
//                     uint16_t length)
//{
//    DBG_PRINTLN(Alexa, "sync_all(): Syncing all parameters to Alexa device.");
//    if (!device) {
//        DBG_PRINTLN(Alexa, "sync_all(): WARNING - device is null; skipping.");
//        return;
//    }
//    // Keep the same order as old module: state -> brightness -> color -> (mode, length)
//    sync_state(state);
//    sync_brightness(brightness);
//    sync_color(color);
//    sync_mode(mode);
//    sync_length(length);
//    DBG_PRINTLN(Alexa, "sync_all(): Sync complete.");
//}
