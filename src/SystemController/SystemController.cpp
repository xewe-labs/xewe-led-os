#include "SystemController.h"

static Ticker ram_print_ticker;

SystemController::SystemController() {}

// main functions
bool SystemController::begin() {
    DBG_PRINTF(SystemController, "SystemController::begin()\n");

    if (!serial_port_begin()) {
        system_restart(1000);
    }
    if (!memory_begin()) {
        serial_port.println("NVS Init Failed!");
        system_restart(1000);
    }

    bool first_init_flag = memory.read_uint8("first_init_done") == 0;

    if (!system_begin(first_init_flag)) {
        serial_port.println("System Init Failed!");
        system_restart(1000);
    }
    if (!led_strip_begin(first_init_flag)) {
        serial_port.println("Led Strip Init Failed!");
        system_restart(1000);
    }
    if (first_init_flag || wifi_module_active) {
        if (!wifi_begin(first_init_flag)) {
            serial_port.println("WiFi Init Failed!");
            system_restart(1000);
        }
    } else {
        serial_port.println("WiFi Module is inactive, skipping init");
    }
    if (wifi_module_active){
        if (!web_server_begin(first_init_flag)) {
            serial_port.println("Web Server Init Failed!");
            system_restart(1000);
        }
        if (!web_interface_begin(first_init_flag)) {
            serial_port.println("Web Interface Init Failed!");
            system_restart(1000);
        }
        if (!alexa_begin(first_init_flag)) {
            serial_port.println("Alexa Init Failed!");
            system_restart(1000);
        }
        if (!homekit_begin(first_init_flag)) {
            serial_port.println("HomeKit Init Failed!");
            system_restart(1000);
        }
    } else {
        serial_port.println("Skipping WebInterface, Alexa, HomeKit inits\nUse $wifi enable, then\n$webinterface enable\n$alexa enable\n$homekit enable\nIf you'd like to use them");
    }
    if (!command_parser_begin(first_init_flag)) {
        serial_port.println("CMD Parser Init Failed!");
        system_restart(1000);
    }

    if (first_init_flag) {
        memory.write_uint8("first_init_done", 1);
        memory.commit();
        serial_port.print("\n+------------------------------------------------+\n"
                          "|              Initial setup success!            |\n"
                          "+------------------------------------------------+\n");
        serial_port.get_string("Press enter to restart");
        system_restart(1000);
    }

    return true;
}

void SystemController::loop() {
    if (serial_port.has_line()) {
        String line = serial_port.read_line();
        command_parser.loop(line);
    }

    if (wifi_module_active) {
        if (wifi.is_connected()) {
            if (homekit_module_active)
                homekit.loop();
            if (alexa_module_active)
                alexa.loop();
            if (webinterface_module_active)
                web_interface.loop();
        }
    }

    led_strip.loop();
}

// init functions
bool SystemController::serial_port_begin    () {
    return serial_port.begin(115200);

}

bool SystemController::memory_begin         () {
    return memory.begin("lsys_store");

}

bool SystemController::system_begin         (bool first_init_flag){
    DBG_PRINTLN(SystemController, "system_begin()");
    serial_port.print("\n\n\n+------------------------------------------------+\n"
                         "|          Welcome to the XeWe Led OS            |\n"
                         "+------------------------------------------------+\n"
                         "|              ESP32 OS to control               |\n"
                         "|            addressable LED lights.             |\n"
                         "+------------------------------------------------+\n"
                         "|            Communication supported:            |\n"
                         "|                                                |\n"
                         "|                      Alexa                     |\n"
                         "|                     HomeKit                    |\n"
                         "|                   Web Browser                  |\n"
                         "|                 Serial Port CLI                |\n"
                         "+------------------------------------------------+\n");

    if (first_init_flag) {
        serial_port.print("\n+------------------------------------------------+\n"
                          "|       Alright lets set things up for you       |\n"
                          "+------------------------------------------------+\n");


        serial_port.print("+------------------------------------------------+\n"
                          "|                   Set up flow                  |\n"
                          "|                                                |\n"
                          "|    - Device Name                               |\n"
                          "|    - Led Strip                                 |\n"
                          "|    - WiFi                                      |\n"
                          "|    - Web Interface           REQUIRES WiFi     |\n"
                          "|    - Alexa                   REQUIRES WiFi     |\n"
                          "|    - HomeKit                 REQUIRES WiFi     |\n"
                          "+------------------------------------------------+\n");

        serial_port.print("\n+------------------------------------------------+\n"
                          "|                 Name Your Device               |\n"
                          "+------------------------------------------------+\n");
        serial_port.println("Set the name your device will proudly hold until\nthe last electron leaves it\nSample names: \"Desk Lights\" or \"Ceiling Lights\"\n");
        String device_name;
        bool confirmed = false;
        while (!confirmed) {
            device_name = serial_port.get_string("Enter device name: ");
            confirmed = serial_port.prompt_user_yn("Confirm name: " + device_name);
        }
        memory.write_str("device_name", device_name);
        memory.commit();
        serial_port.get_string("\nDevice name setup success!\n"
                               "Press enter to continue");
    }
    else {
        wifi_module_active         = memory.read_bool("wifi_mod_act");
        webinterface_module_active = memory.read_bool("webint_mod_act");
        alexa_module_active        = memory.read_bool("alexa_mod_act");
        homekit_module_active      = memory.read_bool("homekit_mod_act");

        serial_port.print(String("\n+------------------------------------------------+\n") +
                          "|       Current System Module Configuration      |\n" +
                          "|                                                |\n" +
                          "| WiFi          : " + (wifi_module_active ? "enabled " : "disabled") + "                       |\n" +
                          "| Web Interface : " + (webinterface_module_active ? "enabled " : "disabled") + "                       |\n" +
                          "| Alexa         : " + (alexa_module_active ? "enabled " : "disabled") + "                       |\n" +
                          "| HomeKit       : " + (homekit_module_active ? "enabled " : "disabled") + "                       |\n" +
                          "+------------------------------------------------+\n");
    }
    return true;
}

