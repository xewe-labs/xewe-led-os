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

#include "../Resources/Nvs/Nvs.h"

#include "../Hardware/LedStrip/LedStrip.h"
#include "../Hardware/Buttons/Buttons.h"

#include "../Software/WebInterface/WebInterface.h"
#include "../Software/Alexa/Alexa.h"
#include "../Software/HomeKit/HomeKit.h"

class SystemController {
public:
    SystemController();

    /**
      * module flow :
      * begin (private)
      * loop  (private)
      * reset
      * status
      * enable
      * disable
    **/

    bool                            begin                           ();
    void                            loop                            ();

    // System commands
    void                            system_reset                    ();
    void                            system_status                   ();
    void                            system_restart                  (uint16_t delay_before=0);
    void                            system_sync_state               (String field, std::array<bool, 4> sync_flags);
    void                            system_module_enable            (bool& active_flag,
                                                                     const char* module_name_full,
                                                                     const char* memory_key,
                                                                     const char* prompt_details,
                                                                     bool force_enable,
                                                                     const bool& dependency_flag,
                                                                     const char* dependency_error_msg,
                                                                     bool requires_restart,
                                                                     std::function<void()> on_enable_action = nullptr,
                                                                     const char* already_enabled_msg = nullptr);
    void                            system_module_disable           (bool& active_flag,
                                                                     const char* module_name_full,
                                                                     const char* memory_key,
                                                                     bool force_disable,
                                                                     bool requires_restart,
                                                                     std::function<void()> on_disable_action = nullptr);

    // NEW: Callback for the button controller to execute commands
    void                            execute_command                 (const String& command);

    // Wi-Fi
    bool                            wifi_reset                      (bool print_info);
    void                            wifi_status                     ();
    void                            wifi_enable                     (bool force_enable, bool force_restart);
    void                            wifi_disable                    (bool force_disable, bool force_restart);
    std::vector<String>             wifi_get_available_networks     ();
    bool                            wifi_connect                    (bool prompt_for_credentials);
    bool                            wifi_read_stored_credentials    (String& ssid, String& pwd);
    uint8_t                         wifi_prompt_for_credentials     (String& ssid, String& pwd);
    bool                            wifi_join                       (const String& ssid, const String& password);
    bool                            wifi_disconnect                 ();

    // LED strip
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
    void                            led_strip_toggle_state          ();
    void                            led_strip_toggle_state          (std::array<bool, 4> sync_flags);
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
    void                            ram_status                      ();
    void                            ram_free                        ();
    void                            ram_watch                       (const String& args);

    // Web Interface commands
    void                            webinterface_reset              ();
    void                            webinterface_status             ();
    void                            webinterface_enable             (bool force_enable, bool force_restart);
    void                            webinterface_disable            (bool force_disable, bool force_restart);

    // Alexa commands
    void                            alexa_reset                     ();
    void                            alexa_status                    ();
    void                            alexa_enable                    (bool force_enable, bool force_restart);
    void                            alexa_disable                   (bool force_disable, bool force_restart);

    // HomeKit commands
    void                            homekit_reset                   ();
    void                            homekit_status                  ();
    void                            homekit_enable                  (bool force_enable, bool force_restart);
    void                            homekit_disable                 (bool force_disable, bool force_restart);

    // NEW: Button commands
    void                            buttons_reset                    ();
    void                            buttons_status                   ();
    void                            buttons_enable                   (bool force_enable, bool force_restart);
    void                            buttons_disable                  (bool force_disable, bool force_restart);
    void                            buttons_add                      (const String& args);
    void                            buttons_remove                   (const String& args);


private:
    // begin methods
    bool                            serial_port_begin               ();
    bool                            nvs_begin                       ();
    bool                            system_begin                    (bool first_init_flag=false);
    bool                            led_strip_begin                 (bool first_init_flag=false);
    bool                            buttons_begin                   (bool first_init_flag=false);
    bool                            wifi_begin                      (bool first_init_flag=false);
    bool                            web_server_begin                (bool first_init_flag=false);
    bool                            webinterface_begin              (bool first_init_flag=false);
    bool                            alexa_begin                     (bool first_init_flag=false);
    bool                            homekit_begin                   (bool first_init_flag=false);
    bool                            command_parser_begin            (bool first_init_flag=false);

    // Helper to get argument values from command strings
    String                          get_arg_value                   (const String& args, const String& key);


    // Member Objects
    SerialPort                      serial_port;
    Nvs                             nvs;
    Wifi                            wifi;
    LedStrip                        led_strip;
    WebServer                       web_server                      {80};
    WebInterface                    webinterface;
    Alexa                           alexa;
    HomeKit                         homekit;
    Buttons                         buttons;
    CommandParser                   command_parser;

    // Module Active Flags
    bool                            wifi_module_active              = false;
    bool                            webinterface_module_active      = false;
    bool                            alexa_module_active             = false;
    bool                            homekit_module_active           = false;
    bool                            buttons_module_active           = false;

    // NEW: Updated command counts
    static const size_t             COMMAND_PARSER_CMD_COUNT        = 1;
    static const size_t             SYSTEM_CMD_COUNT                = 4;
    static const size_t             LED_STRIP_CMD_COUNT             = 18;
    static const size_t             WIFI_CMD_COUNT                  = 8;
    static const size_t             WEBINTERFACE_CMD_COUNT          = 5;
    static const size_t             ALEXA_CMD_COUNT                 = 5;
    static const size_t             HOMEKIT_CMD_COUNT               = 5;
    static const size_t             RAM_CMD_COUNT                   = 4;
    static const size_t             BUTTONS_CMD_COUNT               = 7;
    static const size_t             CMD_GROUP_COUNT                 = 9;

    // Command Definition Arrays
    CommandParser::Command          command_parser_commands         [COMMAND_PARSER_CMD_COUNT];
    CommandParser::Command          system_commands                 [SYSTEM_CMD_COUNT];
    CommandParser::Command          led_strip_commands              [LED_STRIP_CMD_COUNT];
    CommandParser::Command          wifi_commands                   [WIFI_CMD_COUNT];
    CommandParser::Command          webinterface_commands           [WEBINTERFACE_CMD_COUNT];
    CommandParser::Command          alexa_commands                  [ALEXA_CMD_COUNT];
    CommandParser::Command          homekit_commands                [HOMEKIT_CMD_COUNT];
    CommandParser::Command          ram_commands                    [RAM_CMD_COUNT];
    CommandParser::Command          buttons_commands                [BUTTONS_CMD_COUNT];
    CommandParser::CommandGroup     command_groups                  [CMD_GROUP_COUNT];
};

template <typename T>
bool in_range(T val, T low, T high) {
    return (val >= low && val <= high);
}

#endif // SYSTEMCONTROLLER_H
