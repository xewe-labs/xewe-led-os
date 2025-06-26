#pragma once

#include <cstdint>
#include <array>

class SystemController;
class String;

namespace CommandParser { struct Command; }

class ControllerModule {
public:
    ControllerModule(SystemController& controller_ref)
    : controller_ref(controller_ref) {}

    virtual ~ControllerModule()     {}

    virtual bool begin              (void* context = nullptr, const String& device_name = "")   = 0;
    virtual bool loop               ()                                                          = 0;
    virtual bool reset              ()                                                          = 0;
    virtual bool status             ()                                                          = 0;
    virtual bool enable             ()                                                          = 0;
    virtual bool disable            ()                                                          = 0;

protected:
    SystemController&               controller_ref;

    virtual const SystemController::Command* get_commands() const = 0;
    virtual size_t get_commands_num() const = 0;
    virtual const String& get_mem_key() const = 0;

private:
    bool                            enabled=false;
};