bool SystemController::led_strip_begin      (bool first_init_flag) {
    serial_port.print("\n+------------------------------------------------+\n"
                      "|                 Led Strip Init                 |\n"
                      "+------------------------------------------------+\n");

    static CRGB main_leds[LED_STRIP_NUM_LEDS_MAX];
    FastLED.addLeds<LED_STRIP_TYPE, PIN_LED_STRIP, LED_STRIP_COLOR_ORDER>(main_leds, LED_STRIP_NUM_LEDS_MAX).setCorrection( TypicalLEDStrip );
    led_strip.begin(main_leds, 10);

    if (first_init_flag) {

        int led_num_entry = serial_port.get_int("How many LEDs do you have connected?\nEnter a number: ");
        serial_port.println("");

        while (true) {
            if (led_num_entry < 0) {
                serial_port.println("LED number must be greater than 0");
            } else if (led_num_entry > LED_STRIP_NUM_LEDS_MAX) {
                serial_port.println("That's too many. Max supported LED: " + String(LED_STRIP_NUM_LEDS_MAX));
            } else if (led_num_entry <= LED_STRIP_NUM_LEDS_MAX){
                memory.write_uint16("led_len", led_num_entry);
                memory.commit();
                led_strip_reset(led_num_entry);
                break;
            }
        }

        serial_port.get_string("\nLED strip was set to green\n"
                               "If you don't see the green color check the\npin (GPIO), led type, and color order\n\n"
                               "LED setup success!\n"
                               "Press enter to continue");
    } else {
        led_strip_set_length        (memory.read_uint16 ("led_len"),       {false, false, false});
        led_strip_set_state         (memory.read_uint8 ("led_state"),      {false, false, false});
        led_strip_set_mode          (memory.read_uint8("led_mode"),        {false, false, false});
        led_strip_set_rgb           ({memory.read_uint8 ("led_r"),
                                      memory.read_uint8 ("led_g"),
                                      memory.read_uint8 ("led_b")},        {false, false, false});
        led_strip_set_brightness    (memory.read_uint8 ("led_bri"),        {false, false, false});

        uint32_t start_time = millis();
        while(millis() - start_time < 1000)
            led_strip.loop();

        led_strip_status();
        serial_port.println("\nLED setup success!");
    }
    return true;
}

bool SystemController::wifi_begin           (bool first_init_flag) {
    serial_port.print("\n+------------------------------------------------+\n"
                      "|                    WiFi Init                   |\n"
                      "+------------------------------------------------+\n");

    if (first_init_flag){
        wifi_reset(false);
        wifi_module_active = serial_port.prompt_user_yn("Would you like to connect to WiFi?\nThis allows LED control via Browser, Alexa,\nand Apple Home App (iPhone/iPad/Mac)");
        memory.write_bool("wifi_mod_act", wifi_module_active);
        serial_port.println("");
        if (wifi_module_active) {
            wifi_connect(true);
            serial_port.get_string("\nWiFi setup success!\n"
                                   "Press enter to continue");
            return wifi.is_connected();
        } else { // user decided not to set up the wifi
            return true;
        }
    }

    if (wifi_module_active) {
        return wifi_connect(false);
    }
    return false;
}

bool SystemController::web_server_begin     (bool first_init_flag) {
//    serial_port.print("\n+------------------------------------------------+\n"
//                      "|                 Web Server Init                |\n"
//                      "+------------------------------------------------+\n");

    if (wifi_module_active) {
//     depends on alexa
//        web_server.begin();
        // normal server begin(). this depends on whether alexa is used or not.
//        serial_port.println("Web Server setup success!");
        return true;
    }

    return false;
}

bool SystemController::web_interface_begin  (bool first_init_flag) {
    serial_port.print("\n+------------------------------------------------+\n"
                      "|               Web Interface Init               |\n"
                      "+------------------------------------------------+\n");

    // enter setup
    if (first_init_flag){
        // prompt user
        webinterface_module_active = serial_port.prompt_user_yn("Would you like to enable Web Interface Module?\nThis allows LED control via browser");
        memory.write_bool("webint_mod_act", webinterface_module_active);
        serial_port.get_string("\nWeb Interface setup success!\n"
                               "Device will be discoverable after auto restart\n"
                               "Press enter to continue");
    }
    else if (wifi_module_active && webinterface_module_active) {
        web_interface.begin(*this, web_server);
//        web_interface.broadcast_led_state("full");

        serial_port.println("WebInterface routes registered.");
        // depends on alexa.
//        web_server.onNotFound([this]() {
//            if (!alexa.get_instance().handleAlexaApiCall(web_server.uri(), web_server.arg("plain"))) {
//                web_server.send(404, "text/plain", "Endpoint not found.");
//            }
//        });
    serial_port.println(String("\nWeb Interface setup success!\n") +
                               "\nTo control LED from the browser, make sure that\n" +
                               "the device (phone/laptop) connected to the same\nWiFi: " + wifi.get_ssid() + "\n" +
                               "\nOpen in browser:\n" +
                               "http://" + wifi.get_local_ip());
    }

    return true;
}

bool SystemController::alexa_begin          (bool first_init_flag) {
    serial_port.print("\n+------------------------------------------------+\n"
                      "|                   Alexa Init                   |\n"
                      "+------------------------------------------------+\n");
    if (first_init_flag){
        // prompt user
        alexa_module_active = serial_port.prompt_user_yn("Would you like to enable Alexa Module?\nThis allows LED control via Amazon Alexa");
        memory.write_bool("alexa_mod_act", alexa_module_active);
        serial_port.get_string("\nAlexa setup success!\n"
                               "Device will be discoverable after auto restart\n"
                               "Press enter to continue");
    }
    else if (wifi_module_active && alexa_module_active) {
        alexa.begin(*this, web_server);

        web_server.onNotFound([this]() {
            if (!alexa.get_instance().handleAlexaApiCall(web_server.uri(), web_server.arg("plain"))) {
                web_server.send(404, "text/plain", "Endpoint not found.");
            }
        });
//        alexa.sync_state_with_system_controller("full");

        serial_port.println("Alexa setup success!");
        serial_port.println("\nTo control LED with Alexa, make sure that");
        serial_port.println("Alexa is connected to the same\nWiFi: " + wifi.get_ssid());
        serial_port.println("\nAsk Alexa to discover devices");
    }
    return true;
}

