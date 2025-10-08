//#pragma once
//
//#include "../../Interface/Interface.h"
//#include "../../../Debug.h"
//
//// Espalexa / WebServer
//#include <Espalexa.h>
//#include <WebServer.h>
//
//#include <array>
//#include <string>
//
//// Forward declaration to prevent circular dependencies
//class SystemController;
//
//// Optional per‑module configuration, passed into begin()
//struct AlexaConfig : public ModuleConfig {
//    WebServer*  server      = nullptr;           // REQUIRED: running WebServer instance
//    std::string device_name      = "XeWe Lights";
//};
//
///**
// * @class Alexa
// * @brief Integration with Amazon Alexa using the Espalexa library.
// *
// * Notes for migration:
// *  - Inherits from Interface to match the new architecture.
// *  - All sync_* signatures are mapped to Interface (state and mode are uint8_t).
// *  - begin(const ModuleConfig&) expects an AlexaConfig; will validate via dynamic_cast.
// */
//class Alexa : public Interface {
//public:
//    explicit Alexa(SystemController& controller_ref);
//
//    // ~~~~~~~~~~~~~ Module overrides ~~~~~~~~~~~~~
//    void begin (const ModuleConfig& cfg) override;
//    void loop  () override;
//    void reset (bool verbose = false) override;
//
//    // ~~~~~~~~~~~~~ Interface (required) overrides ~~~~~~~~~~~~~
//    void sync_color      (std::array<uint8_t,3> color) override;
//    void sync_brightness (uint8_t brightness)          override;
//    void sync_state      (uint8_t state)               override; // 0 = off, non‑zero = on
//    void sync_mode       (uint8_t mode)                override; // not used by Alexa
//    void sync_length     (uint16_t length)             override; // not used by Alexa
//    void sync_all        (std::array<uint8_t,3> color,
//                          uint8_t brightness,
//                          uint8_t state,
//                          uint8_t mode,
//                          uint16_t length)             override;
//
//    // Convenience (retains old public accessor)
//    Espalexa& get_instance() { return espalexa; }
//
//private:
//    // Callback from Espalexa when the user changes the device via Alexa
//    void change_event(EspalexaDevice* device_ptr);
//
//private:
//    Espalexa        espalexa;
//    EspalexaDevice* device = nullptr;
//
//    // cached for diagnostics
//    std::string configured_device_name = "Smart Light";
//};
