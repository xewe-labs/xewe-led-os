// Alexa.cpp
#include "Alexa.h"
#include "../../SystemController/SystemController.h"

Alexa::Alexa(SystemController& controller_ref) : controller_(controller_ref) {}

void Alexa::begin(WebServer& server_instance) { // WebServer here refers to ESP32 core WebServer
    DBG_PRINTLN(Alexa, "Initializing Espalexa...");
    espalexa_.begin(&server_instance);

    smart_light_device_ = new EspalexaDevice(
        "Smart Light",
        [this](EspalexaDevice* d_ptr) { this->handle_smart_light_change(d_ptr); },
        EspalexaDeviceType::extendedcolor // This type supports on/off, brightness, and color
    );

    if (smart_light_device_) {
        espalexa_.addDevice(smart_light_device_);
        DBG_PRINTLN(Alexa, "Smart Light device added to Espalexa.");
        sync_state_with_system_controller(); // Set initial state on the EspalexaDevice
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

    DBG_PRINTF(Alexa, "Alexa command for device: %s (ID %d), LastChangedProp: %d\n",
        device_ptr->getName().c_str(), device_ptr->getId(), (int)device_ptr->getLastChangedProperty());

    bool target_state_on = device_ptr->getState();
    uint8_t brightness_from_alexa = device_ptr->getValue();

    DBG_PRINTF(Alexa, "Alexa wants: State=%s, BrightnessValue(getValue)=%d\n", target_state_on ? "ON" : "OFF", brightness_from_alexa);

    controller_.led_strip_set_state(target_state_on ? "1" : "0");

    if (target_state_on) {
        uint8_t effective_brightness = brightness_from_alexa;
        if (brightness_from_alexa == 0) {
            effective_brightness = device_ptr->getLastValue();
            if (effective_brightness == 0) effective_brightness = 255;
        }

        controller_.led_strip_set_brightness(String(effective_brightness));

        uint8_t r_val = device_ptr->getR();
        uint8_t g_val = device_ptr->getG();
        uint8_t b_val = device_ptr->getB();

        DBG_PRINTF(Alexa, "Applying to SystemController: Brightness=%d, ColorRGB=(%d,%d,%d)\n", effective_brightness, r_val, g_val, b_val);
        controller_.led_strip_set_rgb(String(r_val) + " " + String(g_val) + " " + String(b_val));
    }
}

void Alexa::sync_state_with_system_controller() {
    if (!smart_light_device_) {
        DBG_PRINTLN(Alexa, "sync_state_with_system_controller: Smart Light device not initialized!");
        return;
    }

    bool current_sys_state = controller_.led_strip_get_state();
    uint8_t current_sys_brightness = controller_.led_strip_get_brightness();
    String hex_color_str = controller_.led_strip_get_color_hex();

    uint8_t r_val = 0, g_val = 0, b_val = 0;
    if (hex_color_str.length() == 7 && hex_color_str.startsWith("#")) {
        long color_value_from_hex = strtol(hex_color_str.c_str() + 1, nullptr, 16);
        r_val = (color_value_from_hex >> 16) & 0xFF;
        g_val = (color_value_from_hex >> 8) & 0xFF;
        b_val = color_value_from_hex & 0xFF;
    }

    smart_light_device_->setState(current_sys_state);

    if (current_sys_state) {
        smart_light_device_->setValue(current_sys_brightness == 0 ? 1 : current_sys_brightness);
    } else {
        smart_light_device_->setValue(0);
    }

    smart_light_device_->setColor(r_val, g_val, b_val);

    DBG_PRINTF(Alexa, "Synced EspalexaDevice: State=%s, ValueSet=%d (eff.SysBri=%d), SysColorRGB=(%d,%d,%d)\n",
               current_sys_state ? "ON" : "OFF",
               smart_light_device_->getValue(),
               current_sys_brightness,
               r_val, g_val, b_val);
}