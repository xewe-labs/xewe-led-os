// Software/Homekit/Homekit.cpp
#include "Homekit.h"
#include "../../../SystemController/SystemController.h"

#if !defined(ESP32)
  #error "This Homekit interface builds only for ESP32."
#endif

#include <Arduino.h>
#include <cmath>
#include "HomeSpan.h"

// ---------------- HSV/RGB helpers ----------------
static inline float clampf(float v, float lo, float hi){ return v<lo?lo:(v>hi?hi:v); }

static std::array<uint8_t,3> hsv_to_rgb(float h_deg, float s_pct, float v_pct){
    float H = std::fmodf(h_deg, 360.0f); if (H<0) H+=360.0f;
    float S = clampf(s_pct/100.0f, 0.0f, 1.0f);
    float V = clampf(v_pct/100.0f, 0.0f, 1.0f);
    if (S==0.0f){
        uint8_t g = (uint8_t)std::round(V*255.0f);
        return {g,g,g};
    }
    float C = V*S;
    float X = C*(1.0f - std::fabsf(std::fmodf(H/60.0f, 2.0f)-1.0f));
    float m = V-C;
    float r=0,g=0,b=0;
    if      (H< 60){ r=C; g=X; b=0; }
    else if (H<120){ r=X; g=C; b=0; }
    else if (H<180){ r=0; g=C; b=X; }
    else if (H<240){ r=0; g=X; b=C; }
    else if (H<300){ r=X; g=0; b=C; }
    else           { r=C; g=0; b=X; }
    uint8_t R=(uint8_t)std::round((r+m)*255.0f);
    uint8_t G=(uint8_t)std::round((g+m)*255.0f);
    uint8_t B=(uint8_t)std::round((b+m)*255.0f);
    return {R,G,B};
}

void Homekit::rgb_to_hs(std::array<uint8_t,3> rgb, float& h_deg, float& s_pct){
    float r = rgb[0]/255.0f, g = rgb[1]/255.0f, b = rgb[2]/255.0f;
    float mx = std::fmaxf(r, std::fmaxf(g,b));
    float mn = std::fminf(r, std::fminf(g,b));
    float d  = mx - mn;

    // Hue
    float h=0.0f;
    if (d==0.0f) h = 0.0f;
    else if (mx==r) h = 60.0f*std::fmodf(((g-b)/d), 6.0f);
    else if (mx==g) h = 60.0f*((b-r)/d + 2.0f);
    else            h = 60.0f*((r-g)/d + 4.0f);
    if (h<0.0f) h+=360.0f;

    // Saturation (relative to V)
    float s = (mx==0.0f) ? 0.0f : (d/mx);

    h_deg = h;
    s_pct = s*100.0f;
}

// ---------------- HomeSpan Service ----------------
struct Homekit::NeoPixel_RGB : Service::LightBulb {
    Characteristic::On         power       {0,   true};
    Characteristic::Hue        H           {0,   true};     // degrees 0..360
    Characteristic::Saturation S           {0,   true};     // percent 0..100
    Characteristic::Brightness V           {100, true};     // percent 0..100

    SystemController* controller;

    explicit NeoPixel_RGB(SystemController* ctrl) : Service::LightBulb(), controller(ctrl) {
        V.setRange(1, 100, 1);
    }

    boolean update() override {
        if (!controller) return false;

        const bool pwr = power.getNewVal();
        const float h  = H.getNewVal<float>();        // 0..360
        const float s  = S.getNewVal<float>();        // 0..100
        const float v  = V.getNewVal<float>();        // 0..100

        // Map to controller: brightness separate, color uses full V for hue/sat fidelity
        const uint8_t bri255 = (uint8_t)std::round(clampf(v,0,100)*255.0f/100.0f);
        const auto rgb = hsv_to_rgb(h, s, 100.0f);    // color with full V; brightness handled separately

        controller->sync_state(pwr ? 1 : 0,           {true,true,true,false,true});
        controller->sync_brightness(bri255,           {true,true,true,false,true});
        controller->sync_color(rgb,                   {true,true,true,false,true});

        return true;
    }
};

// ---------------- Homekit class ----------------
Homekit::Homekit(SystemController& controller_ref)
: Interface(controller_ref, "homekit", "homekit", false, false)
{
    DBG_PRINTLN(Homekit, "Homekit::Homekit()");
}

void Homekit::begin(const ModuleConfig& cfg){
    (void)cfg;
    DBG_PRINTLN(Homekit, "Homekit::begin()");

    homeSpan.setPortNum(1201);                 // avoid port 80
    homeSpan.setSerialInputDisable(true);
    homeSpan.setLogLevel(-1);

    std::string dev = controller.get_name();
    if (dev.empty()) dev = "LED Strip";

    homeSpan.begin(Category::Lighting, dev.c_str());
    SPAN_ACCESSORY(dev.c_str());
    device_ = new NeoPixel_RGB(&controller);

    DBG_PRINTLN(Homekit, "Homekit initialized.");
}

void Homekit::loop(){
    homeSpan.poll();
}

void Homekit::reset(bool verbose){
    (void)verbose;
    DBG_PRINTLN(Homekit, "Homekit::reset() -> factory reset HomeSpan pairing");
    homeSpan.setLogLevel(0);
    homeSpan.processSerialCommand("F");
    homeSpan.setLogLevel(-1);
}

// ---------------- System -> HomeKit sync ----------------
void Homekit::sync_color(std::array<uint8_t,3> color){
    if (!device_) return;
    float h=0.0f, s=0.0f;
    rgb_to_hs(color, h, s);
    device_->H.setVal(std::round(h));
    device_->S.setVal(std::round(s));
    DBG_PRINTF(Homekit, "sync_color: RGB={%u,%u,%u} -> H=%.0f S=%.0f\n",
               color[0], color[1], color[2], h, s);
}

void Homekit::sync_brightness(uint8_t brightness){
    if (!device_) return;
    float v = std::round((brightness/255.0f)*100.0f);
    device_->V.setVal(v);
    DBG_PRINTF(Homekit, "sync_brightness: %u -> V=%.0f%%\n", brightness, v);
}

void Homekit::sync_state(uint8_t state){
    if (!device_) return;
    bool p = (state != 0);
    device_->power.setVal(p);
    DBG_PRINTF(Homekit, "sync_state: %u -> %s\n", state, p ? "ON" : "OFF");
}

void Homekit::sync_mode(uint8_t /*mode*/){
    // Not exposed in HomeKit
}

void Homekit::sync_length(uint16_t /*length*/){
    // Not exposed in HomeKit
}

void Homekit::sync_all(std::array<uint8_t,3> color,
                       uint8_t brightness,
                       uint8_t state,
                       uint8_t mode,
                       uint16_t length){
    DBG_PRINTF(Homekit, "sync_all(rgb={%u,%u,%u}, bri=%u, state=%u, mode=%u, len=%u)\n",
               color[0], color[1], color[2], brightness, state, mode, length);
    sync_state(state);
    sync_brightness(brightness);
    sync_color(color);
    (void)mode; (void)length;
}