bool SystemController::homekit_begin        (bool first_init_flag) {
    serial_port.print("\n+------------------------------------------------+\n"
                      "|                  HomeKit Init                  |\n"
                      "+------------------------------------------------+\n");
    if (first_init_flag){
        // prompt user
        homekit_module_active = serial_port.prompt_user_yn("Would you like to enable HomeKit Module?\nThis allows LED control via Apple Home App");
        memory.write_bool("homekit_mod_act", homekit_module_active);
        serial_port.println("\nHomeKit setup success!");
        if (homekit_module_active)
            serial_port.println("Device will be discoverable after auto restart");
        else
            serial_port.println("You can enable it later using $homekit enable");

        serial_port.get_string("Press enter to continue");
    }
    else if (wifi_module_active && homekit_module_active) {
        homekit.begin(*this);

        uint32_t timestamp = millis();
        while(millis() - timestamp < 2000)
            homekit.loop();
//        homekit.sync_state();

        serial_port.println("HomeKit setup success!");
        serial_port.println("\nTo control LED with Home App on iPhone/iPad/Mac, ");
        serial_port.println("make sure that device is connected to the same\nWiFi: " + wifi.get_ssid());

        serial_port.println("\nScan this QR code:\nhttps://github.com/maxdokukin/XeWe-LedOS/blob/main/doc/HomeKit_Connect_QR.png");
        serial_port.println("Or go to Home App\nPress add device, using code 4663-7726");
    } else {
        serial_port.println("HomeKit setup skipped");
        serial_port.println("You can enable HomeKit using $homekit enable");
    }
    return true;
}

bool SystemController::command_parser_begin (bool first_init_flag) {
    DBG_PRINTLN(SystemController, "command_parser_begin()");

    if (first_init_flag)
        return true;

    serial_port.print("\n+------------------------------------------------+\n"
                      "|           Command Line Interface Init          |\n"
                      "+------------------------------------------------+\n"
                      "|     Use $help to see all available commands    |\n"
                      "+------------------------------------------------+\n");

    help_commands[0] =      { "",                    "Print all cmd available",                  0, [this](auto&){ print_help(); } };

    system_commands[0] =    { "help",              "Show this help message",                   0, [this](auto&){ system_print_help(); } };
    system_commands[1] =    { "reset",             "Reset everything (will restart)",          0, [this](auto&){ system_reset(); } };
    system_commands[2] =    { "restart",           "Restart system",                           0, [this](auto&){ system_restart(0); } };

    wifi_commands[0] =      { "help",                "Show this help message",                0, [this](auto&){ wifi_print_help(); } };
    wifi_commands[1] =      { "connect",             "Connect or reconnect to WiFi",          0, [this](auto&){ wifi_connect(true); } };
    wifi_commands[2] =      { "disconnect",          "Disconnect from WiFi",                  0, [this](auto&){ wifi_disconnect(); } };
    wifi_commands[3] =      { "reset",               "Clear saved WiFi credentials",          0, [this](auto&){ wifi_reset(true); } };
    wifi_commands[4] =      { "status",              "Show connection status, SSID, IP, MAC", 0, [this](auto&){ wifi_print_credentials(); } };
    wifi_commands[5] =      { "scan",                "List available WiFi networks",          0, [this](auto&){ wifi_get_available_networks(); } };

    led_strip_commands[0]  = { "help",           "Show this help message",      0, [this](auto&){ led_strip_print_help(); } };
    led_strip_commands[1]  = { "reset",          "Clear stored led data",       0, [this](auto&){ led_strip_reset(); } };
    led_strip_commands[2]  = { "set_mode",       "Set LED strip mode",          1, [this](auto& a){ led_strip_set_mode(a); } };
    led_strip_commands[3]  = { "set_rgb",        "Set RGB color",               3, [this](auto& a){ led_strip_set_rgb(a); } };
    led_strip_commands[4]  = { "set_r",          "Set red channel",             1, [this](auto& a){ led_strip_set_r(a); } };
    led_strip_commands[5]  = { "set_g",          "Set green channel",           1, [this](auto& a){ led_strip_set_g(a); } };
    led_strip_commands[6]  = { "set_b",          "Set blue channel",            1, [this](auto& a){ led_strip_set_b(a); } };
    led_strip_commands[7]  = { "set_hsv",        "Set HSV color",               3, [this](auto& a){ led_strip_set_hsv(a); } };
    led_strip_commands[8]  = { "set_hue",        "Set hue channel",             1, [this](auto& a){ led_strip_set_hue(a); } };
    led_strip_commands[9]  = { "set_sat",        "Set saturation channel",      1, [this](auto& a){ led_strip_set_sat(a); } };
    led_strip_commands[10] = { "set_val",        "Set value channel",           1, [this](auto& a){ led_strip_set_val(a); } };
    led_strip_commands[11] = { "set_brightness", "Set global brightness",       1, [this](auto& a){ led_strip_set_brightness(a); } };
    led_strip_commands[12] = { "set_state",      "Set on/off state",            1, [this](auto& a){ led_strip_set_state(a); } };
    led_strip_commands[13] = { "turn_on",        "Turn strip on",               0, [this](auto&){ led_strip_turn_on(); } };
    led_strip_commands[14] = { "turn_off",       "Turn strip off",              0, [this](auto&){ led_strip_turn_off(); } };
    led_strip_commands[15] = { "set_length",     "Set new number of LEDs",      1, [this](auto& a){ led_strip_set_length(a); } };

    ram_commands[0] = { "help",     "Show this help message",                           0, [this](auto&){ ram_print_help(); } };
    ram_commands[1] = { "status",   "Show overall heap stats (total/free/high-water)",  0, [this](auto&){ ram_status(); } };
    ram_commands[2] = { "free",     "Print current free heap bytes",                    0, [this](auto&){ ram_free(); } };
    ram_commands[3] = { "watch",    "Continuously print free heap every <ms>",          1, [this](auto& a){ ram_watch(a); } };

    command_groups[0] = { "help",       help_commands,          HELP_CMD_COUNT      };
    command_groups[1] = { "system",     system_commands,        SYSTEM_CMD_COUNT    };
    command_groups[2] = { "wifi",       wifi_commands,          WIFI_CMD_COUNT      };
    command_groups[3] = { "led",        led_strip_commands,     LED_STRIP_CMD_COUNT };
    command_groups[4] = { "ram",        ram_commands,           RAM_CMD_COUNT       };

    command_parser.begin(command_groups, CMD_GROUP_COUNT);
    return true;
}


