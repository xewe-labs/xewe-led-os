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
    void reset();
    std::string_view status() const;

    void                module_enable       (std::string_view module_name);
    void                module_disable      (std::string_view module_name);
    void                module_reset        (std::string_view module_name);
    std::string_view    module_status       (std::string_view module_name) const;
    void                module_print_help   (std::string_view module_name);

    SerialPort           serial_port;
    Nvs                  nvs;
private:
    static constexpr size_t MODULE_COUNT = 4;    // <— updated
    Module*              modules[MODULE_COUNT];

    CommandParser        command_parser;
    Wifi                 wifi;

    std::vector<CommandsGroup> command_groups;
};

#endif // SYSTEM_CONTROLLER_H
