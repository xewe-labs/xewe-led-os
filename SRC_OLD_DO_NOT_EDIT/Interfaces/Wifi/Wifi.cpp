// src/Modules/Software/Wifi/Wifi.cpp

#include "Wifi.h"
#include "../../../SystemController.h"

constexpr unsigned long CONNECT_TIMEOUT_MS    = 10000;
constexpr unsigned long DISCONNECT_TIMEOUT_MS =  5000;

Wifi::Wifi(SystemController& controller)
  : Module(controller, "wifi", "wifi", true, true)
{
    // custom commands (generic ones already in Module ctor)
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

void Wifi::loop() {
    // No recurring tasks
}

void Wifi::enable() {
    if (can_be_disabled) enabled = true;
}

void Wifi::disable() {
    if (can_be_disabled) {
        if (is_connected()) disconnect();
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
    DBG_PRINTF(SystemController, "wifi_connect(prompt=%d)\n", prompt_for_credentials);
    if (!enabled) {
        controller.serial_port.println("WiFi Module disabled\nUse '$wifi enable'");
        return false;
    }
    if (is_connected()) {
        controller.serial_port.println("Already connected");
        controller.serial_port.println(status());
        controller.serial_port.println("Use '$wifi reset' to switch networks");
        return true;
    }

    std::string ssid, password;
    if (read_stored_credentials(ssid, password)) {
        controller.serial_port.println("Stored credentials found");
        if (join(ssid, password)) {
            controller.serial_port.println("Connected to " + ssid);
            return true;
        }
        controller.serial_port.println("Stored credentials invalid");
        if (!prompt_for_credentials) {
            controller.serial_port.println("Use '$wifi reset' to clear them");
            return false;
        }
    } else {
        controller.serial_port.println("No stored WiFi credentials");
        if (!prompt_for_credentials) {
            controller.serial_port.println("Type '$wifi connect' to choose a network");
            return false;
        }
    }

    // interactive loop
    while (!is_connected()) {
        uint8_t res = prompt_for_credentials(ssid, password);
        if (res == 2) {
            controller.serial_port.println("WiFi setup aborted");
            return false;
        }
        if (res == 1) {
            controller.serial_port.println("Invalid choice");
            continue;
        }
        // res == 0: try to join
        if (join(ssid, password)) {
            controller.serial_port.println("Connected to " + ssid);
            return true;
        }
        controller.serial_port.println("Join failed, try again");
    }
    return false;
}

bool Wifi::disconnect() {
    WiFi.disconnect();
    unsigned long start = millis();
    while (WiFi.status() != WL_DISCONNECTED
           && (millis() - start) < DISCONNECT_TIMEOUT_MS)
    {
        delay(100);
    }
    return WiFi.status() == WL_DISCONNECTED;
}

bool Wifi::join(const std::string& ssid, const std::string& password) {
    WiFi.begin(ssid.c_str(), password.c_str());
    unsigned long start = millis();
    while ((millis() - start) < CONNECT_TIMEOUT_MS) {
        if (WiFi.status() == WL_CONNECTED) {
            return true;
        }
        delay(200);
    }
    WiFi.disconnect(true);
    return false;
}

std::vector<std::string> Wifi::get_available_networks(bool print_results) {
    int count = WiFi.scanNetworks(true, true);
    while (count == WIFI_SCAN_RUNNING) {
        delay(10);
        count = WiFi.scanComplete();
    }

    std::vector<std::string> list;
    std::set<std::string>    seen;
    int index = 0;

    for (int i = 0; i < count; ++i) {
        String ss = WiFi.SSID(i);
        if (ss.isEmpty()) continue;
        std::string s = ss.c_str();
        if (!seen.insert(s).second) continue;

        if (print_results) {
            controller.serial_port.println(String(index) + ". " + ss);
            ++index;
        }
        list.push_back(std::move(s));
    }
    WiFi.scanDelete();
    return list;
}

bool Wifi::is_connected() const {
    return (WiFi.status() == WL_CONNECTED);
}

std::string Wifi::get_local_ip() const {
    auto ip = WiFi.localIP();
    char buf[16];
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
             ip[0], ip[1], ip[2], ip[3]);
    return buf;
}

std::string Wifi::get_ssid() const {
    return WiFi.SSID().c_str();
}

std::string Wifi::get_mac_address() const {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char buf[18];
    snprintf(buf, sizeof(buf),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2],
             mac[3], mac[4], mac[5]);
    return buf;
}

bool Wifi::read_stored_credentials(std::string& ssid, std::string& password) {
    ssid     = controller.nvs.read_str(nvs_namespace, "ssid");
    password = controller.nvs.read_str(nvs_namespace, "password");
    return !ssid.empty() && !password.empty();
}

uint8_t Wifi::prompt_for_credentials(std::string& ssid, std::string& password) {
    if (!enabled) {
        controller.serial_port.println("WiFi Module disabled\nUse '$wifi enable'");
        return 2;
    }

    auto nets = get_available_networks(true);
    int choice = controller.serial_port.get_int(
        "\nSelect network by number, or -1 to cancel: "
    );
    if (choice < 0) {
        return 2;
    }
    if (choice >= static_cast<int>(nets.size())) {
        return 1;
    }

    ssid = nets[choice];
    String pwd_str = controller.serial_port.get_string(
        "Selected: '" + String(ssid.c_str()) + "'\nPassword: "
    );
    password = pwd_str.c_str();

    // save for next time
    controller.nvs.write_str(nvs_namespace, "ssid", ssid);
    controller.nvs.write_str(nvs_namespace, "password", password);

    return 0;
}