// --- HELP ---
void SystemController::print_help(){
    DBG_PRINTLN(SystemController, "print_help()");
    system_print_help();
    wifi_print_help();
    led_strip_print_help();
    ram_print_help();
}

// --- SYSTEM ---
void SystemController::system_print_help(){
    DBG_PRINTLN(SystemController, "system_print_help()");
    serial_port.print_spacer();
    serial_port.println("System commands:");
    for (size_t i = 0; i < SYSTEM_CMD_COUNT; ++i) {
        const auto &cmd = system_commands[i];
        serial_port.print("  $system ");
        serial_port.print(cmd.name);
        serial_port.print(" - ");
        serial_port.print(cmd.description);
        serial_port.print(", argument count: ");
        serial_port.println(String(cmd.arg_count));
    }
    serial_port.print_spacer();
}

void SystemController::system_reset(){
    DBG_PRINTLN(SystemController, "system_reset()");
    serial_port.println("\n+------------------------------------------------+\n"
                        "|                  Resetting...                  |\n"
                        "+------------------------------------------------+\n");
    memory.reset();
    serial_port.println("NOTE: You need to manually remove device from\nAlexa and Apple Home Apps!");
    serial_port.get_string("System reset success!\n\nPress enter to restart");
    system_restart(1000);
}

void SystemController::system_restart(uint16_t delay_before){
    DBG_PRINTLN(SystemController, "system_restart()");
    serial_port.println("\n+------------------------------------------------+\n"
                        "|                 Restarting...                  |\n"
                        "+------------------------------------------------+\n");
    delay(delay_before);
    ESP.restart();
}


// --- LED handlers ---
void                            SystemController::led_strip_print_help            () {
    DBG_PRINTLN(SystemController, "led_strip_print_help()");
    serial_port.print_spacer();
    serial_port.println("Led commands:");
    for (size_t i = 0; i < LED_STRIP_CMD_COUNT; ++i) {
        const auto &cmd = led_strip_commands[i];
        serial_port.print("  $led ");
        serial_port.print(cmd.name);
        serial_port.print(" - ");
        serial_port.print(cmd.description);
        serial_port.print(", argument count: ");
        serial_port.println(String(cmd.arg_count));
    }
    serial_port.print_spacer();
}

void                            SystemController::led_strip_reset (uint16_t led_num){
    DBG_PRINTLN(SystemController, "led_strip_reset()");
    led_strip_set_length        (led_num,            {false, false, false});
    led_strip_set_state         (1,             {false, false, false});
    led_strip_set_mode          (0,             {false, false, false});
    led_strip_set_rgb           ({0, 255,  0},   {false, false, false});
    led_strip_set_brightness    (10,           {false, false, false});
    led_strip_status();

    uint32_t start_time = millis();
    while(millis() - start_time < 1000)
        led_strip.loop();
}

void SystemController::led_strip_status() {
    DBG_PRINTLN(SystemController, "led_strip_status()");
    serial_port.println("LED Strip Config:");
    serial_port.println("    Pin          : GPIO" + String(PIN_LED_STRIP));
    serial_port.println("    Type         : " + String(TO_STRING(LED_STRIP_TYPE)));
    serial_port.println("    Color order  : " + String(TO_STRING(LED_STRIP_COLOR_ORDER)));
    serial_port.println("    Max LED      : " + String(LED_STRIP_NUM_LEDS_MAX));
    serial_port.println("    These can only be changed in the src/Config.h\n");
    serial_port.println("    Length       : " + String(led_strip.get_length()));
    serial_port.println("    State        : " + String(led_strip.get_state() ? "ON" : "OFF"));
    serial_port.println("    Brightness   : " + String(led_strip.get_brightness()));
    serial_port.println("    Mode         : " + led_strip.get_mode_name());
    serial_port.println("    R            : " + String(led_strip.get_r()));
    serial_port.println("    G            : " + String(led_strip.get_g()));
    serial_port.println("    B            : " + String(led_strip.get_b()));
}

void                            SystemController::led_strip_set_mode              (const String& args) {
    DBG_PRINTF(SystemController, "led_strip_set_mode(args=\"%s\")\n", args.c_str());
    uint8_t new_mode = static_cast<uint8_t>(args.toInt());
    led_strip_set_mode(new_mode, {true, true, true});
}

void                            SystemController::led_strip_set_mode              (uint8_t new_mode, std::array<bool, 3> update_flags) {
    DBG_PRINTF(SystemController, "led_strip_set_mode(new_mode=%u, update_flags=[%d,%d,%d])\n", new_mode, update_flags[0], update_flags[1], update_flags[2]);
    led_strip.set_mode(new_mode);
    memory.write_uint8("led_mode", new_mode);
    memory.commit();

    if (update_flags[0])
        web_interface.broadcast_led_state("mode");
    if (update_flags[1])
        alexa.sync_state_with_system_controller("mode");
    if (update_flags[2])
        homekit.sync_state();
}

void                            SystemController::led_strip_set_rgb               (const String& args) {
    DBG_PRINTF(SystemController, "led_strip_set_rgb(args=\"%s\")\n", args.c_str());
    int i1 = args.indexOf(' '), i2 = args.indexOf(' ', i1 + 1);
    uint8_t new_r = args.substring(0, i1).toInt();
    uint8_t new_g = args.substring(i1 + 1, i2).toInt();
    uint8_t new_b = args.substring(i2 + 1).toInt();
    led_strip_set_rgb({new_r, new_g, new_b}, {true, true, true});
}

