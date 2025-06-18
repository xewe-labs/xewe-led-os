#ifndef SYSTEMCONTROLLER_H
#define SYSTEMCONTROLLER_H

#include <Arduino.h>
#include <WebServer.h> // Use synchronous server
#include <Ticker.h>            // for periodic heartbeat
#include <vector>

#include "CommandParser/CommandParser.h"

#include "../Config.h"
#include "../Debug.h"

#include "../Interfaces/SerialPort/SerialPort.h"
#include "../Interfaces/Wifi/Wifi.h"

#include "../Resources/Memory/Memory.h"

#include "../Hardware/LedStrip/LedStrip.h"

#include "../Software/WebInterface/WebInterface.h"
#include "../Software/Alexa/Alexa.h"
#include "../Software/HomeKit/HomeKit.h"

class SystemController {
public:
    SystemController();

    bool                            begin                           ();
    void                            loop                            ();

    // System commands
    void                            system_print_help               ();
    void                            system_reset                    ();
    void                            system_restart                  (uint16_t delay_before=0);
    void                            system_sync_state               (String field, std::array<bool, 4> sync_flags);

    // Wi-Fi
    void                            wifi_print_help                 ();
//    void                            wifi_enable                     ();
//    void                            wifi_disable                    ();
    bool                            wifi_reset                      (bool print_info);
    void                            wifi_status                     ();

    std::vector<String>             wifi_get_available_networks     ();
    bool                            wifi_connect                    (bool prompt_for_credentials);
    bool                            wifi_read_stored_credentials    (String& ssid, String& pwd);
    uint8_t                         wifi_prompt_for_credentials     (String& ssid, String& pwd);
    bool                            wifi_join                       (const String& ssid, const String& password);
    bool                            wifi_disconnect                 ();

    // LED strip
    void                            led_strip_print_help            ();
//    void                            led_strip_enable                ();
//    void                            led_strip_disable               ();
    void                            led_strip_reset                 (uint16_t led_num=10);
    void                            led_strip_status                ();

    void                            led_strip_set_mode              (const String& args);
    void                            led_strip_set_mode              (uint8_t new_mode, std::array<bool, 4> sync_flags);
    void                            led_strip_set_rgb               (const String& args);
    void                            led_strip_set_rgb               (std::array<uint8_t, 3> new_rgb, std::array<bool, 4> sync_flags);
    void                            led_strip_set_r                 (const String& args);
    void                            led_strip_set_r                 (uint8_t new_r, std::array<bool, 4> sync_flags);
    void                            led_strip_set_g                 (const String& args);
    void                            led_strip_set_g                 (uint8_t new_g, std::array<bool, 4> sync_flags);
    void                            led_strip_set_b                 (const String& args);
    void                            led_strip_set_b                 (uint8_t new_b, std::array<bool, 4> sync_flags);
    void                            led_strip_set_hsv               (const String& args);
    void                            led_strip_set_hsv               (std::array<uint8_t, 3> new_hsv, std::array<bool, 4> sync_flags);
    void                            led_strip_set_hue               (const String& args);
    void                            led_strip_set_hue               (uint8_t new_hue, std::array<bool, 4> sync_flags);
    void                            led_strip_set_sat               (const String& args);
    void                            led_strip_set_sat               (uint8_t new_sat, std::array<bool, 4> sync_flags);
    void                            led_strip_set_val               (const String& args);
    void                            led_strip_set_val               (uint8_t new_val, std::array<bool, 4> sync_flags);
    void                            led_strip_set_brightness        (const String& args);
    void                            led_strip_set_brightness        (uint8_t new_brightness, std::array<bool, 4> sync_flags);
    void                            led_strip_set_state             (const String& args);
    void                            led_strip_set_state             (bool new_state, std::array<bool, 4> sync_flags);
    void                            led_strip_turn_on               ();
    void                            led_strip_turn_on               (std::array<bool, 4> sync_flags);
    void                            led_strip_turn_off              ();
    void                            led_strip_turn_off              (std::array<bool, 4> sync_flags);
    void                            led_strip_set_length            (const String& args);
    void                            led_strip_set_length            (uint16_t new_length, std::array<bool, 4> sync_flags);

    std::array<uint8_t, 3>          led_strip_get_target_rgb        ()                      const;
    std::array<uint8_t, 3>          led_strip_get_target_hsv        ()                      const;
    uint8_t                         led_strip_get_target_brightness ()                      const;
    bool                            led_strip_get_target_state      ()                      const;
    uint8_t                         led_strip_get_target_mode_id    ()                      const;
    String                          led_strip_get_target_mode_name  ()                      const;


    // RAM commands
    void                            ram_print_help                  ();
    void                            ram_status                      ();
    void                            ram_free                        ();
    void                            ram_watch                       (const String& args);

//    void                            webinterface_strip_print_help   ();
//    void                            webinterface_strip_enable       ();
//    void                            webinterface_strip_disable      ();
//    void                            webinterface_strip_reset        ();
//    void                            webinterface_strip_status       ();
//
//    void                            alexa_print_help                ();
//    void                            alexa_enable                    ();
//    void                            alexa_disable                   ();
//    void                            alexa_reset                     ();
//    void                            alexa_status                    ();
//
//    void                            homekit_print_help              ();
//    void                            homekit_enable                  ();
//    void                            homekit_disable                 ();
//    void                            homekit_reset                   ();
//    void                            homekit_status                  ();
//
    void                            command_parser_print_help       ();

private:
    // begin methods
    bool                            serial_port_begin               ();
    bool                            memory_begin                    ();
    bool                            system_begin                    (bool first_init_flag=false);
    bool                            wifi_begin                      (bool first_init_flag=false);
    bool                            led_strip_begin                 (bool first_init_flag=false);
    bool                            web_server_begin                (bool first_init_flag=false);
    bool                            web_interface_begin             (bool first_init_flag=false);
    bool                            alexa_begin                     (bool first_init_flag=false);
    bool                            homekit_begin                   (bool first_init_flag=false);
    bool                            command_parser_begin            (bool first_init_flag=false);


    SerialPort                      serial_port;
    Memory                          memory;
    Wifi                            wifi;
    LedStrip                        led_strip;
    WebServer                       web_server                      {80};
    WebInterface                    web_interface;
    Alexa                           alexa;
    HomeKit                         homekit;
    CommandParser                   command_parser;

    bool                            wifi_module_active              = false;
    bool                            webinterface_module_active      = false;
    bool                            alexa_module_active             = false;
    bool                            homekit_module_active           = false;

    static const size_t             HELP_CMD_COUNT                  = 1;
    static const size_t             SYSTEM_CMD_COUNT                = 3;
    static const size_t             WIFI_CMD_COUNT                  = 6;
    static const size_t             LED_STRIP_CMD_COUNT             = 16;
    static const size_t             RAM_CMD_COUNT                   = 4;
    static const size_t             CMD_GROUP_COUNT                 = 5;

    CommandParser::Command          help_commands                   [HELP_CMD_COUNT];
    CommandParser::Command          system_commands                 [SYSTEM_CMD_COUNT];
    CommandParser::Command          wifi_commands                   [WIFI_CMD_COUNT];
    CommandParser::Command          led_strip_commands              [LED_STRIP_CMD_COUNT];
    CommandParser::Command          ram_commands                    [RAM_CMD_COUNT];

    CommandParser::CommandGroup     command_groups                  [CMD_GROUP_COUNT];
};

template <typename T>
bool in_range(T val, T low, T high) {
    return (val >= low && val <= high);
}

#endif // SYSTEMCONTROLLER_H
