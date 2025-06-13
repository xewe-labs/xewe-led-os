#ifndef HOMEKIT_H
#define HOMEKIT_H

#include "HomeSpan.h"
#include "../../Debug.h"

class SystemController;

class HomeKit {
public:
    // Constructor is now empty
    HomeKit                         ();

    // begin() now takes the controller dependency
    void                begin       (SystemController& controller_ref);

    void                loop        ();
    void                sync_state  ();
    bool                is_paired   ();

private:
    // The controller is now a pointer instead of a reference
    SystemController* controller;
};

#endif // HOMEKIT_H