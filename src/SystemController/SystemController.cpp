// SystemController.cpp
#include "SystemController.h"

// SystemController.cpp

// Ensure you have the necessary includes at the top of your SystemController.cpp,
// for example:
// #include "SystemController.h"
// #include "../Alexa/Alexa.h" // If Alexa.h is in a parallel directory, adjust path

SystemController::SystemController(CRGB* leds_ptr)
  : serial_port(115200)
  , wifi("ESP32-C3-Device")
  , memory(512)
  , storage()
  , led_strip(leds_ptr)
  , sync_web_server_(80) // Initialize the synchronous ESP32 core WebServer
  , web_interface_module_(*this, sync_web_server_) // Pass it to your WebServer wrapper
  , alexa_module_(*this) // Initialize Alexa module, passing this SystemController
{
    serial_port.println("\n\n\n");
    if(!storage.init()){
        serial_port.println("Failed to init NVS");
        // Consider a more robust error handling here, like a halt or safe mode,
        // as system_restart() might lead to a boot loop if NVS init always fails.
        system_restart();
    }
    serial_port.print("+------------------------------------------------+\n"
                      "|          Welcome to the XeWe Led OS            |\n"
                      "+------------------------------------------------+\n"
                      "|        ESP32 Lightweight OS to control         |\n"
                      "|            addressable LED lights.             |\n"
                      "+------------------------------------------------+\n"
                      "|            Communication supported:            |\n"
                      "|             Serial Port, Web Server            |\n"
                      "+------------------------------------------------+\n"
                      "|         Communication to be supported:         |\n"
                      "|          Alexa, Homekit, Yandex-Alisa          |\n"
                      "+------------------------------------------------+\n");

    if (storage.is_first_startup()){
        serial_port.print("+------------------------------------------------+\n"
                          "|           Entering initial setup mode          |\n"
                          "+------------------------------------------------+\n");
        system_reset(); // This likely includes LED and WiFi reset calls
        storage.reset_first_startup_flag();
        serial_port.print("+------------------------------------------------+\n"
                          "|           Initial setup mode success!          |\n"
                          "+------------------------------------------------+\n");
        system_restart(); // Restart after initial setup to apply fresh state
    }

    serial_port.print("+------------------------------------------------+\n"
                      "|                   WiFi Setup                   |\n"
                      "+------------------------------------------------+\n");
    wifi_connect(false); // Attempt to connect with stored credentials, don't prompt if fails

    if (wifi.is_connected()) {
        serial_port.print("+------------------------------------------------+\n"
                          "|                 WebServer Setup                |\n"
                          "+------------------------------------------------+\n");

        // Step 1: WebInterface module registers its specific routes.
        // (Ensure WebInterface::begin() NO LONGER calls server.begin() or sets its own onNotFound)
        web_interface_module_.begin();
        serial_port.println("WebInterface routes registered.");

        // Step 2: SystemController defines the FINAL onNotFound handler.
        // This handler will try to pass the request to Alexa if no other route matched.
        sync_web_server_.onNotFound([this]() { // Capture 'this' to access member variables
            String requestBody = "";
            if (sync_web_server_.hasArg("plain")) {
                requestBody = sync_web_server_.arg("plain");
            } else if (sync_web_server_.args() > 0) {
                // Fallback for other types of arguments if "plain" isn't present.
                // Espalexa::handleAlexaApiCall expects the body for commands.
                // If using server.arg(0), ensure it correctly represents the body Espalexa needs.
                // For GETs (like discovery /api calls), body will be empty, which is fine.
                requestBody = sync_web_server_.arg(0); // Or combine all args if necessary
            }

            // Assumes alexa_module_.getEspalexaCoreInstance() exists and returns Espalexa&
            if (!alexa_module_.getEspalexaCoreInstance().handleAlexaApiCall(sync_web_server_.uri(), requestBody)) {
                // If Espalexa didn't handle it, then it's a true 404 for your system.
                sync_web_server_.send(404, "text/plain", "SystemController: Endpoint not found.");
            }
        });
        serial_port.println("Main onNotFound handler registered.");

        serial_port.println("To control LED from the browser, make sure that");
        serial_port.println("the device (laptop/phone) connected to the same\nWiFi: " + wifi.get_ssid());
        serial_port.println("Open in browser:\nhttp://" + wifi.get_local_ip());

        serial_port.print("+------------------------------------------------+\n"
                          "|                   Alexa Setup                  |\n"
                          "+------------------------------------------------+\n");
        // Step 3: Alexa module begins.
        // Espalexa will register its specific routes (e.g., /description.xml).
        // Espalexa v2.7.0 WILL CALL sync_web_server_.begin() INTERNALLY.
        // This becomes the single, correctly timed server start.
        alexa_module_.begin(sync_web_server_);

        serial_port.println("Alexa support initialized. Ask Alexa to discover devices.");

    } else {
        serial_port.print("+------------------------------------------------+\n"
                          "| WebServer & Alexa Setup SKIPPED (No WiFi)      |\n"
                          "| Use $wifi reset and $wifi connect to retry     |\n"
                          "| Then use $system restart to try again          |\n"
                          "+------------------------------------------------+\n");
    }

    serial_port.print("+------------------------------------------------+\n"
                      "|                    LED Setup                   |\n"
                      "+------------------------------------------------+\n");
    led_strip_set_length(String(memory.read_uint16 ("led_strip_length")));
    led_strip_set_state(String(memory.read_uint8 ("led_strip_state")));
    led_strip_set_mode(String(memory.read_uint8("led_strip_mode")));
    led_strip_set_rgb(String(memory.read_uint8 ("led_strip_r")) + " " + String(memory.read_uint8 ("led_strip_g")) + " " + String(memory.read_uint8 ("led_strip_b")));
    led_strip_set_brightness(String(memory.read_uint8 ("led_strip_brightness")));

    // define_commands() was in your original SystemController.cpp structure,
    // likely after all initial setup. Ensure it's called appropriately.
    // If it depends on WebServer/Alexa being up, its placement might matter,
    // but typically CLI commands can be defined regardless.
    // For this constructor, assuming it's called after this block or elsewhere.
    // Based on your provided snippets, define_commands() was part of the constructor in the first C++ code block you showed.
    // Placing it here if it doesn't depend on the conditional WiFi setup:
    define_commands();
    serial_port.print("+------------------------------------------------+\n"
                      "|             Command Line Interface             |\n"
                      "+------------------------------------------------+\n"
                      "|     Use $help to see all available commands    |\n"
                      "+------------------------------------------------+\n");

} // End of SystemController constructor

