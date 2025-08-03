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
        [this](std::string_view){ wifi_connect(true); }
    });
    commands_storage.push_back({
        "disconnect",
        "Disconnect from WiFi",
        "",
        0,
        [this](std::string_view){ wifi_disconnect(); }
    });
    commands_storage.push_back({
        "scan",
        "List available WiFi networks",
        "",
        0,
        [this](std::string_view){ wifi_get_available_networks(); }
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
    WiFi.disconnect(true);
    delay(100);
}

std::string_view Wifi::status() const {
    return is_connected() ? "connected" : "disconnected";
}

bool Wifi::connect(std::string_view ssid, std::string_view password) {
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
    std::set<std::string> seen;
    for (int i = 0; i < cnt; ++i) {
        const char* s = WiFi.SSID(i).c_str();
        if (!s || !*s) continue;
        if (seen.insert(s).second) nets.emplace_back(s);
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

// CLI handler one‐liners
void Wifi::wifi_reset(bool v) {
    reset();
    if (v) Serial.println("WiFi interface reset.");
}

void Wifi::wifi_status() {
    Serial.printf("WiFi Status: %s\n", status().data());
}

void Wifi::wifi_enable(bool s, bool v) {
    enable();
    if (v && !s) Serial.println("WiFi interface enabled.");
}

void Wifi::wifi_disable(bool s, bool v) {
    disable();
    if (v && !s) Serial.println("WiFi interface disabled.");
}

void Wifi::wifi_connect(bool v) {
    const std::string ssid = "<your-ssid>";
    const std::string pwd  = "<your-password>";
    bool ok = connect(ssid, pwd);
    Serial.println(ok ? "Connected" : "Failed");
}

void Wifi::wifi_disconnect() {
    disconnect();
    Serial.println("Disconnected from WiFi.");
}

void Wifi::wifi_get_available_networks() {
    auto n = get_available_networks();
    Serial.println("Available Networks:");
    for (auto& x : n) {
        Serial.print("- ");
        Serial.println(x.c_str());
    }
}
