// src/Modules/Software/Wifi/Wifi.cpp

#include "Wifi.h"
#include "../../../SystemController.h"

Wifi::Wifi(SystemController& controller)
  : Module(controller,
           "wifi",
           "wifi",
           true),
    wifi_commands{
        { "help",       "Show this help message",       "", 0, [this](std::string_view){ this->controller.module_print_help("wifi"); } },
        { "reset",      "Reset web interface",          "", 0, [this](std::string_view){ wifi_reset(true); } },
        { "status",     "Get web interface status",     "", 0, [this](std::string_view){ wifi_status(); } },
        { "enable",     "Enable web interface",         "", 0, [this](std::string_view){ wifi_enable(false, true); } },
        { "disable",    "Disable web interface",        "", 0, [this](std::string_view){ wifi_disable(false, true); } },
        { "connect",    "Connect or reconnect to WiFi", "", 0, [this](std::string_view){ wifi_connect(true); } },
        { "disconnect", "Disconnect from WiFi",         "", 0, [this](std::string_view){ wifi_disconnect(); } },
        { "scan",       "List available WiFi networks", "", 0, [this](std::string_view){ wifi_get_available_networks(); } },
    }
{
    commands_group = CommandsGroup{
        "wifi",
        std::span<const Command>(wifi_commands)
    };
}

void Wifi::begin(const ModuleConfig& cfg) {
    const auto& c = static_cast<const WifiConfig&>(cfg);
    hostname = c.hostname;

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(hostname);
    WiFi.disconnect(true);
    delay(100);
}

void Wifi::loop() {
    // no regular polling required
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

bool Wifi::connect(const char* ssid, const char* password) {
    WiFi.begin(ssid, password);
    unsigned long start = millis();
    constexpr unsigned long timeout = 10000;
    while (millis() - start < timeout) {
        if (WiFi.status() == WL_CONNECTED) return true;
        delay(200);
    }
    WiFi.disconnect(true);
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

std::vector<std::string> Wifi::get_available_networks() {
    int cnt = WiFi.scanNetworks(true, true);
    while (cnt == WIFI_SCAN_RUNNING) {
        delay(10);
        cnt = WiFi.scanComplete();
    }

    std::vector<std::string> nets;
    std::set<std::string>    seen;
    for (int i = 0; i < cnt; ++i) {
        const char* s = WiFi.SSID(i).c_str();
        if (!s || s[0] == '\0') continue;
        if (seen.insert(s).second) {
            nets.emplace_back(s);
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
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
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
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(buf);
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
    const char* ssid     = "<your-ssid>";
    const char* password = "<your-password>";
    bool ok = connect(ssid, password);
    if (verbose) {
        if (ok) Serial.println("Connected to WiFi.");
        else   Serial.println("Failed to connect to WiFi.");
    }
}

void Wifi::wifi_disconnect() {
    disconnect();
    Serial.println("Disconnected from WiFi.");
}

void Wifi::wifi_get_available_networks() {
    auto nets = get_available_networks();
    Serial.println("Available Networks:");
    for (auto& s : nets) {
        Serial.print("- ");
        Serial.println(s.c_str());
    }
}
