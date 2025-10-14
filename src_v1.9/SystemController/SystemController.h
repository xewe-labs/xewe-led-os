#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include <string>
#include <string_view>
#include <vector>
#include <Arduino.h>
#include <string>
#include <WebServer.h>  // <-- needed for WebServer*

#include "../StringUtils.h"
using namespace xewe::str;

#include "../Modules/Module/Module.h"
#include "../Modules/Software/System/System.h"
#include "../Modules/Software/SerialPort/SerialPort.h"
#include "../Modules/Software/CommandParser/CommandParser.h"
#include "../Modules/Software/Wifi/Wifi.h"

#include "../Interfaces/Interface/Interface.h"
#include "../Interfaces/Hardware/LedStrip/LedStrip.h"
#include "../Interfaces/Hardware/Nvs/Nvs.h"
#include "../Interfaces/Software/Web/Web.h"
#include "../Interfaces/Software/Homekit/Homekit.h"
#include "../Interfaces/Software/Alexa/Alexa.h"

#define MODULE_COUNT 9 // modules (4) + interfaces
#define INTERFACE_COUNT 5


class SystemController {
public:
    SystemController();

    bool                        begin();
    void                        loop();

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

    Nvs                         nvs;
    SerialPort                  serial_port;
    LedStrip                    led_strip;
    System                      system;
private:
    CommandParser               command_parser;
    Wifi                        wifi;
    Web                         web;
    Homekit                     homekit;
    Alexa                       alexa;

    Module*                     modules                     [MODULE_COUNT];
    Interface*                  interfaces                  [INTERFACE_COUNT];

    std::vector<CommandsGroup>  command_groups;
};

#endif // SYSTEM_CONTROLLER_H
