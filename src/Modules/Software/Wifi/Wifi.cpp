// src/Modules/Software/Wifi/Wifi.cpp

#include "Wifi.h"
#include "../../../SystemController.h"

Wifi::Wifi(SystemController& controller)
  : Module(controller, "wifi", "wifi", true, true)
{
    DBG_PRINTLN(Wifi, "Constructor: registering commands");
    // Custom WiFi commands (generic commands are added by Module ctor)
    commands_storage.push_back({
        "connect",
        "Connect or reconnect to WiFi",
        "",
        0,
        [this](std::string_view){ connect(true); }
    });
    commands_storage.push_back({
        "disconnect",
        "Disconnect from WiFi",
        "",
        0,
        [this](std::string_view){ disconnect(); }
    });
    commands_storage.push_back({
        "scan",
        "List available WiFi networks",
        "",
        0,
        [this](std::string_view){ scan(true); }
    });
    DBG_PRINTLN(Wifi, "Constructor: done");
}

void Wifi::begin(const ModuleConfig& cfg_base) {
    const auto& cfg = static_cast<const WifiConfig&>(cfg_base);
    hostname = cfg.hostname;
    DBG_PRINTLN(Wifi, (std::string("begin(): hostname = ") + hostname).c_str());
    WiFi.mode(WIFI_STA);
    DBG_PRINTLN(Wifi, "begin(): WiFi.mode(WIFI_STA)");
    WiFi.setHostname(hostname.c_str());
    DBG_PRINTLN(Wifi, "begin(): WiFi.setHostname()");
    WiFi.disconnect(true);
    DBG_PRINTLN(Wifi, "begin(): WiFi.disconnect(true)");
    delay(100);
    DBG_PRINTLN(Wifi, "begin(): completed");
}

void Wifi::loop() {
    DBG_PRINTLN(Wifi, "loop()");
}

void Wifi::enable() {
    DBG_PRINTLN(Wifi, "enable()");
    if (can_be_disabled) {
        enabled = true;
        DBG_PRINTLN(Wifi, "enable(): enabled = true");
    }
}

void Wifi::disable() {
    DBG_PRINTLN(Wifi, "disable()");
    if (can_be_disabled) {
        if (is_connected()) {
            DBG_PRINTLN(Wifi, "disable(): disconnecting");
            disconnect();
        }
        enabled = false;
        DBG_PRINTLN(Wifi, "disable(): enabled = false");
    }
}

void Wifi::reset() {
    DBG_PRINTLN(Wifi, "reset(): disconnecting and delaying");
    disconnect();
    delay(100);
    DBG_PRINTLN(Wifi, "reset(): done");
}

std::string_view Wifi::status() const {
    bool conn = is_connected();
    DBG_PRINTF(Wifi, "status(): %s\n", conn ? "connected" : "disconnected");
    return conn ? "connected" : "disconnected";
}

bool Wifi::connect(bool prompt_for_credentials) {
    DBG_PRINTF(Wifi, "connect(prompt_for_credentials=%d)\n", prompt_for_credentials);
    if (!enabled) {
        DBG_PRINTLN(Wifi, "connect(): module disabled");
        controller.serial_port.println("WiFi Module disabled\n Use $wifi enable");
        return false;
    }
    if (is_connected()) {
        DBG_PRINTLN(Wifi, "connect(): already connected");
        controller.serial_port.println("Already connected");
        controller.serial_port.println(status().data());
        controller.serial_port.println("Use '$wifi reset' to change network");
        return true;
    }

    std::string ssid, pwd;
    if (read_stored_credentials(ssid, pwd)) {
        DBG_PRINTLN(Wifi, "connect(): stored credentials found");
        controller.serial_port.println("Stored WiFi credentials found");
        if (join(ssid, pwd)) {
            DBG_PRINTLN(Wifi, "connect(): join() succeeded with stored credentials");
            return true;
        } else {
            DBG_PRINTLN(Wifi, "connect(): join() failed with stored credentials");
            controller.serial_port.println("Stored WiFi credentials not valid.");
            if (!prompt_for_credentials) {
                controller.serial_port.println("Use '$wifi reset' to reset credentials");
            }
        }
    } else {
        DBG_PRINTLN(Wifi, "connect(): no stored credentials");
        controller.serial_port.println("Stored WiFi credentials not found");
        if (!prompt_for_credentials) {
            controller.serial_port.println("Type '$wifi connect' to select a new network");
        }
    }
    if (!prompt_for_credentials) {
        DBG_PRINTLN(Wifi, "connect(): prompt_for_credentials=false, exiting");
        return false;
    }

    while (!is_connected()) {
        DBG_PRINTLN(Wifi, "connect(): prompting for credentials");
        uint8_t prompt_status = prompt_credentials(ssid, pwd);
        DBG_PRINTF(Wifi, "connect(): prompt_credentials returned %d\n", prompt_status);
        if (prompt_status == 2) {
            DBG_PRINTLN(Wifi, "connect(): user terminated setup");
            controller.serial_port.println("Terminated WiFi setup");
            return false;
        } else if (prompt_status == 1) {
            DBG_PRINTLN(Wifi, "connect(): invalid choice, retrying");
            controller.serial_port.println("Invalid choice");
            continue;
        } else {
            DBG_PRINTLN(Wifi, "connect(): attempting join() with user credentials");
            if (join(ssid, pwd)) {
                DBG_PRINTLN(Wifi, "connect(): join() succeeded with user credentials");
                return true;
            }
        }
    }
    return false;
}