void                            SystemController::led_strip_set_rgb               (std::array<uint8_t, 3> new_rgb, std::array<bool, 3> update_flags) {
    DBG_PRINTF(SystemController, "led_strip_set_rgb(new_rgb=[%u,%u,%u], update_flags=[%d,%d,%d])\n", new_rgb[0], new_rgb[1], new_rgb[2], update_flags[0], update_flags[1], update_flags[2]);
if (!in_range(new_rgb[0], (uint8_t)0, (uint8_t)255) ||
    !in_range(new_rgb[1], (uint8_t)0, (uint8_t)255) ||
    !in_range(new_rgb[2], (uint8_t)0, (uint8_t)255)) {
        serial_port.println("RGB should be in the range 0 to 255");
        return;
    }

    led_strip.set_rgb(new_rgb[0], new_rgb[1], new_rgb[2]);
    memory.write_uint8("led_r", new_rgb[0]);
    memory.write_uint8("led_g", new_rgb[1]);
    memory.write_uint8("led_b", new_rgb[2]);
    memory.commit();

    if (update_flags[0])
        web_interface.broadcast_led_state("color");
    if (update_flags[1])
        alexa.sync_state_with_system_controller("color");
    if (update_flags[2])
        homekit.sync_state();
}

void                            SystemController::led_strip_set_r                 (const String& args) {
    DBG_PRINTF(SystemController, "led_strip_set_r(args=\"%s\")\n", args.c_str());
    uint8_t new_r = static_cast<uint8_t>(args.toInt());
    led_strip_set_r(new_r, {true, true, true});
}

void                            SystemController::led_strip_set_r                 (uint8_t new_r, std::array<bool, 3> update_flags) {
    DBG_PRINTF(SystemController, "led_strip_set_r(new_r=%u, update_flags=[%d,%d,%d])\n", new_r, update_flags[0], update_flags[1], update_flags[2]);
    if (!in_range(new_r, (uint8_t)0, (uint8_t)255)) {
        serial_port.println("R should be in the range 0 to 255");
        return;
    }

    led_strip.set_r(new_r);
    memory.write_uint8("led_r", new_r);
    memory.commit();

    if (update_flags[0])
        web_interface.broadcast_led_state("color");
    if (update_flags[1])
        alexa.sync_state_with_system_controller("color");
    if (update_flags[2])
        homekit.sync_state();
}

void                            SystemController::led_strip_set_g                 (const String& args) {
    DBG_PRINTF(SystemController, "led_strip_set_g(args=\"%s\")\n", args.c_str());
    uint8_t new_g = static_cast<uint8_t>(args.toInt());
    led_strip_set_g(new_g, {true, true, true});
}

void                            SystemController::led_strip_set_g                 (uint8_t new_g, std::array<bool, 3> update_flags) {
    DBG_PRINTF(SystemController, "led_strip_set_g(new_g=%u, update_flags=[%d,%d,%d])\n", new_g, update_flags[0], update_flags[1], update_flags[2]);
    if (!in_range(new_g, (uint8_t)0, (uint8_t)255)) {
        serial_port.println("G should be in the range 0 to 255");
        return;
    }

    led_strip.set_g(new_g);
    memory.write_uint8("led_g", new_g);
    memory.commit();

    if (update_flags[0])
        web_interface.broadcast_led_state("color");
    if (update_flags[1])
        alexa.sync_state_with_system_controller("color");
    if (update_flags[2])
        homekit.sync_state();
}

void                            SystemController::led_strip_set_b                 (const String& args) {
    DBG_PRINTF(SystemController, "led_strip_set_b(args=\"%s\")\n", args.c_str());
    uint8_t new_b = static_cast<uint8_t>(args.toInt());
    led_strip_set_b(new_b, {true, true, true});
}

void                            SystemController::led_strip_set_b                 (uint8_t new_b, std::array<bool, 3> update_flags) {
    DBG_PRINTF(SystemController, "led_strip_set_b(new_b=%u, update_flags=[%d,%d,%d])\n", new_b, update_flags[0], update_flags[1], update_flags[2]);
    if (!in_range(new_b, (uint8_t)0, (uint8_t)255)) {
        serial_port.println("B should be in the range 0 to 255");
        return;
    }

    led_strip.set_b(new_b);
    memory.write_uint8("led_b", new_b);
    memory.commit();

    if (update_flags[0])
        web_interface.broadcast_led_state("color");
    if (update_flags[1])
        alexa.sync_state_with_system_controller("color");
    if (update_flags[2])
        homekit.sync_state();
}

void                            SystemController::led_strip_set_hsv               (const String& args) {
    DBG_PRINTF(SystemController, "led_strip_set_hsv(args=\"%s\")\n", args.c_str());
    int i1 = args.indexOf(' '), i2 = args.indexOf(' ', i1 + 1);
    uint8_t new_hue = args.substring(0, i1).toInt();
    uint8_t new_sat = args.substring(i1 + 1, i2).toInt();
    uint8_t new_val = args.substring(i2 + 1).toInt();
    led_strip_set_hsv({new_hue, new_sat, new_val}, {true, true, true});
}

void                            SystemController::led_strip_set_hsv               (std::array<uint8_t, 3> new_hsv, std::array<bool, 3> update_flags) {
    DBG_PRINTF(SystemController, "led_strip_set_hsv(new_hsv=[%u,%u,%u], update_flags=[%d,%d,%d])\n", new_hsv[0], new_hsv[1], new_hsv[2], update_flags[0], update_flags[1], update_flags[2]);
    if (!in_range(new_hsv[0], (uint8_t)0, (uint8_t)255) ||
        !in_range(new_hsv[1], (uint8_t)0, (uint8_t)255) ||
        !in_range(new_hsv[2], (uint8_t)0, (uint8_t)255)) {
        serial_port.println("HSV should be in the range 0 to 255");
        return;
    }

    led_strip.set_hsv(new_hsv[0], new_hsv[1], new_hsv[2]);
    memory.write_uint8("led_r", led_strip.get_r());
    memory.write_uint8("led_g", led_strip.get_g());
    memory.write_uint8("led_b", led_strip.get_b());
    memory.commit();


    if (update_flags[0])
        web_interface.broadcast_led_state("color");
    if (update_flags[1])
        alexa.sync_state_with_system_controller("color");
    if (update_flags[2])
        homekit.sync_state();
}

