#ifndef HOMEKIT_H
#define HOMEKIT_H

#include "HomeSpan.h"
#include "../../Debug.h"

class SystemController;

class HomeKit {
public:
    HomeKit                         (SystemController& controller_ref);
    void                begin       ();
    void                loop        ();
    void                sync_state  ();
    bool                is_paired   ();

private:
    SystemController&   controller;
};

#endif // HOMEKIT_H
