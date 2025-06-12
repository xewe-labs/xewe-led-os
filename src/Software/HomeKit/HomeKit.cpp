#include "HomeKit.h"
#include "../../SystemController/SystemController.h"

struct NeoPixel_RGB : Service::LightBulb {      // Addressable single-wire RGB LED Strand (e.g. NeoPixel)
  Characteristic::On power{0,true};
  Characteristic::Hue H{0,true};
  Characteristic::Saturation S{0,true};
  Characteristic::Brightness V{100,true};
//  Pixel *pixel;
//  int nPixels;
  SystemController& controller;

    NeoPixel_RGB(SystemController& controller_ref) :
      Service::LightBulb(),
      controller(controller_ref)
    {
        V.setRange(5,100,1);                      // sets the range of the Brightness to be from a min of 5%, to a max of 100%, in steps of 1%
    }

    boolean update() override {
        int p = power.getNewVal();
        float h = H.getNewVal<float>();       // range = [0,360]
        float s = S.getNewVal<float>();       // range = [0,100]
        float v = V.getNewVal<float>();       // range = [0,100]

        bool state = p;
        uint8_t h_byte = (h / 360.0) * 255.0;
        uint8_t s_byte = (s / 100.0) * 255.0;     // <-- BUG FIX: Was using 'h' here
        uint8_t brightness = (v / 100.0) * 255.0;

        Serial.printf("HOMEKIT UPDATE: h=%.1f, s=%.1f, v=%.1f, p=%d\n", h, s, v, p);
        Serial.printf("HOMEKIT BYTE CONVERSION: h=%u, s=%u, v=%u, p=%u\n", h_byte, s_byte, brightness, state);

        controller.led_strip_set_state(state, {true, true, false});
        controller.led_strip_set_brightness(brightness, {true, true, false});
        controller.led_strip_set_hsv(h_byte, s_byte, 255}, {true, true, false});
        return(true);
    }
};

HomeKit::HomeKit(SystemController& controller_ref) : controller(controller_ref) {}

void HomeKit::begin() {
    homeSpan.setLogLevel(1);

    homeSpan.setHostNameSuffix("");         // use null string for suffix (rather than the HomeSpan device ID)
    homeSpan.setPortNum(1201);              // change port number for HomeSpan so we can use port 80 for the Web Server
//    homeSpan.enableOTA();                   // enable OTA updates

    homeSpan.begin(Category::Lighting,"Xewe Lights");
    SPAN_ACCESSORY();
    SPAN_ACCESSORY("RGB Lights");
        new NeoPixel_RGB(controller);                       // create 8-LED NeoPixel RGB Strand with full color control
}

void HomeKit::loop() {
  homeSpan.poll();
}

void HomeKit::sync_state() {

}