#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include <string>
#include <string_view>
#include <vector>
#include <Arduino.h>
#include <string>

#include "../Modules/Module/Module.h"
#include "../Modules/Software/SerialPort/SerialPort.h"
#include "../Modules/Software/CommandParser/CommandParser.h"
#include "../Modules/Software/Wifi/Wifi.h"

#include "../Interfaces/Hardware/Nvs/Nvs.h"
#include "../Interfaces/Hardware/LedStrip/LedStrip.h"

class SystemController {
public:
    SystemController();

    void                begin();
    void                loop();
    void                reset();
    std::string         status() const;

    void                module_enable       (std::string_view module_name);
    void                module_disable      (std::string_view module_name);
    void                module_reset        (std::string_view module_name);
    std::string         module_status       (std::string_view module_name) const;
    void                module_print_help   (std::string_view module_name);

    SerialPort           serial_port;
    Nvs                  nvs;
private:
    CommandParser        command_parser;
    LedStrip             led_strip;
    Wifi                 wifi;

    static constexpr size_t MODULE_COUNT = 5;
    Module*              modules[MODULE_COUNT];

    std::vector<CommandsGroup> command_groups;
};

#endif // SYSTEM_CONTROLLER_H