// --- update() ---
void SystemController::update() {
    if (serial_port.has_line()) {
        String line = serial_port.read_line();
        serial_port.println(line);
        command_parser.parse_and_execute(line);
    }

    if (wifi.is_connected()) {
        web_interface_module_.update();    // For your webpage server
        alexa_module_.loop();   // For Espalexa
    }

    led_strip.frame();
}

// --- define_commands ---
// No changes needed in define_commands itself
void SystemController::define_commands() {
    // ... (your existing command definitions) ...
    // populate Wi-Fi commands
    help_commands[0] = { "",                    "Print all cmd available",                  0, [this](auto&){ print_help(); } };

    system_commands[0] = { "help",              "Show this help message",                   0, [this](auto&){ system_print_help(); } };
    system_commands[1] = { "reset",             "Reset everything in EEPROM",               0, [this](auto&){ system_reset(); } };
    system_commands[2] = { "restart",           "Restart system",                           0, [this](auto&){ system_restart(); } };

    wifi_commands[0] = { "help",                "Show this help message",                0, [this](auto&){ wifi_print_help(); } };
    wifi_commands[1] = { "connect",             "Connect or reconnect to WiFi",          0, [this](auto&){ wifi_connect(true); } };
    wifi_commands[2] = { "disconnect",          "Disconnect from WiFi",                  0, [this](auto&){ wifi_disconnect(); } };
    wifi_commands[3] = { "reset",   "Clear saved WiFi credentials",          0, [this](auto&){ wifi_reset(); } };
    wifi_commands[4] = { "status",              "Show connection status, SSID, IP, MAC", 0, [this](auto&){ wifi_print_credentials(); } };
    wifi_commands[5] = { "scan",                "List available WiFi networks",          0, [this](auto&){ wifi_get_available_networks(); } };

    // populate LED-strip commands
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

    // ram
    ram_commands[0] = { "help",     "Show this help message",                           0, [this](auto&){ ram_print_help(); } };
    ram_commands[1] = { "status",   "Show overall heap stats (total/free/high-water)",  0, [this](auto&){ ram_status(); } };
    ram_commands[2] = { "free",     "Print current free heap bytes",                    0, [this](auto&){ ram_free(); } };
    ram_commands[3] = { "watch",    "Continuously print free heap every <ms>",          1, [this](auto& a){ ram_watch(a); } };

    // storage
    storage_commands[0] = { "help",                     "Show this help message",               0, [this](auto&){ storage_print_help(); } };
    storage_commands[1] = { "set_first_startup_flag",   "Set first statup flag to true",        0, [this](auto&){ storage_set_first_startup_flag(); } };

    // populate groups
    command_groups[0] = { "help",       help_commands,          HELP_CMD_COUNT      };
    command_groups[1] = { "system",     system_commands,        SYSTEM_CMD_COUNT    };
    command_groups[2] = { "wifi",       wifi_commands,          WIFI_CMD_COUNT      };
    command_groups[3] = { "led",        led_strip_commands,     LED_STRIP_CMD_COUNT };
    command_groups[4] = { "ram",        ram_commands,           RAM_CMD_COUNT       };
    command_groups[5] = { "storage",    storage_commands,       STORAGE_CMD_COUNT   };

    // register
    command_parser.set_groups(command_groups, CMD_GROUP_COUNT);
}