bool Wifi::disconnect() {
    DBG_PRINTLN(Wifi, "disconnect(): start");
    WiFi.disconnect();
    unsigned long start = millis();
    constexpr unsigned long timeout = 5000;
    while (WiFi.status() != WL_DISCONNECTED && millis() - start < timeout) {
        delay(100);
    }
    bool done = (WiFi.status() == WL_DISCONNECTED);
    DBG_PRINTF(Wifi, "disconnect(): %s\n", done ? "success" : "timeout/failure");
    return done;
}

bool Wifi::join(std::string_view ssid, std::string_view password) {
    DBG_PRINTF(Wifi, "join(): ssid='%.*s'\n", int(ssid.size()), ssid.data());
    WiFi.begin(ssid.data(), password.data());
    unsigned long start = millis();
    constexpr unsigned long timeout = 10000;
    while (millis() - start < timeout) {
        if (WiFi.status() == WL_CONNECTED) {
            DBG_PRINTLN(Wifi, "join(): connected");
            return true;
        }
        delay(200);
    }
    WiFi.disconnect(true);
    DBG_PRINTLN(Wifi, "join(): timeout, disconnected");
    return false;
}

std::vector<std::string> Wifi::scan(bool print_result) {
    DBG_PRINTLN(Wifi, "scan(): starting scan");
    controller.serial_port.println("Scanning WiFi networks...");
    int num_networks = WiFi.scanNetworks(true, true);
    while (num_networks == WIFI_SCAN_RUNNING) {
        delay(10);
        num_networks = WiFi.scanComplete();
    }
    DBG_PRINTF(Wifi, "scan(): scan complete, %d networks found\n", num_networks);

    std::vector<std::string> unique_ssid_list;
    if (num_networks > 0) {
        std::set<std::string> seen_ssids;
        unique_ssid_list.reserve(num_networks);

        for (int i = 0; i < num_networks; ++i) {
            String cur = WiFi.SSID(i);
            if (cur.isEmpty()) continue;
            std::string ssid(cur.c_str());
            if (seen_ssids.insert(ssid).second) {
                unique_ssid_list.push_back(ssid);
                DBG_PRINTF(Wifi, "scan(): adding [%s]\n", ssid.c_str());
            }
        }
    }

    if (print_result) {
        for (size_t j = 0; j < unique_ssid_list.size(); ++j) {
            char line[64];
            snprintf(line, sizeof(line), "%zu. %s", j, unique_ssid_list[j].c_str());
            controller.serial_port.println(line);
        }
    }

    WiFi.scanDelete();
    DBG_PRINTLN(Wifi, "scan(): done");
    return unique_ssid_list;
}

bool Wifi::is_connected() const {
    bool conn = (WiFi.status() == WL_CONNECTED);
    DBG_PRINTF(Wifi, "is_connected(): %s\n", conn ? "true" : "false");
    return conn;
}

std::string Wifi::get_local_ip() const {
    auto ip = WiFi.localIP();
    char buf[16];
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
             ip[0], ip[1], ip[2], ip[3]);
    DBG_PRINTF(Wifi, "get_local_ip(): %s\n", buf);
    return std::string(buf);
}

std::string Wifi::get_ssid() const {
    const char* s = WiFi.SSID().c_str();
    std::string result = s ? std::string(s) : std::string{};
    DBG_PRINTF(Wifi, "get_ssid(): '%s'\n", result.c_str());
    return result;
}

std::string Wifi::get_mac_address() const {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char buf[18];
    snprintf(buf, sizeof(buf),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2],
             mac[3], mac[4], mac[5]);
    DBG_PRINTF(Wifi, "get_mac_address(): %s\n", buf);
    return std::string(buf);
}

bool Wifi::read_stored_credentials(std::string ssid, std::string password) {
    DBG_PRINTLN(Wifi, "read_stored_credentials(): reading NVS");
    ssid = std::move(controller.nvs.read_str(nvs_key, "ssid"));
    password = std::move(controller.nvs.read_str(nvs_key, "ssid"));
    bool has = ssid.length() > 0;
    DBG_PRINTF(Wifi, "read_stored_credentials(): %s\n", has ? "found" : "none");
    return has;
}

uint8_t Wifi::prompt_credentials(std::string ssid, std::string password) {
    DBG_PRINTLN(Wifi, "prompt_credentials");
    if (!enabled) {
        DBG_PRINTLN(Wifi, "prompt_credentials(): module disabled");
        controller.serial_port.println("WiFi Module disabled\n Use $wifi enable");
        return 2;
    }

    std::vector<std::string> networks = scan(true);
    int choice = controller.serial_port.get_int("\nSelect network by number, or enter -1 to exit: ");
    DBG_PRINTF(Wifi, "prompt_credentials(): user choice = %d\n", choice);
    if (choice == -1) {
        DBG_PRINTLN(Wifi, "prompt_credentials(): user exit");
        return 2;
    } else if (choice >= 0 && choice < (int)networks.size()) {
        ssid = networks[choice];
        DBG_PRINTF(Wifi, "prompt_credentials(): selected ssid = %s\n", ssid.c_str());
    } else {
        DBG_PRINTLN(Wifi, "prompt_credentials(): invalid choice");
        return 1;
    }
    password = controller.serial_port.get_string("Selected: '" + ssid + "'\nPassword: ");
    DBG_PRINTLN(Wifi, "prompt_credentials(): password entered");
    return 0;
}
