// src/Modules/Software/Wifi/Wifi.cpp
#include "Wifi.h"
#include "../CommandParser/CommandParser.h"

Wifi::Wifi(SystemController& controller)
  : Module(controller,
           /* module_name     */ "wifi",
           /* nvs_key         */ "wifi",
           /* can_be_disabled */ true),
    wifi_commands{
        Command{ "help",       "Show this help message",       "", 0, [this](std::string_view) { Serial.println("Wifi Help"); } },
        Command{ "reset",      "Reset web interface",          "", 0, [this](std::string_view) { wifi_reset(true); } },
        Command{ "status",     "Get web interface status",     "", 0, [this](std::string_view) { wifi_status(); } },
        Command{ "enable",     "Enable web interface",         "", 0, [this](std::string_view) { wifi_enable(false, true); } },
        Command{ "disable",    "Disable web interface",        "", 0, [this](std::string_view) { wifi_disable(false, true); } },
        Command{ "connect",    "Connect or reconnect to WiFi", "", 0, [this](std::string_view) { wifi_connect(true); } },
        Command{ "disconnect", "Disconnect from WiFi",         "", 0, [this](std::string_view) { wifi_disconnect(); } },
        Command{ "scan",       "List available WiFi networks", "", 0, [this](std::string_view) { wifi_get_available_networks(); } },
    }
{
    commands_group = CommandsGroup{
        "wifi",
        std::span<const Command>(wifi_commands)
    };
}

void Wifi::begin(const ModuleConfig& cfg) {
    const auto& config = static_cast<const WifiConfig&>(cfg);
    hostname = config.hostname;

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(hostname.c_str());
    WiFi.disconnect(true);
    delay(100);
}

void Wifi::loop() {
    // Wifi module doesn't require regular polling.
}

void Wifi::enable() {
    if (can_be_disabled) enabled = true;
}

void Wifi::disable() {
    if (can_be_disabled) {
        disconnect();
        enabled = false;
    }
}

void Wifi::reset() {
    disconnect();
    WiFi.disconnect(true);
    delay(100);
}

std::string_view Wifi::status() const {
    return is_connected() ? "connected" : "disconnected";
}

bool Wifi::connect(const String& ssid, const String& password) {
    WiFi.begin(ssid.c_str(), password.c_str());
    unsigned long start_time = millis();
    constexpr unsigned long timeout_ms = 10000;

    while (millis() - start_time < timeout_ms) {
        if (WiFi.status() == WL_CONNECTED) return true;
        delay(200);
    }
    WiFi.disconnect(true);
    return false;
}

bool Wifi::disconnect() {
    WiFi.disconnect();
    unsigned long start_time = millis();
    constexpr unsigned long timeout_ms = 5000;

    while (WiFi.status() != WL_DISCONNECTED && millis() - start_time < timeout_ms) {
        delay(100);
    }
    return (WiFi.status() == WL_DISCONNECTED);
}

std::vector<String> Wifi::get_available_networks() {
    int network_count = WiFi.scanNetworks(true, true);
    while (network_count == WIFI_SCAN_RUNNING) {
        delay(10);
        network_count = WiFi.scanComplete();
    }

    std::vector<String> unique_ssids;
    if (network_count > 0) {
        std::set<String> seen;
        unique_ssids.reserve(network_count);
        for (int i = 0; i < network_count; ++i) {
            String ssid = WiFi.SSID(i);
            if (!ssid.isEmpty() && seen.insert(ssid).second) {
                unique_ssids.push_back(ssid);
            }
        }
    }
    WiFi.scanDelete();
    return unique_ssids;
}

bool Wifi::is_connected() const {
    return (WiFi.status() == WL_CONNECTED);
}

String Wifi::get_local_ip() const {
    return WiFi.localIP().toString();
}

String Wifi::get_ssid() const {
    return WiFi.SSID();
}

String Wifi::get_mac_address() const {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char buffer[18];
    sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buffer);
}

void Wifi::wifi_reset(bool verbose) {
    reset();
    if (verbose) Serial.println("WiFi interface reset.");
}

void Wifi::wifi_status() {
    Serial.printf("WiFi Status: %s\n", status().data());
}

void Wifi::wifi_enable(bool silent, bool verbose) {
    enable();
    if (verbose && !silent) Serial.println("WiFi interface enabled.");
}

void Wifi::wifi_disable(bool silent, bool verbose) {
    disable();
    if (verbose && !silent) Serial.println("WiFi interface disabled.");
}

void Wifi::wifi_connect(bool verbose) {
    String ssid = "<your-ssid>";
    String password = "<your-password>";
    bool success = connect(ssid, password);
    if (verbose) {
        if (success) Serial.println("Connected to WiFi.");
        else Serial.println("Failed to connect to WiFi.");
    }
}

void Wifi::wifi_disconnect() {
    disconnect();
    Serial.println("Disconnected from WiFi.");
}

void Wifi::wifi_get_available_networks() {
    auto networks = get_available_networks();
    Serial.println("Available Networks:");
    for (const auto& ssid : networks) {
        Serial.print("- ");
        Serial.println(ssid);
    }
}
