#include "HomeKit.h"
#include "../../SystemController/SystemController.h"

// ~~~~~~~~~~~~~~~~~~ HomeKit Service Definition ~~~~~~~~~~~~~~~~~~

// Service definition is updated to use a pointer
struct NeoPixel_RGB : Service::LightBulb {
    Characteristic::On            power   {0,   true};
    Characteristic::Hue           H       {0,   true};
    Characteristic::Saturation    S       {0,   true};
    Characteristic::Brightness    V       {100, true};

    SystemController* controller; // Store a pointer

    // Constructor stores the controller pointer
    NeoPixel_RGB(SystemController* controller_ptr) :
        Service::LightBulb(), controller(controller_ptr) {
        V.setRange(5, 100, 1); // Set brightness range
    }

    // This function is called by HomeSpan when a characteristic is updated from a Home client
    boolean update() override {
        if (!controller) return false;

        // Get new values from HomeKit
        bool state = power.getNewVal();
        uint8_t h_byte = (H.getNewVal<float>() / 360.0) * 255.0;
        uint8_t s_byte = (S.getNewVal<float>() / 100.0) * 255.0;
        uint8_t brightness = (V.getNewVal<float>() / 100.0) * 255.0;

        controller->led_strip_set_state(state, {true, true, true, false});
        controller->led_strip_set_brightness(brightness, {true, true, true, false});
        controller->led_strip_set_hsv({h_byte, s_byte, 255}, {true, true, true, false});

        return true;
    }
};

// ~~~~~~~~~~~~~~~~~~ HomeKit Class Implementation ~~~~~~~~~~~~~~~~~~

/**
 * @brief Constructor for the HomeKit module.
 */
HomeKit::HomeKit(SystemController& controller_ref) : ControllerModule(controller_ref) {
}

/**
 * @brief Initializes the HomeSpan service and creates the accessory.
 */
void HomeKit::begin(void* context) {
    (void)context;

    DBG_PRINTLN(HomeKit, "begin(): Initializing HomeSpan...");
    homeSpan.setPortNum(1201); // Use a different port if 80 is in use
    homeSpan.setSerialInputDisable(true);
    homeSpan.setLogLevel(-1); // Set to 1 for more verbose logging
    homeSpan.begin(Category::Lighting,"XeWe Lights");

    SPAN_ACCESSORY();
    SPAN_ACCESSORY("RGB Lights");

    device = new NeoPixel_RGB(&controller);

    DBG_PRINTLN(HomeKit, "begin(): HomeSpan initialization complete.");
}

/**
 * @brief Main loop function for HomeSpan.
 */
void HomeKit::loop() {
    homeSpan.poll();
}

/**
 * @brief Resets HomeKit by deleting all pairings and WiFi credentials.
 */
void HomeKit::reset() {
    DBG_PRINTLN(HomeKit, "reset(): Deleting all HomeSpan pairings and WiFi credentials.");
//    homeSpan.deleteEverything();
    delay(500);
    ESP.restart();
}

// ~~~~~~~~~~~~~~~~~~ Sync Methods (System -> HomeKit) ~~~~~~~~~~~~~~~~~~

void HomeKit::sync_color(std::array<uint8_t, 3> color) {
    if (!device) return;

    // Convert system RGB to HSV for HomeKit
    float current_hue = round(color[0] / 255.0f * 360.0f);
    float current_sat = round(color[1] / 255.0f * 100.0f);
    // Update the characteristics without triggering a loopback `update()` call
    device->H.setVal(current_hue);
    device->S.setVal(current_sat);

    DBG_PRINTF(HomeKit, "sync_color: Synced HSV(%f,%f,%f) to HomeKit.\n", current_hue, current_sat, 100.0f);
}

void HomeKit::sync_brightness(uint8_t brightness) {
    if (!device) return;

    float current_brightness = round(brightness / 255.0f * 100.0f);
    device->V.setVal(current_brightness);

    DBG_PRINTF(HomeKit, "sync_brightness: Synced brightness %u as V(%.1f) to HomeKit.\n", brightness, current_brightness);
}

void HomeKit::sync_state(bool state) {
    if (!device) return;

    device->power.setVal(state);

    DBG_PRINTF(HomeKit, "sync_state: Synced state (%s) to HomeKit.\n", state ? "ON" : "OFF");
}

void HomeKit::sync_mode(uint8_t mode_id, String mode_name) {
    // HomeKit does not have a concept of "modes", so this is ignored.
    (void)mode_id;
    (void)mode_name;
}

void HomeKit::sync_length(uint16_t length) {
    // HomeKit does not need to know the strip length, so this is ignored.
    (void)length;
}

void HomeKit::sync_all(std::array<uint8_t, 3> color, uint8_t brightness, bool state,
                     uint8_t mode_id, String mode_name, uint16_t length) {
    if (!device) {
        DBG_PRINTLN(HomeKit, "sync_all: FAILED - Service not initialized.");
        return;
    }
    DBG_PRINTLN(HomeKit, "sync_all: Performing full state synchronization to HomeKit.");

    // Call individual sync methods
    sync_state(state);
    sync_brightness(brightness);
    sync_color(color);

    // These are ignored but called for interface completeness
    sync_mode(mode_id, mode_name);
    sync_length(length);
}
