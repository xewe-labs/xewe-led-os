#include "HomeKit.h"
#include "../../SystemController/SystemController.h"

volatile bool homekit_values_sync = false;

// Service definition is updated to use a pointer
struct NeoPixel_RGB : Service::LightBulb {
    Characteristic::On            power   {0,   true};
    Characteristic::Hue           H       {0,   true};
    Characteristic::Saturation    S       {0,   true};
    Characteristic::Brightness    V       {100, true};

    SystemController* controller; // Store a pointer

    // Constructor now accepts a pointer
    NeoPixel_RGB(SystemController* controller_ptr) :
        Service::LightBulb(),
        controller(controller_ptr) { // Assign the pointer

        V.setRange(5,100,1);
    }

    boolean update() override {
        if (!controller) return false;

        bool state = power.getNewVal();
        uint8_t h_byte = (H.getNewVal<float>() / 360.0) * 255.0;
        uint8_t s_byte = (S.getNewVal<float>() / 100.0) * 255.0;
        uint8_t brightness = (V.getNewVal<float>() / 100.0) * 255.0;

        controller->led_strip_set_state(state, {true, true, true});
        controller->led_strip_set_brightness(brightness, {true, true, true});
        controller->led_strip_set_hsv({h_byte, s_byte, 255}, {true, true, true});

        return true;
    }

    void loop () {
        if (!homekit_values_sync)
            return;
        homekit_values_sync = false;

        // Use -> to access members via pointer
        if(!controller) return;

        std::array<uint8_t, 3> hsv_color = controller->led_strip_get_target_hsv();
        float current_hue = round(hsv_color[0] / 255.0f * 360.0f);
        float current_sat = round(hsv_color[1] / 255.0f * 100.0f);
        float current_brightness = round(controller->led_strip_get_brightness() / 255.0f * 100.0f);
        bool current_sys_state = controller->led_strip_get_state();

        DBG_PRINTF(
            HomeKit,
            "Syncing state to HomeKit: State=%d, H=%.1f, S=%.1f, V=%.1f\n",
            current_sys_state,
            current_hue,
            current_sat,
            current_brightness
        );

        power.setVal(current_sys_state);
        H.setVal(current_hue);
        S.setVal(current_sat);
        V.setVal(current_brightness);
    }
};

// Implement the new empty constructor, initializing the pointer to null
HomeKit::HomeKit() : controller(nullptr) {}

// Implement the new begin() method
void HomeKit::begin(SystemController& controller_ref) {
    // 1. Store the address of the controller object
    this->controller = &controller_ref;

    // 2. Perform all original initialization logic
    homeSpan.setPortNum(1201);
//    homeSpan.setStatusCallback(homespan_status_callback);
//    homeSpan.setGetCharacteristicsCallback(homespan_getchars_callback);
    homeSpan.setSerialInputDisable(true);
    homeSpan.setLogLevel(-1);
    homeSpan.begin(Category::Lighting,"XeWe Lights");

    SPAN_ACCESSORY();
    SPAN_ACCESSORY("RGB Lights");

    // 3. Pass the controller pointer to the service
    new NeoPixel_RGB(this->controller);
}

void HomeKit::loop() {
    homeSpan.poll();
}

void HomeKit::sync_state() {
    homekit_values_sync = true;
}
