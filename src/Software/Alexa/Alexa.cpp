#include "Alexa.h"
#include "../../SystemController/SystemController.h"

Alexa::Alexa(SystemController& controller_ref) : controller(controller_ref) {}

void Alexa::begin(AsyncWebServer& server_instance) {
    DBG_PRINTLN(Alexa, "begin(): Initializing Espalexa (Async Mode).");
    espalexa.begin(&server_instance); // Espalexa handles the AsyncWebServer pointer
    DBG_PRINTF(Alexa, "begin(): Espalexa AsyncWebServer initialized with instance at %p.\n", &server_instance);

    const char* deviceName = "Smart Light";
    EspalexaDeviceType deviceType = EspalexaDeviceType::extendedcolor;
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
    espalexa.loop(); // Espalexa::loop() is still needed for UDP discovery, etc.
}

void Alexa::handle_smart_light_change(EspalexaDevice* device_ptr) {
    if (!device_ptr) {
        DBG_PRINTLN(Alexa, "handle_smart_light_change: FAILED - Null device pointer.");
        return;
    }

    DBG_PRINTF(Alexa, "handle_smart_light_change: Device Name='%s', ID=%d, LastChangedProp=%d.\n", device_ptr->getName().c_str(), device_ptr->getId(), (int)device_ptr->getLastChangedProperty());
    DBG_PRINTF(Alexa, "handle_smart_light_change: Incoming Alexa data: State(raw)=%s, Brightness(raw)=%u, R(raw)=%u, G(raw)=%u, B(raw)=%u.\n",
        device_ptr->getState() ? "true" : "false",
        device_ptr->getValue(),
        device_ptr->getR(),
        device_ptr->getG(),
        device_ptr->getB());

    bool target_state_on = device_ptr->getState();
    uint8_t brightness_from_alexa = device_ptr->getValue();
    uint8_t r_val = device_ptr->getR();
    uint8_t g_val = device_ptr->getG();
    uint8_t b_val = device_ptr->getB();

    DBG_PRINTF(Alexa, "handle_smart_light_change: SystemController set_state call with: '%s'.\n", target_state_on ? "1" : "0");
    controller.led_strip_set_state(target_state_on ? "1" : "0");

    DBG_PRINTF(Alexa, "handle_smart_light_change: SystemController set_brightness call with: '%s'.\n", String(brightness_from_alexa).c_str());
    controller.led_strip_set_brightness(String(brightness_from_alexa));

    DBG_PRINTF(Alexa, "handle_smart_light_change: SystemController set_rgb call with: R=%u G=%u B=%u.\n", r_val, g_val, b_val);
    controller.led_strip_set_rgb(String(r_val) + " " + String(g_val) + " " + String(b_val));

    DBG_PRINTF(Alexa, "handle_smart_light_change: Applied to SystemController: State=%s, Brightness=%u, ColorRGB=(%u,%u,%u).\n",
        target_state_on ? "ON" : "OFF",
        brightness_from_alexa,
        r_val, g_val, b_val);
}

void Alexa::sync_state_with_system_controller(const char* field) {
    if (!smart_light_device_) {
        DBG_PRINTLN(Alexa, "sync_state_with_system_controller: FAILED - Smart Light device not initialized.");
        return;
    }

    DBG_PRINTF(Alexa, "sync_state_with_system_controller: Executing for field: '%s'.\n", field);

    bool sync_color = false;
    bool sync_brightness = false;
    bool sync_state = false;

    if (strcmp(field, "full") == 0) {
        DBG_PRINTLN(Alexa, "sync_state_with_system_controller: Syncing all fields (full).");
        sync_color = true;
        sync_brightness = true;
        sync_state = true;
    } else if (strcmp(field, "color") == 0) {
        DBG_PRINTLN(Alexa, "sync_state_with_system_controller: Syncing 'color' field.");
        sync_color = true;
    } else if (strcmp(field, "brightness") == 0) {
        DBG_PRINTLN(Alexa, "sync_state_with_system_controller: Syncing 'brightness' field.");
        sync_brightness = true;
    } else if (strcmp(field, "state") == 0) {
        DBG_PRINTLN(Alexa, "sync_state_with_system_controller: Syncing 'state' field.");
        sync_state = true;
    } else {
        DBG_PRINTF(Alexa, "sync_state_with_system_controller: Unknown field '%s', performing full sync as fallback.\n", field);
        sync_color = true;
        sync_brightness = true;
        sync_state = true;
    }

    if (sync_color) {
        std::array<uint8_t, 3> rgb_color = controller.led_strip_get_target_rgb();
        DBG_PRINTF(Alexa, "sync_state_with_system_controller: SystemController RGB: (%u,%u,%u).\n", rgb_color[0], rgb_color[1], rgb_color[2]);
        if (smart_light_device_->getR() != rgb_color[0] || smart_light_device_->getG() != rgb_color[1] || smart_light_device_->getB() != rgb_color[2]) {
            smart_light_device_->setColor(rgb_color[0], rgb_color[1], rgb_color[2]);
            DBG_PRINTF(Alexa, "sync_state_with_system_controller: Set EspalexaDevice color to R=%u, G=%u, B=%u.\n", rgb_color[0], rgb_color[1], rgb_color[2]);
        } else {
            DBG_PRINTLN(Alexa, "sync_state_with_system_controller: EspalexaDevice color already matches SystemController. No change.");
        }
    }

    if (sync_brightness) {
        uint8_t current_sys_brightness = controller.led_strip_get_brightness();
        DBG_PRINTF(Alexa, "sync_state_with_system_controller: SystemController Brightness: %u.\n", current_sys_brightness);
        if (smart_light_device_->getValue() != current_sys_brightness) {
            smart_light_device_->setValue(current_sys_brightness);
            DBG_PRINTF(Alexa, "sync_state_with_system_controller: Set EspalexaDevice value (brightness) to %u.\n", current_sys_brightness);
        } else {
            DBG_PRINTLN(Alexa, "sync_state_with_system_controller: EspalexaDevice brightness already matches SystemController. No change.");
        }
    }

    if (sync_state) {
        bool current_sys_state = controller.led_strip_get_state();
        DBG_PRINTF(Alexa, "sync_state_with_system_controller: SystemController State: %s.\n", current_sys_state ? "ON" : "OFF");
        if (smart_light_device_->getState() != current_sys_state) {
            smart_light_device_->setState(current_sys_state);
            DBG_PRINTF(Alexa, "sync_state_with_system_controller: Set EspalexaDevice state to %s.\n", current_sys_state ? "ON" : "OFF");
        } else {
            DBG_PRINTLN(Alexa, "sync_state_with_system_controller: EspalexaDevice state already matches SystemController. No change.");
        }
    }

    DBG_PRINTF(Alexa, "sync_state_with_system_controller: EspalexaDevice final state after sync attempt: Name='%s', ID=%d, State=%s, Brightness=%u, R=%u, G=%u, B=%u.\n",
        smart_light_device_->getName().c_str(),
        smart_light_device_->getId(),
        smart_light_device_->getState() ? "ON" : "OFF",
        smart_light_device_->getValue(),
        smart_light_device_->getR(),
        smart_light_device_->getG(),
        smart_light_device_->getB());
}
