// src/Modules/Software/Wifi/Wifi.cpp

#include "Wifi.h"
#include "../../../SystemController.h"

Wifi::Wifi(SystemController& controller)
  : Module(controller, "wifi", "wifi", true, true)
{
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
        [this](std::string_view){ get_available_networks(true); }
    });
}

void Wifi::begin(const ModuleConfig& cfg_base) {
    const auto& cfg = static_cast<const WifiConfig&>(cfg_base);
    hostname = cfg.hostname;
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(hostname.c_str());
    WiFi.disconnect(true);
    delay(100);
}

void Wifi::loop() {}

void Wifi::enable() {
    if (can_be_disabled) enabled = true;
}

void Wifi::disable() {
    if (can_be_disabled) {
        if(is_connected())
            disconnect();
        enabled = false;
    }
}

void Wifi::reset() {
    disconnect();
    delay(100);
}

std::string_view Wifi::status() const {
    return is_connected() ? "connected" : "disconnected";
}


bool Wifi::connect(bool prompt_for_credentials) {
    DBG_PRINTF(SystemController, "wifi_connect(prompt_for_credentials=%d)\n", prompt_for_credentials);
    if (!enabled) {
        controller.serial_port.println("WiFi Module disabled\n Use $wifi enable");
        return false;
    }
    if (is_connected()) {
        controller.serial_port.println("Already connected");
        status();
        controller.serial_port.println("Use '$wifi reset' to change network");
        return true;
    }
    std::string ssid, pwd;
    if (read_stored_credentials(ssid, pwd)) {
        controller.serial_port.println("Stored WiFi credentials found");
        if (join(ssid, pwd)) {
            return true;
        } else {
            controller.serial_port.println("Stored WiFi credentials not valid.");
            if (!prompt_for_credentials) {
                controller.serial_port.println("Use '$wifi reset' to reset credentials");
            }
        }
    } else {
        controller.serial_port.println("Stored WiFi credentials not found");
        if (!prompt_for_credentials) {
            controller.serial_port.println("Type '$wifi connect' to select a new network");
        }
    }
    if (!prompt_for_credentials) {
        return false;
    }
    while (!is_connected()) {
        uint8_t prompt_status = prompt_credentials(ssid, pwd);
        if (prompt_status == 2) {
            controller.serial_port.println("Terminated WiFi setup");
            return false;
        } else if (prompt_status == 1) {
            controller.serial_port.println("Invalid choice");
            continue;
        } else if (prompt_status == 0) {
            if (join(ssid, pwd)) {
                return true;
            } else {
                continue;
            }
        }
    }
    return false;
}

bool Wifi::disconnect() {
    WiFi.disconnect();
    unsigned long start = millis();
    constexpr unsigned long timeout = 5000;
    while (WiFi.status() != WL_DISCONNECTED && millis() - start < timeout) {
        delay(100);
    }
    return (WiFi.status() == WL_DISCONNECTED);
}

bool Wifi::join(std::string_view ssid, std::string_view password) {
    WiFi.begin(ssid.data(), password.data());
    unsigned long start = millis();
    constexpr unsigned long timeout = 10000;
    while (millis() - start < timeout) {
        if (WiFi.status() == WL_CONNECTED) return true;
        delay(200);
    }
    WiFi.disconnect(true);
    return false;
}

std::vector<std::string> Wifi::get_available_networks(bool print_result) {
    int cnt = WiFi.scanNetworks(true, true);
    while (cnt == WIFI_SCAN_RUNNING) {
        delay(10);
        cnt = WiFi.scanComplete();
    }
    std::vector<std::string> nets;
    std::set<std::string> seen;
    int j = 0;
    for (int i = 0; i < cnt; ++i) {
        const char* s = WiFi.SSID(i).c_str();
        if (!s || !*s) continue;
        if (seen.insert(s).second){
            nets.emplace_back(s);
            if (print_result) {
                controller.serial_port.println(j + ". " + s);
                j++;
            }
        }
    }
    WiFi.scanDelete();
    return nets;
}

bool Wifi::is_connected() const {
    return (WiFi.status() == WL_CONNECTED);
}

std::string Wifi::get_local_ip() const {
    auto ip = WiFi.localIP();
    char buf[16];
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
             ip[0], ip[1], ip[2], ip[3]);
    return std::string(buf);
}

std::string Wifi::get_ssid() const {
    const char* s = WiFi.SSID().c_str();
    return s ? std::string(s) : std::string{};
}

std::string Wifi::get_mac_address() const {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char buf[18];
    snprintf(buf, sizeof(buf),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2],
             mac[3], mac[4], mac[5]);
    return std::string(buf);
}

bool Wifi::read_stored_credentials(std::string ssid, std::string password){
    ssid = std::move(controller.nvs.read_str(nvs_key, "ssid"));
    password = std::move(controller.nvs.read_str(nvs_key, "ssid"));
    return ssid.length() > 0;
}

uint8_t Wifi::prompt_credentials(std::string ssid, std::string password) {
    DBG_PRINTLN(SystemController, "prompt_credentials(...)");
    if (!enabled) {
        controller.serial_port.println("WiFi Module disabled\n Use $wifi enable");
        return 2;
    }

    std::vector<String> networks = wifi_get_available_networks();
    unit8_t choice = controller.serial_port.get_int("\nSelect network by number, or enter -1 to exit: ");
    if (choice == -1) {
        return 2;
    } else if (choice >= 0 && choice < (int)networks.size()) {
        ssid = networks[choice];
    } else {
        return 1;
    }
    password = controller.serial_port.get_string("Selected: '" + ssid + "'\nPassword: ");
    return 0;
}