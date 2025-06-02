// Alexa.cpp
#include "Alexa.h"
#include "../../SystemController/SystemController.h" // Needs full definition for controller_ methods

Alexa::Alexa(SystemController& controller_ref) : controller_(controller_ref) {
    // Constructor
}

void Alexa::begin(::WebServer& server_instance) {
    DBG_PRINTLN(Alexa, "Initializing Espalexa...");

    espalexa_.begin(&server_instance);

    smart_light_device_ = new EspalexaDevice(
        "Smart Light",
        [this](EspalexaDevice* d_ptr) { this->handle_smart_light_change(d_ptr); },
        EspalexaDeviceType::extendedcolor
    );

    if (smart_light_device_) {
        espalexa_.addDevice(smart_light_device_);
        DBG_PRINTLN(Alexa, "Smart Light device added to Espalexa.");
        sync_state_with_system_controller(); // Set initial state
    } else {
        DBG_PRINTLN(Alexa, "ERROR: Failed to create Espalexa device!");
    }
}

void Alexa::loop() {
    espalexa_.loop();
}

void Alexa::handle_smart_light_change(EspalexaDevice* device_ptr) {
    if (!device_ptr) {
        DBG_PRINTLN(Alexa, "handle_smart_light_change: Null device pointer!");
        return;
    }

    DBG_PRINTF(Alexa, "Alexa command for device ID %d: %s\n", device_ptr->getId(), device_ptr->getName().c_str());

    bool target_state_on = device_ptr->getState();
    uint8_t target_brightness = device_ptr->getBrightness();

    DBG_PRINTF(Alexa, "Target state: %s, Target brightness: %d\n", target_state_on ? "ON" : "OFF", target_brightness);
    controller_.led_strip_set_state(target_state_on ? "1" : "0");

    if (target_state_on) {
        controller_.led_strip_set_brightness(String(target_brightness));

        EspalexaColor espalexa_color_val = device_ptr->getColor();
        DBG_PRINTF(Alexa, "Target color RGB: %d, %d, %d\n", espalexa_color_val.R, espalexa_color_val.G, espalexa_color_val.B);
        controller_.led_strip_set_rgb(String(espalexa_color_val.R) + " " + String(espalexa_color_val.G) + " " + String(espalexa_color_val.B));
    }

    // SystemController should call alexa_module.sync_state_with_system_controller()
    // after its state is fully updated, which will then update the EspalexaDevice.
    // This design relies on that feedback loop.
}

void Alexa::sync_state_with_system_controller() {
    if (!smart_light_device_) {
        DBG_PRINTLN(Alexa, "sync_state_with_system_controller: Smart Light device not initialized!");
        return;
    }

    bool current_sys_state = controller_.led_strip_get_state();
    uint8_t current_sys_brightness = controller_.led_strip_get_brightness();
    String hex_color_str = controller_.led_strip_get_color_hex(); // Format: "#RRGGBB"

    uint8_t r_val = 0, g_val = 0, b_val = 0; // Using _val to distinguish from potential single letter vars
    if (hex_color_str.length() == 7 && hex_color_str.startsWith("#")) {
        long color_value_from_hex = strtol(hex_color_str.c_str() + 1, nullptr, 16); // Skip '#'
        r_val = (color_value_from_hex >> 16) & 0xFF;
        g_val = (color_value_from_hex >> 8) & 0xFF;
        b_val = color_value_from_hex & 0xFF;
    }

    smart_light_device_->setState(current_sys_state);
    smart_light_device_->setBrightness(current_sys_brightness);
    smart_light_device_->setColor(r_val, g_val, b_val);

    DBG_PRINTF(Alexa, "Synced EspalexaDevice state: Power=%s, Brightness=%d, Color=(%d,%d,%d)\n",
               current_sys_state ? "ON" : "OFF", current_sys_brightness, r_val, g_val, b_val);
}