// --- HELP ---
// No changes
void SystemController::print_help(){
    // ... (your existing code) ...
    system_print_help();
    wifi_print_help();
    led_strip_print_help();
    ram_print_help();
    storage_print_help();
}

// --- SYSTEM ---
// No changes
void SystemController::system_print_help(){
    // ... (your existing code) ...
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
// system_reset calls led_strip_reset, wifi_reset, wifi_connect which will handle Alexa sync

void SystemController::system_reset(){
    memory.reset();
    led_strip_reset();
    wifi_reset();
    wifi_connect(true);
}

void SystemController::system_restart(){
    serial_port.print("+------------------------------------------------+\n"
                      "|                 Restarting...                  |\n"
                      "+------------------------------------------------+\n");
    ESP.restart();
}


// --- LED handlers ---
void SystemController::led_strip_print_help() {
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

void SystemController::led_strip_reset(){
    led_strip_set_length("10"); // Does not directly affect Alexa state for on/off/bri/color
    led_strip_set_state("1");
    led_strip_set_mode("0");
    led_strip_set_rgb("0 10 0");
    led_strip_set_brightness("100");


    serial_port.println("LED Strip Config:");
    serial_port.println("LED strip data pin: GPIO" + String(2)); // Assuming LED_PIN is 2 from context
    serial_port.println("    Pin can only be changed in the sketch, before uploading");
    serial_port.println("    Change #define LED_PIN <your_pin> if needed");

    while (true) {
        serial_port.println("How many LEDs do you have connected?\nEnter a number: ");
        int choice = serial_port.get_int();

        if (choice < 0) {
            serial_port.println("LED number must be greater than 0");
        } else if (choice > 1000) {
            serial_port.println("That's too many. Max supported LED: " + String(1000));
        } else if (choice <= 1000){
            led_strip.set_length(choice);
            memory.write_uint16("led_strip_length", choice);
            break;
        }
    }
    serial_port.println("LED Reset Success!");
    led_strip.frame();
}

void SystemController::led_strip_set_mode(const String& args) {
    uint8_t mode = static_cast<uint8_t>(args.toInt());
    led_strip.set_mode(mode);
    memory.write_uint8("led_strip_mode", mode);
    web_interface_module_.broadcast_led_state("mode");
    if(wifi.is_connected()) alexa_module_.sync_state_with_system_controller("mode"); // Mode might change color/brightness
}

void SystemController::led_strip_set_rgb(const String& args) {
    int i1 = args.indexOf(' '), i2 = args.indexOf(' ', i1 + 1);
    uint8_t r = args.substring(0, i1).toInt();
    uint8_t g = args.substring(i1 + 1, i2).toInt();
    uint8_t b = args.substring(i2 + 1).toInt();
    led_strip.set_rgb(r, g, b);
    memory.write_uint8("led_strip_r", led_strip.get_r());
    memory.write_uint8("led_strip_g", led_strip.get_g());
    memory.write_uint8("led_strip_b", led_strip.get_b());
    web_interface_module_.broadcast_led_state("color");
    if(wifi.is_connected())
        alexa_module_.sync_state_with_system_controller("color");
}

void SystemController::led_strip_set_r(const String& args) {
    led_strip.set_r(static_cast<uint8_t>(args.toInt()));
    memory.write_uint8("led_strip_r", led_strip.get_r());
    web_interface_module_.broadcast_led_state("color");
    if(wifi.is_connected()) alexa_module_.sync_state_with_system_controller("color");
}

void SystemController::led_strip_set_g(const String& args) {
    led_strip.set_g(static_cast<uint8_t>(args.toInt()));
    memory.write_uint8("led_strip_g", led_strip.get_g());
    web_interface_module_.broadcast_led_state("color");
    if(wifi.is_connected()) alexa_module_.sync_state_with_system_controller("color");
}

void SystemController::led_strip_set_b(const String& args) {
    led_strip.set_b(static_cast<uint8_t>(args.toInt()));
    memory.write_uint8("led_strip_b", led_strip.get_b());
    web_interface_module_.broadcast_led_state("color");
    if(wifi.is_connected()) alexa_module_.sync_state_with_system_controller("color");
}

void SystemController::led_strip_set_hsv(const String& args) {
    int i1 = args.indexOf(' '), i2 = args.indexOf(' ', i1 + 1);
    uint8_t h = args.substring(0, i1).toInt();
    uint8_t s = args.substring(i1 + 1, i2).toInt();
    uint8_t v = args.substring(i2 + 1).toInt();
    led_strip.set_hsv(h, s, v);
    memory.write_uint8("led_strip_r", led_strip.get_r());
    memory.write_uint8("led_strip_g", led_strip.get_g());
    memory.write_uint8("led_strip_b", led_strip.get_b());
    web_interface_module_.broadcast_led_state("color");
    if(wifi.is_connected()) alexa_module_.sync_state_with_system_controller("color");
}

void SystemController::led_strip_set_hue(const String& args) {
    led_strip.set_hue(static_cast<uint8_t>(args.toInt()));
    memory.write_uint8("led_strip_r", led_strip.get_r());
    memory.write_uint8("led_strip_g", led_strip.get_g());
    memory.write_uint8("led_strip_b", led_strip.get_b());
    web_interface_module_.broadcast_led_state("color");
    if(wifi.is_connected()) alexa_module_.sync_state_with_system_controller("color");
}

void SystemController::led_strip_set_sat(const String& args) {
    led_strip.set_sat(static_cast<uint8_t>(args.toInt()));
    memory.write_uint8("led_strip_r", led_strip.get_r());
    memory.write_uint8("led_strip_g", led_strip.get_g());
    memory.write_uint8("led_strip_b", led_strip.get_b());
    web_interface_module_.broadcast_led_state("color");
    if(wifi.is_connected()) alexa_module_.sync_state_with_system_controller("color");
}

void SystemController::led_strip_set_val(const String& args) {
    led_strip.set_val(static_cast<uint8_t>(args.toInt()));
    memory.write_uint8("led_strip_r", led_strip.get_r());
    memory.write_uint8("led_strip_g", led_strip.get_g());
    memory.write_uint8("led_strip_b", led_strip.get_b());
    web_interface_module_.broadcast_led_state("color");      // Sync both to be safe if V changes RGB
    if(wifi.is_connected()) alexa_module_.sync_state_with_system_controller("color");
}

void SystemController::led_strip_set_brightness(const String& args) {
    led_strip.set_brightness(static_cast<uint8_t>(args.toInt()));
    memory.write_uint8("led_strip_brightness", static_cast<uint8_t>(args.toInt()));
    web_interface_module_.broadcast_led_state("brightness");
    if(wifi.is_connected()) alexa_module_.sync_state_with_system_controller("brightness");
}

void SystemController::led_strip_set_state(const String& args) {
    led_strip.set_state(static_cast<byte>(args.toInt())); // Note: byte is uint8_t
    memory.write_uint8("led_strip_state", static_cast<uint8_t>(args.toInt()));
    web_interface_module_.broadcast_led_state("state");
    if(wifi.is_connected()) alexa_module_.sync_state_with_system_controller("state");
}

void SystemController::led_strip_turn_on() {
    led_strip.turn_on();
    memory.write_uint8("led_strip_state", 1);
    web_interface_module_.broadcast_led_state("state");
    if(wifi.is_connected()) alexa_module_.sync_state_with_system_controller("state");
}

void SystemController::led_strip_turn_off() {
    led_strip.turn_off();
    memory.write_uint8("led_strip_state", 0);
    web_interface_module_.broadcast_led_state("state");
    if(wifi.is_connected()) alexa_module_.sync_state_with_system_controller("state");
}

void SystemController::led_strip_set_length(const String& args){
    uint16_t length = static_cast<uint16_t>(args.toInt());
    led_strip.set_length(length);
    memory.write_uint16("led_strip_length", length);
}

String SystemController::led_strip_get_color_hex() const {
    DBG_PRINTLN(SystemController, "String SystemController::led_strip_get_color_hex() const {");
    std::array<uint8_t, 3> target_rgb = led_strip.get_target_rgb();
    char buf[8];
    sprintf(buf, "#%02X%02X%02X", target_rgb[0], target_rgb[1], target_rgb[2]);
    return String(buf);
}

uint8_t SystemController::led_strip_get_brightness() const {
    DBG_PRINTLN(SystemController, "uint8_t SystemController::led_strip_get_brightness() const {");
    return led_strip.get_brightness();
}

bool SystemController::led_strip_get_state() const {
    DBG_PRINTLN(SystemController, "bool SystemController::led_strip_get_state() const {");
    return led_strip.get_state();
}

std::array<uint8_t, 3> SystemController::led_strip_get_target_rgb() const {
    DBG_PRINTLN(SystemController, "led_strip_get_target_rgb() const {");
    return led_strip.get_target_rgb();
}




uint8_t SystemController::led_strip_get_mode_id() const {
    DBG_PRINTLN(SystemController, "String SystemController::led_strip_get_mode() const {");
    // Assuming led_strip.get_mode_id() exists or is what you meant
    // return led_strip.get_mode_id(); // If you have this method in LedStrip
    return memory.read_uint8("led_strip_mode"); // Or read from memory if that's the source of truth for the ID
}


//////WIFI/////
// ——— wifi_connect ———
bool SystemController::wifi_connect(bool prompt_for_credentials) {
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
    if (!wifi.is_connected()) {
        serial_port.println("WiFi not connected");
        return;
    }
    serial_port.println("Connected to " + wifi.get_ssid());
    serial_port.println("Local IP: " + wifi.get_local_ip());
    serial_port.println("MAC: " + wifi.get_mac_address());
}

bool SystemController::wifi_read_stored_credentials(String& ssid, String& pwd) {
    if (!memory.read_bit("wifi_flags", 0)) {
        return false;
    }
    ssid = memory.read_str("wifi_name");
    pwd  = memory.read_str("wifi_pass");
    return true;
}

uint8_t SystemController::wifi_prompt_for_credentials(String& ssid, String& pwd) {
    memory.write_bit("wifi_flags", 0, 0);

    std::vector<String> networks = wifi_get_available_networks();
    serial_port.println("Select network by number, or enter -1 to exit:");
    int choice = serial_port.get_int();

    if (choice == -1) {
        return 2;
    } else if (choice == 0) {
        bool confirmed = false;
        while (!confirmed) {
            serial_port.println("Enter SSID:");
            ssid = serial_port.get_string();
            serial_port.println("Confirm SSID: " + ssid);
            confirmed = serial_port.get_confirmation();
        }
    } else if (choice > 0 && choice <= (int)networks.size()) {
        ssid = networks[choice - 1];
    } else {
        return 1;
    }

    serial_port.println("Enter password for network '" + ssid + "':");
    pwd = serial_port.get_string();

    return 0;
}

bool SystemController::wifi_join(const String& ssid, const String& pwd) {
    serial_port.println("Connecting to '" + ssid + "'...");

    if (wifi.connect(ssid, pwd)) {
        wifi_print_credentials();
        memory.write_bit("wifi_flags", 0, 1);
        memory.write_str("wifi_name", ssid);
        memory.write_str("wifi_pass", pwd);
        return true;
    }

    serial_port.println("Failed to connect to '" + ssid + "'");
    return false;
}

bool SystemController::wifi_disconnect() {
    if (!wifi.is_connected()) {
        serial_port.println("Not currently connected to WiFi");
        return true;
    }

    String current_ssid = wifi.get_ssid();
    wifi.disconnect();
    serial_port.println("Disconnected from '" + current_ssid + "'");
    return true;
}

bool SystemController::wifi_reset() {
    memory.write_bit("wifi_flags", 0, 0);
    memory.write_str("wifi_name", "");
    memory.write_str("wifi_pass", "");

    if (wifi.is_connected()) {
        wifi.disconnect();
    }

    serial_port.println("WiFi credentials have been reset. Use '$wifi connect' to select a new network");
    return true;
}

std::vector<String> SystemController::wifi_get_available_networks() {
    serial_port.println("Scanning available networks...");
    std::vector<String> networks = wifi.get_available_networks();
    serial_port.println("Available networks:\n0: Enter custom SSID");
    for (size_t i = 0; i < networks.size(); ++i) {
        serial_port.println(String(i + 1) + ": " + networks[i]);
    }
    return networks;
}

void SystemController::wifi_print_help() {
    serial_port.println("WiFi commands:");
    for (size_t i = 0; i < WIFI_CMD_COUNT; ++i) {
        const auto &cmd = wifi_commands[i];
        serial_port.println("  $wifi " + String(cmd.name) + " - " + String(cmd.description) + ", argument count: " + String(cmd.arg_count));
    }
}


/////ram
void SystemController::ram_print_help() {
    serial_port.println("RAM commands:");
    for (size_t i = 0; i < RAM_CMD_COUNT; ++i) {
        const auto &cmd = ram_commands[i];
        serial_port.println("  $ram " + String(cmd.name) + " - " + String(cmd.description) + ", argument count: " + String(cmd.arg_count));
    }
}

void SystemController::ram_status() {
    serial_port.print_spacer();

    // gather heap metrics
    uint32_t total       = ESP.getHeapSize();
    uint32_t free_bytes  = ESP.getFreeHeap();
    uint32_t min_free    = ESP.getMinFreeHeap();
    uint32_t max_alloc   = ESP.getMaxAllocHeap();

    // calculate usage
    uint32_t used        = total - free_bytes;
    float    pct_used    = used * 100.0f / total;
    float    pct_free    = free_bytes * 100.0f / total;

    // print raw numbers
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

    // print usage bar
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

    // print free bar
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
    serial_port.print("Free heap: ");
    serial_port.println(String(ESP.getFreeHeap()));
}

void SystemController::ram_watch(const String& args) {
    uint32_t interval = (uint32_t)args.toInt();
    serial_port.println("Entering RAM watch. Press any key to exit.");
    while (true) {
        serial_port.print("Free heap: ");
        serial_port.println(String(ESP.getFreeHeap()));
        delay(interval);
        if (serial_port.has_line()) {
            serial_port.read_line();  // consume input
            serial_port.println("RAM watch cancelled.");
            break;
        }
    }
}



/////storage/////
void SystemController::storage_print_help() {
    serial_port.println("Storage commands:");
    for (size_t i = 0; i < STORAGE_CMD_COUNT; ++i) {
        const auto &cmd = storage_commands[i];
        serial_port.println("  $storage " + String(cmd.name) + " - " + String(cmd.description) + ", argument count: " + String(cmd.arg_count));
    }
}

void SystemController::storage_set_first_startup_flag() {
    storage.set_first_startup_flag();
}