// Alexa.h
#ifndef ALEXA_H
#define ALEXA_H

#include <Espalexa.h>
#include <WebServer.h>
#include "../../Debug.h"

class SystemController;

class Alexa {
public:
    Alexa(SystemController& controller_ref);
    void begin(WebServer& server_instance);
    void loop();
    void sync_state_with_system_controller(const char* field);

    Espalexa& getEspalexaCoreInstance() { return espalexa; }
private:
    SystemController& controller;
    Espalexa espalexa; // This is the instance you want to return
    EspalexaDevice* smart_light_device_ = nullptr;
    void handle_smart_light_change(EspalexaDevice* device_ptr);
};

#endif // ALEXA_H