void                            SystemController::led_strip_set_hue               (const String& args) {
    DBG_PRINTF(SystemController, "led_strip_set_hue(args=\"%s\")\n", args.c_str());
    uint8_t new_hue = static_cast<uint8_t>(args.toInt());
    led_strip_set_hue(new_hue, {true, true, true});
}

void                            SystemController::led_strip_set_hue               (uint8_t new_hue, std::array<bool, 3> update_flags) {
    DBG_PRINTF(SystemController, "led_strip_set_hue(new_hue=%u, update_flags=[%d,%d,%d])\n", new_hue, update_flags[0], update_flags[1], update_flags[2]);
    if (!in_range(new_hue, (uint8_t)0, (uint8_t)255)) {
        serial_port.println("Hue should be in the range 0 to 255");
        return;
    }

    led_strip.set_h(new_hue);
    memory.write_uint8("led_r", led_strip.get_r());
    memory.write_uint8("led_g", led_strip.get_g());
    memory.write_uint8("led_b", led_strip.get_b());
    memory.commit();

    if (update_flags[0])
        web_interface.broadcast_led_state("color");
    if (update_flags[1])
        alexa.sync_state_with_system_controller("color");
    if (update_flags[2])
        homekit.sync_state();
}

void                            SystemController::led_strip_set_sat               (const String& args) {
    DBG_PRINTF(SystemController, "led_strip_set_sat(args=\"%s\")\n", args.c_str());
    uint8_t new_sat = static_cast<uint8_t>(args.toInt());
    led_strip_set_sat(new_sat, {true, true, true});
}

void                            SystemController::led_strip_set_sat               (uint8_t new_sat, std::array<bool, 3> update_flags) {
    DBG_PRINTF(SystemController, "led_strip_set_sat(new_sat=%u, update_flags=[%d,%d,%d])\n", new_sat, update_flags[0], update_flags[1], update_flags[2]);
    if (!in_range(new_sat, (uint8_t)0, (uint8_t)255)) {
        serial_port.println("Sat should be in the range 0 to 255");
        return;
    }

    led_strip.set_s(new_sat);
    memory.write_uint8("led_r", led_strip.get_r());
    memory.write_uint8("led_g", led_strip.get_g());
    memory.write_uint8("led_b", led_strip.get_b());
    memory.commit();

    if (update_flags[0])
        web_interface.broadcast_led_state("color");
    if (update_flags[1])
        alexa.sync_state_with_system_controller("color");
    if (update_flags[2])
        homekit.sync_state();
}

void                            SystemController::led_strip_set_val               (const String& args) {
    DBG_PRINTF(SystemController, "led_strip_set_val(args=\"%s\")\n", args.c_str());
    uint8_t new_val = static_cast<uint8_t>(args.toInt());
    led_strip_set_val(new_val, {true, true, true});
}

void                            SystemController::led_strip_set_val               (uint8_t new_val, std::array<bool, 3> update_flags) {
    DBG_PRINTF(SystemController, "led_strip_set_val(new_val=%u, update_flags=[%d,%d,%d])\n", new_val, update_flags[0], update_flags[1], update_flags[2]);
    if (!in_range(new_val, (uint8_t)0, (uint8_t)255)) {
        serial_port.println("Val should be in the range 0 to 255");
        return;
    }

    led_strip.set_v(new_val);
    memory.write_uint8("led_r", led_strip.get_r());
    memory.write_uint8("led_g", led_strip.get_g());
    memory.write_uint8("led_b", led_strip.get_b());
    memory.commit();

    if (update_flags[0])
        web_interface.broadcast_led_state("color");
    if (update_flags[1])
        alexa.sync_state_with_system_controller("color");
    if (update_flags[2])
        homekit.sync_state();
}

void                            SystemController::led_strip_set_brightness        (const String& args) {
    DBG_PRINTF(SystemController, "led_strip_set_brightness(args=\"%s\")\n", args.c_str());
    uint8_t new_brightness = static_cast<uint8_t>(args.toInt());
    led_strip_set_brightness(new_brightness, {true, true, true});

}

void                            SystemController::led_strip_set_brightness        (uint8_t new_brightness, std::array<bool, 3> update_flags) {
    DBG_PRINTF(SystemController, "led_strip_set_brightness(new_brightness=%u, update_flags=[%d,%d,%d])\n", new_brightness, update_flags[0], update_flags[1], update_flags[2]);
    if (!in_range(new_brightness, (uint8_t)0, (uint8_t)255)) {
        serial_port.println("Brightness should be in the range 0 to 255");
        return;
    }

    led_strip.set_brightness(new_brightness);
    memory.write_uint8("led_bri", new_brightness);
    memory.commit();

    if (update_flags[0])
        web_interface.broadcast_led_state("brightness");
    if (update_flags[1])
        alexa.sync_state_with_system_controller("brightness");
    if (update_flags[2])
        homekit.sync_state();
}

void                            SystemController::led_strip_set_state             (const String& args) {
    DBG_PRINTF(SystemController, "led_strip_set_state(args=\"%s\")\n", args.c_str());
    bool new_state = static_cast<bool>(args.toInt());
    led_strip_set_state(new_state, {true, true, true});
}

void                            SystemController::led_strip_set_state             (bool new_state, std::array<bool, 3> update_flags) {
    DBG_PRINTF(SystemController, "led_strip_set_state(new_state=%d, update_flags=[%d,%d,%d])\n", new_state, update_flags[0], update_flags[1], update_flags[2]);
    led_strip.set_state(new_state);
    memory.write_uint8("led_state", new_state);
    memory.commit();

    if (update_flags[0])
        web_interface.broadcast_led_state("state");
    if (update_flags[1])
        alexa.sync_state_with_system_controller("state");
    if (update_flags[2])
        homekit.sync_state();
}

void                            SystemController::led_strip_turn_on               () {
    DBG_PRINTLN(SystemController, "led_strip_turn_on()");
    led_strip_turn_on({true, true, true});

}

