// src/SystemController.h

#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include <string>
#include <string_view>
#include <vector>
#include "Modules/Module.h"

#include "Modules/Software/SerialPort/SerialPort.h"
#include "Modules/Software/CommandParser/CommandParser.h"
#include "Modules/Software/Wifi/Wifi.h"
#include "Interfaces/Hardware/Nvs/Nvs.h"    // <— added Nvs

class SystemController {
public:
    SystemController();

    void begin();
    void loop();
    void enable();
    void disable();
    void reset();
    std::string_view status() const;

    void enable_module(std::string_view module_name);
    void disable_module(std::string_view module_name);
    void reset_module(std::string_view module_name);
    std::string_view module_status(std::string_view module_name) const;

    void module_print_help(std::string_view module_name);

private:
    static constexpr size_t MODULE_COUNT = 4;    // <— updated
    Module*              modules[MODULE_COUNT];

    Nvs                  nvs;                   // <— new
    SerialPort           serial_port;
    CommandParser        command_parser;
    Wifi                 wifi;
    bool                 enabled = true;

    // Holds only those modules that actually have CLI commands:
    std::vector<CommandsGroup> command_groups;
};

#endif // SYSTEM_CONTROLLER_H
