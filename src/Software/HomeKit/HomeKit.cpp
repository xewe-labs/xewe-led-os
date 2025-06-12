#include "HomeKit.h"
#include "../../SystemController/SystemController.h"

volatile bool homekit_values_sync = false;

// 1. Create a global variable to hold the current status.
// 'volatile' is good practice as it can be updated from a different code context.
volatile HS_STATUS current_HS_Status;

// 2. Define the callback function that HomeSpan will call on status changes.
// It simply updates our global variable. We also use homeSpan.statusString()
// which is a real function in the library for helpful debug prints.
void homespan_status_callback(HS_STATUS status){
  current_HS_Status = status;
  Serial.printf("\n*** HOMESPAN STATUS CHANGE: %s\n\n", homeSpan.statusString(status));
}


struct NeoPixel_RGB : Service::LightBulb {
    Characteristic::On            power   {0,   true};
    Characteristic::Hue           H       {0,   true};
    Characteristic::Saturation    S       {0,   true};
    Characteristic::Brightness    V       {100, true};

    SystemController& controller;

    NeoPixel_RGB(SystemController& controller_ref) :
        Service::LightBulb(),
        controller(controller_ref) {

        V.setRange(5,100,1);
    }

    boolean update() override {
        bool state = power.getNewVal();
        uint8_t h_byte = (H.getNewVal<float>() / 360.0) * 255.0;
        uint8_t s_byte = (S.getNewVal<float>() / 100.0) * 255.0;
        uint8_t brightness = (V.getNewVal<float>() / 100.0) * 255.0;

        controller.led_strip_set_state(state, {true, true, true});
        controller.led_strip_set_brightness(brightness, {true, true, true});
        controller.led_strip_set_hsv({h_byte, s_byte, 255}, {true, true, true});

        return true;
    }

    void loop () {
        if (!homekit_values_sync)
            return;
        homekit_values_sync = false;

        std::array<uint8_t, 3> hsv_color = controller.led_strip_get_target_hsv();
        float current_hue = ceil(hsv_color[0] / 255.0f * 360.0f);
        float current_sat = ceil(hsv_color[1] / 255.0f * 100.0f);
        float current_brightness = ceil(controller.led_strip_get_brightness() / 255.0f * 100.0f);
        bool current_sys_state = controller.led_strip_get_state();

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

HomeKit::HomeKit(SystemController& controller_ref) : controller(controller_ref) {}

void HomeKit::begin() {
    homeSpan.setPortNum(1201); // change port number for HomeSpan so we can use port 80 for the Web Server
    homeSpan.setStatusCallback(homespan_status_callback);

    homeSpan.begin(Category::Lighting,"XeWe Lights");
    SPAN_ACCESSORY();
    SPAN_ACCESSORY("RGB Lights");
    new NeoPixel_RGB(controller);
}

void HomeKit::loop() {
    homeSpan.poll();
}

void HomeKit::sync_state() {
    homekit_values_sync = true;
}

bool HomeKit::is_paired() {
    return current_HS_Status == HS_PAIRED;
}