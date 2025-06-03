// Alexa.cpp
#include "Alexa.h"
#include "../../SystemController/SystemController.h"

Alexa::Alexa(SystemController& controller_ref) : controller(controller_ref) {}

void Alexa::begin(WebServer& server_instance) { // WebServer here refers to ESP32 core WebServer
    DBG_PRINTLN(Alexa, "Initializing Espalexa...");
    espalexa.begin(&server_instance);

    smart_light_device_ = new EspalexaDevice(
        "Smart Light",
        [this](EspalexaDevice* d_ptr) { this->handle_smart_light_change(d_ptr); },
        EspalexaDeviceType::extendedcolor // This type supports on/off, brightness, and color
    );

    if (smart_light_device_) {
        espalexa.addDevice(smart_light_device_);
        DBG_PRINTLN(Alexa, "Smart Light device added to Espalexa.");
    } else {
        DBG_PRINTLN(Alexa, "ERROR: Failed to create Espalexa device!");
    }
}

void Alexa::loop() {
    espalexa.loop();
}

void Alexa::handle_smart_light_change(EspalexaDevice* device_ptr) {
    if (!device_ptr) {
        DBG_PRINTLN(Alexa, "handle_smart_light_change: Null device pointer!");
        return;
    }

    DBG_PRINTF(Alexa, "Alexa command for device: %s (ID %d), LastChangedProp: %d\n", device_ptr->getName().c_str(), device_ptr->getId(), (int)device_ptr->getLastChangedProperty());

    controller.led_strip_set_state(device_ptr->getState() ? "1" : "0");
    controller.led_strip_set_brightness(String(device_ptr->getValue()));
    controller.led_strip_set_rgb(String(device_ptr->getR()) + " " + String(device_ptr->getG()) + " " + String(device_ptr->getB()));

    DBG_PRINTF(Alexa, "Applying to SystemController: Brightness=%d, ColorRGB=(%d,%d,%d)\n", device_ptr->getValue(), device_ptr->getR(), device_ptr->getG(), device_ptr->getB());
}

void Alexa::sync_state_with_system_controller() {
    if (!smart_light_device_) {
        DBG_PRINTLN(Alexa, "sync_state_with_system_controller: Smart Light device not initialized!");
        return;
    }

    DBG_PRINTLN(Alexa, "sync_state_with_system_controller()");

    std::array<uint8_t, 3> rgb_color = controller.led_strip_get_target_rgb();
    smart_light_device_->setColor(rgb_color[0], rgb_color[1], rgb_color[2]);
    smart_light_device_->setState(controller.led_strip_get_state());
    smart_light_device_->setValue(controller.led_strip_get_brightness());
    smart_light_device_->setState(controller.led_strip_get_state());
}