void                            SystemController::led_strip_turn_on               (std::array<bool, 3> update_flags) {
    DBG_PRINTF(SystemController, "led_strip_turn_on(update_flags=[%d,%d,%d])\n", update_flags[0], update_flags[1], update_flags[2]);
    led_strip.turn_on();
    memory.write_uint8("led_state", 1);
    memory.commit();

    if (update_flags[0])
        web_interface.broadcast_led_state("state");
    if (update_flags[1])
        alexa.sync_state_with_system_controller("state");
    if (update_flags[2])
        homekit.sync_state();
}

void                            SystemController::led_strip_turn_off              () {
    DBG_PRINTLN(SystemController, "led_strip_turn_off()");
    led_strip_turn_off({true, true, true});

}

void                            SystemController::led_strip_turn_off              (std::array<bool, 3> update_flags) {
    DBG_PRINTF(SystemController, "led_strip_turn_off(update_flags=[%d,%d,%d])\n", update_flags[0], update_flags[1], update_flags[2]);
    led_strip.turn_off();
    memory.write_uint8("led_state", 0);
    memory.commit();

    if (update_flags[0])
        web_interface.broadcast_led_state("state");
    if (update_flags[1])
        alexa.sync_state_with_system_controller("state");
    if (update_flags[2])
        homekit.sync_state();
}

void                            SystemController::led_strip_set_length            (const String& args) {
    DBG_PRINTF(SystemController, "led_strip_set_length(args=\"%s\")\n", args.c_str());
    uint16_t new_length = static_cast<uint16_t>(args.toInt());
    led_strip_set_length(new_length, {false, false, false});
}

void                            SystemController::led_strip_set_length            (uint16_t new_length, std::array<bool, 3> update_flags) {
    DBG_PRINTF(SystemController, "led_strip_set_length(new_length=%u, update_flags=[%d,%d,%d])\n", new_length, update_flags[0], update_flags[1], update_flags[2]);
    led_strip.set_length(new_length);
    memory.write_uint16("led_len", new_length);
    memory.commit();

    if (update_flags[0])
        web_interface.broadcast_led_state("length");
    if (update_flags[1])
        alexa.sync_state_with_system_controller("length");
    if (update_flags[2])
        homekit.sync_state();
}

std::array<uint8_t, 3>          SystemController::led_strip_get_target_rgb        ()                      const {
    DBG_PRINTLN(SystemController, "led_get_target_rgb() const {");
    return led_strip.get_target_rgb();
}

std::array<uint8_t, 3>          SystemController::led_strip_get_target_hsv        ()                      const {
    DBG_PRINTLN(SystemController, "led_get_target_hsv() const {");
    return led_strip.get_target_hsv();
}

String                          SystemController::led_strip_get_color_hex         ()                      const {
    DBG_PRINTLN(SystemController, "String SystemController::led_strip_get_color_hex() const {");
    std::array<uint8_t, 3> target_rgb = led_strip.get_target_rgb();
    char buf[8];
    sprintf(buf, "#%02X%02X%02X", target_rgb[0], target_rgb[1], target_rgb[2]);
    return String(buf);
}

uint8_t                         SystemController::led_strip_get_brightness        ()                      const {
    DBG_PRINTLN(SystemController, "uint8_t SystemController::led_strip_get_brightness() const {");
    return led_strip.get_brightness();
}

bool                            SystemController::led_strip_get_state             ()                      const {
    DBG_PRINTLN(SystemController, "bool SystemController::led_strip_get_state() const {");
    return led_strip.get_state();
}

uint8_t                         SystemController::led_strip_get_mode_id           ()                            {
    DBG_PRINTLN(SystemController, "uint8_t SystemController::led_strip_get_mode() const {");
    return led_strip.get_mode_id();
}


//////WIFI/////
bool SystemController::wifi_connect(bool prompt_for_credentials) {
    DBG_PRINTF(SystemController, "wifi_connect(prompt_for_credentials=%d)\n", prompt_for_credentials);
    if (wifi.is_connected()) {
        serial_port.println("Already connected");
        wifi_print_credentials();
        serial_port.println("Use '$wifi reset' to change network");
        return true;
    }
    String ssid, pwd;
    if (wifi_read_stored_credentials(ssid, pwd)) {
        serial_port.println("Stored WiFi credentials found");
        if (wifi_join(ssid, pwd)) {
            return true;
        } else {
            serial_port.println("Stored WiFi credentials not valid.");
            if (!prompt_for_credentials) {
                serial_port.println("Use '$wifi reset' to reset credentials");
            }
        }
    } else {
        serial_port.println("Stored WiFi credentials not found");
        if (!prompt_for_credentials) {
            serial_port.println("Type '$wifi connect' to select a new network");
        }
    }
    if (!prompt_for_credentials) {
        return false;
    }
    while (!wifi.is_connected()) {
        uint8_t prompt_status = wifi_prompt_for_credentials(ssid, pwd);
        if (prompt_status == 2) {
            serial_port.println("Terminated WiFi setup");
            return false;
        } else if (prompt_status == 1) {
            serial_port.println("Invalid choice");
            continue;
        } else if (prompt_status == 0) {
            if (wifi_join(ssid, pwd)) {
                return true;
            } else {
                continue;
            }
        }
    }
    return false;
}

void SystemController::wifi_print_credentials() {
    DBG_PRINTLN(SystemController, "wifi_print_credentials()");
    if (!wifi.is_connected()) {
        serial_port.println("\nWiFi not connected");
        return;
    }
    serial_port.println("\nConnected to " + wifi.get_ssid());
    serial_port.println("Local IP: " + wifi.get_local_ip());
    serial_port.println("MAC: " + wifi.get_mac_address());
}

bool SystemController::wifi_read_stored_credentials(String& ssid, String& pwd) {
    DBG_PRINTLN(SystemController, "wifi_read_stored_credentials(...)");
    if (!memory.read_bit("wifi_flags", 0)) {
        return false;
    }
    ssid = memory.read_str("wifi_name");
    pwd  = memory.read_str("wifi_pass");
    return true;
}

