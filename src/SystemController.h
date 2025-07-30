// SystemController.h
#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include <string>
#include "Modules/Module.h"
#include "Modules/Software/CommandParser/CommandParser.h"

class SystemController {
public:
    SystemController();

    // No longer void*, we pass std::string directly to modules that expect it.
    void begin();
    void loop();
    void enable();
    void disable();
    void reset();
    const char* status() const;

    void enable_module(const char* module_name);
    void disable_module(const char* module_name);
    void reset_module(const char* module_name);
    const char* module_status(const char* module_name) const;

private:
    Module*              modules[1];
    CommandParser        cmdParser;
    bool                 enabled = true;
};

#endif // SYSTEM_CONTROLLER_H
