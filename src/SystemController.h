// src/SystemController.h
#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include <string>
#include <cstring>
#include "Modules/Module.h"

#include "Modules/Software/SerialPort/SerialPort.h"
#include "Modules/Software/CommandParser/CommandParser.h"
#include "Modules/Software/Wifi/Wifi.h"

class SystemController {
public:
    SystemController();

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

    void module_print_help(const char* module_name);
private:
    static constexpr size_t MODULE_COUNT = 3;
    Module* modules[MODULE_COUNT];

    SerialPort serial_port;
    CommandParser command_parser;
    Wifi wifi;
    bool enabled = true;
};

#endif // SYSTEM_CONTROLLER_H
