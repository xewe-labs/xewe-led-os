// Alexa.h
#ifndef ALEXA_H
#define ALEXA_H

#include <Espalexa.h>
#include <WebServer.h> // ESP32 core WebServer (for type hinting)
#include "../../Debug.h" // Assuming your DBG_PRINTLN is here

// Forward declaration
class SystemController; 

class Alexa {
public:
    Alexa(SystemController& controller_ref);

    // Call this in SystemController's setup, after WiFi and WebServer are ready
    void begin(WebServer& server_instance); // Takes the ESP32 core WebServer instance

    // Call this in SystemController's update() or main loop()
    void loop();

    // Call this from SystemController whenever an LED state changes from any source
    void sync_state_with_system_controller();

private:
    SystemController& controller_;
    Espalexa espalexa_;

    EspalexaDevice* smart_light_device_ = nullptr;

    void handle_smart_light_change(EspalexaDevice* device_ptr);
};

#endif // ALEXA_H