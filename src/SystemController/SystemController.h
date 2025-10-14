#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <utility>

#include "../StringUtils.h"

#include "../Modules/Module/Module.h"
#include "../Modules/Software/System/System.h"
#include "../Modules/Software/SerialPort/SerialPort.h"
#include "../Modules/Software/CommandParser/CommandParser.h"
//#include "../Modules/Software/Wifi/Wifi.h"

//#include "../Interfaces/Interface/Interface.h"
//#include "../Interfaces/Hardware/LedStrip/LedStrip.h"
//#include "../Interfaces/Hardware/Nvs/Nvs.h"
//#include "../Interfaces/Software/Web/Web.h"
//#include "../Interfaces/Software/Homekit/Homekit.h"
//#include "../Interfaces/Software/Alexa/Alexa.h"

constexpr std::size_t MODULE_COUNT    = 9;
constexpr std::size_t INTERFACE_COUNT = 5;


class SystemController {
public:
    SystemController();

    void                        begin();
    void                        loop();

    void                        sync_color                  (const std::array<uint8_t,3> color,
                                                             const std::array<uint8_t,INTERFACE_COUNT>& sync_flags);
    void                        sync_brightness             (const uint8_t brightness,
                                                             const std::array<uint8_t,INTERFACE_COUNT>& sync_flags);
    void                        sync_state                  (const uint8_t state,
                                                             const std::array<uint8_t,INTERFACE_COUNT>& sync_flags);
    void                        sync_mode                   (const uint8_t mode,
                                                             const std::array<uint8_t,INTERFACE_COUNT>& sync_flags);
    void                        sync_length                 (const uint16_t length,
                                                             const std::array<uint8_t,INTERFACE_COUNT>& sync_flags);
    void                        sync_all                    (const std::array<uint8_t,3> color,
                                                             const uint8_t brightness,
                                                             const uint8_t state,
                                                             const uint8_t mode,
                                                             const uint16_t length,
                                                             const std::array<uint8_t,INTERFACE_COUNT>& sync_flags);

    SerialPort                  serial_port;
    Nvs                         nvs;
    System                      system;
    CommandParser               command_parser;

private:
    template <typename Fn>
    void                        for_each_interface          (const std::array<uint8_t,INTERFACE_COUNT>& sync_flags,
                                                             Fn&& fn);
//    LedStrip                    led_strip;
//    Wifi                        wifi;
//    Web                         web;
//    Homekit                     homekit;
//    Alexa                       alexa;

    Module*                     modules                     [MODULE_COUNT] = {};
//    Interface*                  interfaces                  [INTERFACE_COUNT] = {};

    std::vector<CommandsGroup>  command_groups;
};

//template <typename Fn>
//void SystemController::for_each_interface(
//    const std::array<uint8_t, INTERFACE_COUNT>& flags, Fn&& fn) {
//  for (std::size_t i = 0; i < INTERFACE_COUNT; ++i) {
//    if (flags[i] && interfaces[i]) {
//      std::forward<Fn>(fn)(*interfaces[i]);
//    }
//  }
//}

#endif // SYSTEM_CONTROLLER_H
