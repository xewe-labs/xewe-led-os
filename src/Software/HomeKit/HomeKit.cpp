#include "HomeKit.h"
#include "../../SystemController/SystemController.h"

struct NeoPixel_RGB : Service::LightBulb {      // Addressable single-wire RGB LED Strand (e.g. NeoPixel)

  Characteristic::On power{0,true};
  Characteristic::Hue H{0,true};
  Characteristic::Saturation S{0,true};
  Characteristic::Brightness V{100,true};
  Pixel *pixel;
  int nPixels;

  NeoPixel_RGB(uint8_t pin, int nPixels) : Service::LightBulb(){

    V.setRange(5,100,1);                      // sets the range of the Brightness to be from a min of 5%, to a max of 100%, in steps of 1%
    pixel=new Pixel(pin);                     // creates Pixel RGB LED on specified pin
    nPixels=37;
    this->nPixels=nPixels;                    // save number of Pixels in this LED Strand
    update();                                 // manually call update() to set pixel with restored initial values
  }

  boolean update() override {

    int p=power.getNewVal();

    float h=H.getNewVal<float>();       // range = [0,360]
    float s=S.getNewVal<float>();       // range = [0,100]
    float v=V.getNewVal<float>();       // range = [0,100]

    Pixel::Color color;

    pixel->set(color.HSV(h*p, s*p, v*p),nPixels);       // sets all nPixels to the same HSV color

    return(true);
  }
};

HomeKit::HomeKit(SystemController& controller_ref) : controller(controller_ref) {}

void HomeKit::begin() {
    homeSpan.setLogLevel(1);

    homeSpan.setHostNameSuffix("");         // use null string for suffix (rather than the HomeSpan device ID)
    homeSpan.setPortNum(1201);              // change port number for HomeSpan so we can use port 80 for the Web Server
    homeSpan.enableOTA();                   // enable OTA updates

    homeSpan.begin(Category::Lighting,"Xewe Lights");
    SPAN_ACCESSORY();
// create Bridge (note this sketch uses the SPAN_ACCESSORY() macro, introduced in v1.5.1 --- see the HomeSpan API Reference for details on this convenience macro)
//    homeSpan.begin(Category::Bridges,"XeWe Lights Hub","hub");
//
//    new SpanAccessory(1);
//    new DEV_Identify("XeWe Lights","XeWe Labs","000-000-001","Addressable LED","1.0",3);
//    new Service::HAPProtocolInformation();
//        new Characteristic::Version("1.0.0");

    SPAN_ACCESSORY("RGB Lights");
        new NeoPixel_RGB(2,8);                       // create 8-LED NeoPixel RGB Strand with full color control
}

void HomeKit::loop() {
  homeSpan.poll();
}

void HomeKit::sync_state() {

}