#include "Alexa.h"
#include "../../SystemController/SystemController.h"

/**
 * @brief Constructor for the Alexa module.
 */
Alexa::Alexa(SystemController& controller_ref) : ControllerModule(controller_ref) {
}

/**
 * @brief Initializes the Espalexa service using a void* context for the WebServer.
 */
void Alexa::begin(void* context) {
    DBG_PRINTLN(Alexa, "begin(): Initializing Espalexa...");

    // A null check is critical when using a void pointer context.
    if (!context) {
        DBG_PRINTLN(Alexa, "begin(): ERROR - WebServer context is null! Cannot start Espalexa.");
        return;
    }

    // Cast the void pointer to the required WebServer pointer.
    // This is an unsafe operation that relies on the caller to pass the correct type.
    WebServer* server_instance = static_cast<WebServer*>(context);

    espalexa.begin(server_instance);
    DBG_PRINTF(Alexa, "begin(): Espalexa WebServer initialized with instance at %p.\n", (void*)server_instance);

    const char* deviceName = "Smart Light";

    if (device) {
        delete device;
        device = nullptr;
    }

    device = new EspalexaDevice(
        deviceName,
        [this](EspalexaDevice* d) { this->change_event(d); },
        EspalexaDeviceType::color
    );

    if (device) {
        espalexa.addDevice(device);
        DBG_PRINTF(Alexa, "begin(): Smart Light device added to Espalexa. Device ID: %d.\n", device->getId());
    } else {
        DBG_PRINTLN(Alexa, "begin(): ERROR - Failed to create Espalexa device!");
    }
}

/**
 * @brief Main loop function for Espalexa.
 */
void Alexa::loop() {
    espalexa.loop();
}


/**
 * @brief Handles incoming change requests from the Alexa service.
 */
void Alexa::change_event(EspalexaDevice* device_ptr) {
    if (!device_ptr) {
        DBG_PRINTLN(Alexa, "change_event: FAILED - Null device pointer.");
        return;
    }

    bool state_from_alexa = device_ptr->getState();
    uint8_t brightness_from_alexa = device_ptr->getValue();
    uint8_t r_val = device_ptr->getR();
    uint8_t g_val = device_ptr->getG();
    uint8_t b_val = device_ptr->getB();

    controller.led_strip_set_state(state_from_alexa, {true, true, false, true});
    controller.led_strip_set_brightness(brightness_from_alexa, {true, true, false, true});
    controller.led_strip_set_rgb({r_val, g_val, b_val}, {true, true, false, true});
}

// ~~~~~~~~~~~~~~~~~~ Sync Methods (System -> Alexa) ~~~~~~~~~~~~~~~~~~

void Alexa::sync_color(std::array<uint8_t, 3> color) {
    if (!device) return;
    device->setColor(color[0], color[1], color[2]);
}

void Alexa::sync_brightness(uint8_t brightness) {
    if (!device) return;
    device->setValue(brightness);
}

void Alexa::sync_state(bool state) {
    if (!device) return;
    device->setState(state);
}

void Alexa::sync_mode(uint8_t mode_id, String mode_name) {
    // No action for Alexa.
    (void)mode_id;
    (void)mode_name;
}

void Alexa::sync_length(uint16_t length) {
    // No action for Alexa.
    (void)length;
}

void Alexa::sync_all(std::array<uint8_t, 3> color, uint8_t brightness, bool state,
                     uint8_t mode_id, String mode_name, uint16_t length) {
    if (!device) return;
    sync_state(state);
    sync_brightness(brightness);
    sync_color(color);
    sync_mode(mode_id, mode_name);
    sync_length(length);
}

//todo
void Alexa::status() {
//    homeSpan.processSerialCommand('s');
}
void Alexa::reset() {
    espalexa.setDiscoverable(false);
}