uint8_t SystemController::wifi_prompt_for_credentials(String& ssid, String& pwd) {
    DBG_PRINTLN(SystemController, "wifi_prompt_for_credentials(...)");
    memory.write_bit("wifi_flags", 0, 0);
    memory.commit();

    std::vector<String> networks = wifi_get_available_networks();
    int choice = serial_port.get_int("\nSelect network by number, or enter -1 to exit: ");
    if (choice == -1) {
        return 2;
    } else if (choice >= 0 && choice < (int)networks.size()) {
        ssid = networks[choice];
    } else {
        return 1;
    }
    pwd = serial_port.get_string("Selected: '" + ssid + "'\nPassword: ");
    return 0;
}

bool SystemController::wifi_join(const String& ssid, const String& pwd) {
    DBG_PRINTF(SystemController, "wifi_join(ssid=\"%s\", pwd=\"%s\")\n", ssid.c_str(), pwd.c_str());
    serial_port.println("Connecting to '" + ssid + "'...");
    if (wifi.connect(ssid, pwd)) {
        wifi_print_credentials();
        memory.write_bit("wifi_flags", 0, 1);
        memory.write_str("wifi_name", ssid);
        memory.write_str("wifi_pass", pwd);
        memory.commit();

        return true;
    }
    serial_port.println("Failed to connect to '" + ssid + "'");
    return false;
}

bool SystemController::wifi_disconnect() {
    DBG_PRINTLN(SystemController, "wifi_disconnect()");
    if (!wifi.is_connected()) {
        serial_port.println("Not currently connected to WiFi");
        return true;
    }
    String current_ssid = wifi.get_ssid();
    wifi.disconnect();
    serial_port.println("Disconnected from '" + current_ssid + "'");
    return true;
}

bool SystemController::wifi_reset(bool print_info) {
    DBG_PRINTLN(SystemController, "wifi_reset()");
    memory.write_bit("wifi_flags", 0, 0);
    memory.write_str("wifi_name", "");
    memory.write_str("wifi_pass", "");
    memory.commit();

    if (wifi.is_connected()) {
        wifi.disconnect();
    }
    if (print_info) {
        serial_port.println("WiFi credentials have been reset.\nUse '$wifi connect' to select a new network");
    }
    return true;
}

std::vector<String> SystemController::wifi_get_available_networks() {
    DBG_PRINTLN(SystemController, "wifi_get_available_networks()");
    serial_port.println("\nScanning available networks...");
    std::vector<String> networks = wifi.get_available_networks();
    serial_port.println("Available networks:");
    for (size_t i = 0; i < networks.size(); ++i) {
        serial_port.println(String(i) + ": " + networks[i]);
    }
    return networks;
}

void SystemController::wifi_print_help() {
    DBG_PRINTLN(SystemController, "wifi_print_help()");
    serial_port.println("WiFi commands:");
    for (size_t i = 0; i < WIFI_CMD_COUNT; ++i) {
        const auto &cmd = wifi_commands[i];
        serial_port.println("  $wifi " + String(cmd.name) + " - " + String(cmd.description) + ", argument count: " + String(cmd.arg_count));
    }
}


/// RAM ///
void SystemController::ram_print_help() {
    DBG_PRINTLN(SystemController, "ram_print_help()");
    serial_port.println("RAM commands:");
    for (size_t i = 0; i < RAM_CMD_COUNT; ++i) {
        const auto &cmd = ram_commands[i];
        serial_port.println("  $ram " + String(cmd.name) + " - " + String(cmd.description) + ", argument count: " + String(cmd.arg_count));
    }
}

void SystemController::ram_status() {
    DBG_PRINTLN(SystemController, "ram_status()");
    serial_port.print_spacer();
    uint32_t total       = ESP.getHeapSize();
    uint32_t free_bytes  = ESP.getFreeHeap();
    uint32_t min_free    = ESP.getMinFreeHeap();
    uint32_t max_alloc   = ESP.getMaxAllocHeap();
    uint32_t used        = total - free_bytes;
    float    pct_used    = used * 100.0f / total;
    float    pct_free    = free_bytes * 100.0f / total;
    serial_port.println("=== RAM Status ===");
    serial_port.print("Total heap:             ");
    serial_port.println(String(total));
    serial_port.print("Free heap:              ");
    serial_port.println(String(free_bytes));
    serial_port.print("Used heap:              ");
    serial_port.println(String(used));
    serial_port.print("Min ever free heap:     ");
    serial_port.println(String(min_free));
    serial_port.print("Max allocatable block:  ");
    serial_port.println(String(max_alloc));
    const uint8_t bar_width = 10;
    uint8_t used_bars = (uint8_t)((pct_used * bar_width / 100.0f) + 0.5f);
    serial_port.print("Usage: |");
    for (uint8_t i = 0; i < used_bars; ++i) {
        serial_port.print("x");
    }
    for (uint8_t i = used_bars; i < bar_width; ++i) {
        serial_port.print(".");
    }
    serial_port.print("| ");
    serial_port.println(String(pct_used, 1) + "%");
    uint8_t free_bars = bar_width - used_bars;
    serial_port.print("Free : |");
    for (uint8_t i = 0; i < free_bars; ++i) {
        serial_port.print("x");
    }
    for (uint8_t i = free_bars; i < bar_width; ++i) {
        serial_port.print(".");
    }
    serial_port.print("| ");
    serial_port.println(String(pct_free, 1) + "%");
    serial_port.print_spacer();
}

void SystemController::ram_free() {
    DBG_PRINTLN(SystemController, "ram_free()");
    serial_port.print("Free heap: ");
    serial_port.println(String(ESP.getFreeHeap()));
}

void SystemController::ram_watch(const String& args) {
    DBG_PRINTF(SystemController, "ram_watch(args=\"%s\")\n", args.c_str());
    uint32_t interval = (uint32_t)args.toInt();
    serial_port.println("Entering RAM watch. Press any key to exit.");

    ram_print_ticker.attach(int(interval / 1000), [this]() {
        serial_port.print("Free heap: ");
        serial_port.println(String(ESP.getFreeHeap()));
  });
}

