#include "Alexa.h"
#include "../../SystemController/SystemController.h"

// Initialize the controller pointer to nullptr
Alexa::Alexa() : controller(nullptr) {}

// The begin method now takes both dependencies
void Alexa::begin(SystemController& controller_ref, WebServer* server_instance) {
    // Store the address of the controller object
    this->controller = &controller_ref;

    DBG_PRINTLN(Alexa, "begin(): Initializing Espalexa (Sync Mode).");
    espalexa.begin(server_instance);
    DBG_PRINTF(Alexa, "begin(): Espalexa WebServer initialized with instance at %p.\n", server_instance);

    const char* deviceName = "Smart Light";
    EspalexaDeviceType deviceType = EspalexaDeviceType::color;
    DBG_PRINTF(Alexa, "begin(): Creating EspalexaDevice: Name='%s', Type=%d.\n", deviceName, (int)deviceType);

    smart_light_device_ = new EspalexaDevice(
        deviceName,
        [this](EspalexaDevice* d_ptr) { this->handle_smart_light_change(d_ptr); },
        deviceType
    );

    if (smart_light_device_) {
        espalexa.addDevice(smart_light_device_);
        DBG_PRINTF(Alexa, "begin(): Smart Light device added to Espalexa. Device ID: %d.\n", smart_light_device_->getId());
    } else {
        DBG_PRINTLN(Alexa, "begin(): ERROR: Failed to create Espalexa device!");
    }
}

void Alexa::loop() {
    espalexa.loop();
}

void Alexa::handle_smart_light_change(EspalexaDevice* device_ptr) {
    // Add a null check for the controller pointer
    if (!device_ptr || !controller) {
        DBG_PRINTLN(Alexa, "handle_smart_light_change: FAILED - Null device or controller pointer.");
        return;
    }

    DBG_PRINTF(Alexa, "handle_smart_light_change: Device Name='%s', ID=%d, LastChangedProp=%d.\n", device_ptr->getName().c_str(), device_ptr->getId(), (int)device_ptr->getLastChangedProperty());

    bool target_state_on = device_ptr->getState();
    uint8_t brightness_from_alexa = device_ptr->getValue();
    uint8_t r_val = device_ptr->getR();
    uint8_t g_val = device_ptr->getG();
    uint8_t b_val = device_ptr->getB();

    // Use -> to access members via the controller pointer
    controller->led_strip_set_state(target_state_on, {true, false, true});
    controller->led_strip_set_brightness(brightness_from_alexa, {true, false, true});
    controller->led_strip_set_rgb({r_val, g_val, b_val}, {true, false, true});

    DBG_PRINTF(Alexa, "handle_smart_light_change: Applied to SystemController: State=%s, Brightness=%u, ColorRGB=(%u,%u,%u).\n",
        target_state_on ? "ON" : "OFF",
        brightness_from_alexa,
        r_val, g_val, b_val);
}

void Alexa::sync_state_with_system_controller(const char* field) {
    // Add a null check for the controller pointer
    if (!smart_light_device_ || !controller) {
        DBG_PRINTLN(Alexa, "sync_state_with_system_controller: FAILED - Device or controller not initialized.");
        return;
    }

    // ... (rest of the function is the same, but with controller-> instead of controller.)

    bool sync_color = false;
    bool sync_brightness = false;
    bool sync_state = false;

    if (strcmp(field, "color") == 0) {
        sync_color = true;
    } else if (strcmp(field, "brightness") == 0) {
        sync_brightness = true;
    } else if (strcmp(field, "state") == 0) {
        sync_state = true;
    } else {
        sync_color = true;
        sync_brightness = true;
        sync_state = true;
    }

    if (sync_color) {
        std::array<uint8_t, 3> rgb_color = controller->led_strip_get_target_rgb();
        smart_light_device_->setColor(rgb_color[0], rgb_color[1], rgb_color[2]);
    }

    if (sync_brightness) {
        uint8_t current_sys_brightness = controller->led_strip_get_brightness();
        smart_light_device_->setValue(current_sys_brightness);
    }

    if (sync_state) {
        bool current_sys_state = controller->led_strip_get_state();
        smart_light_device_->setState(current_sys_state);
    }
}