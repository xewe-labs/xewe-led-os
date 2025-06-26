#pragma once

#include <cstdint>
#include <array>

class SystemController;

class ControllerInterface {
public:
    ControllerInterface(SystemController& controller_ref) : public ControllerModule
    : controller(controller_ref) {}

    virtual ~ControllerInterface()     {}

    virtual bool begin              (void* context = nullptr, const String& device_name = "")   = 0;
    virtual bool loop               ()                                                          = 0;
    virtual bool reset              ()                                                          = 0;
    virtual bool status             ()                                                          = 0;
    virtual bool enable             ()                                                          = 0;
    virtual bool disable            ()                                                          = 0;

protected:
    SystemController&               controller;

private:
    bool                            enabled=false;
    virtual CommandParser::Command               commands [];
    virtual String                  mem_key;
};
