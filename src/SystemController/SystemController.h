#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include <string>
#include <string_view>
#include <vector>
#include <Arduino.h>
#include <string>

#include "../Modules/Module/Module.h"
#include "../Modules/Software/System/System.h"
#include "../Modules/Software/SerialPort/SerialPort.h"
#include "../Modules/Software/CommandParser/CommandParser.h"
#include "../Modules/Software/Wifi/Wifi.h"
#define MODULE_COUNT 5 // modules (4) + interfaces


#include "../Interfaces/Interface/Interface.h"
#include "../Interfaces/Hardware/LedStrip/LedStrip.h"
#include "../Interfaces/Hardware/Nvs/Nvs.h"
#include "../Interfaces/Software/Web/Web.h"
#include "../Interfaces/Software/Homekit/Homekit.h"
#include "../Interfaces/Software/Alexa/Alexa.h"
#define INTERFACE_COUNT 5 // stay at 5 for array sizes


class SystemController {
public:
    SystemController();

    void                        begin();
    void                        loop();
    void                        reset();
    std::string                 status() const;

    void                        module_enable               (std::string_view module_name);
    void                        module_disable              (std::string_view module_name);
    void                        module_reset                (std::string_view module_name);
    std::string                 module_status               (std::string_view module_name) const;
    void                        module_print_help           (std::string_view module_name);

    void                        sync_color                  (std::array<uint8_t,3> color,
                                                             std::array<uint8_t,INTERFACE_COUNT> sync_flags);
    void                        sync_brightness             (uint8_t brightness,
                                                             std::array<uint8_t,INTERFACE_COUNT> sync_flags);
    void                        sync_state                  (uint8_t state,
                                                             std::array<uint8_t,INTERFACE_COUNT> sync_flags);
    void                        sync_mode                   (uint8_t mode,
                                                             std::array<uint8_t,INTERFACE_COUNT> sync_flags);
    void                        sync_length                 (uint16_t length,
                                                             std::array<uint8_t,INTERFACE_COUNT> sync_flags);
    void                        sync_all                    (std::array<uint8_t,3> color,
                                                             uint8_t brightness,
                                                             uint8_t state,
                                                             uint8_t mode,
                                                             uint16_t length,
                                                             std::array<uint8_t,INTERFACE_COUNT> sync_flags);

    std::string                 get_name();

    Nvs                         nvs;
    SerialPort                  serial_port;
private:
    System                      system;
    CommandParser               command_parser;
    LedStrip                    led_strip;
//    Wifi                        wifi;
//    Web                         web;
//    Homekit                     homekit;
//    Alexa                       alexa;

//    static constexpr size_t     MODULE_COUNT                = 6;
//    static constexpr size_t     INTERFACE_COUNT             = 5;

    Module*                     modules                     [MODULE_COUNT];
    Interface*                  interfaces                  [INTERFACE_COUNT];

    std::vector<CommandsGroup>  command_groups;
};

#endif // SYSTEM_CONTROLLER_H
