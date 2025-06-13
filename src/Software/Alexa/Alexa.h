#ifndef ALEXA_H
#define ALEXA_H

#include <Espalexa.h>
#include <WebServer.h>
#include "../../Debug.h"

class SystemController;

class Alexa {
public:
    // Constructor is now empty
    Alexa                               ();

    // begin() now takes both dependencies
    void                begin           (SystemController& controller_ref, WebServer* server_instance);

    void                loop            ();
    void                sync_state_with_system_controller(const char* field);
    Espalexa&           get_instance    () { return espalexa; }

private:
    // Pointers to dependencies
    SystemController* controller;
    Espalexa            espalexa;
    EspalexaDevice* smart_light_device_ = nullptr;

    void                handle_smart_light_change(EspalexaDevice* device_ptr);
};

#endif // ALEXA_H