// SystemController.h
#ifndef SYSTEMCONTROLLER_H
#define SYSTEMCONTROLLER_H

#include <Arduino.h>
#include "../../lib/Adafruit_NeoPixel/Adafruit_NeoPixel.h"
#include <vector>
#include "../Interfaces/SerialPort/SerialPort.h"
#include "../Interfaces/Wifi/Wifi.h"
#include "../Resources/Memory/Memory.h"
#include "../LedController/LedController.h"
#include "CommandParser/CommandParser.h"

class SystemController {
public:
    SystemController(Adafruit_NeoPixel* strip);
    void init_system_setup();
    void update();

private:
    void define_commands();

    // Wi-Fi flows
    void wifi_print_help();
    void wifi_print_credentials();
    std::vector<String> wifi_get_available_networks();
    bool wifi_connect();
    bool read_memory_wifi_credentials(String& ssid, String& pwd);
    bool prompt_user_for_wifi_credentials(String& ssid, String& pwd);
    bool disconnect_wifi();
    bool reset_wifi_credentials();

    // LED
    void led_strip_print_help();
    void led_strip_set_mode(const String& args);
    void led_strip_set_rgb(const String& args);
    void led_strip_set_r(const String& args);
    void led_strip_set_g(const String& args);
    void led_strip_set_b(const String& args);
    void led_strip_set_hsv(const String& args);
    void led_strip_set_hue(const String& args);
    void led_strip_set_sat(const String& args);
    void led_strip_set_val(const String& args);
    void led_strip_set_brightness(const String& args);
    void led_strip_set_state(const String& args);
    void led_strip_turn_on();
    void led_strip_turn_off();


    SerialPort                     serial_port;
    Wifi                           wifi;
    Memory                         memory;
    CommandParser                  command_parser;
    LedController                  led_controller;

//    command storage definitions
    static const size_t            WIFI_CMD_COUNT       = 6;
    static const size_t            LED_STRIP_CMD_COUNT  = 14;
    static const size_t            CMD_GROUP_COUNT      = 2;

    CommandParser::Command         wifi_commands[WIFI_CMD_COUNT];
    CommandParser::Command         led_strip_commands[LED_STRIP_CMD_COUNT];
    CommandParser::CommandGroup    command_groups[CMD_GROUP_COUNT];

};

#endif // SYSTEMCONTROLLER_H
