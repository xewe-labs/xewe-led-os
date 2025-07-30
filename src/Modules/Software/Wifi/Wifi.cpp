#include "Wifi.h"

Wifi::Wifi(const char* hostname) {
    // Operate as station and set hostname
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(hostname);

    // Clear any previously stored credentials
    WiFi.disconnect(true);
    delay(100);
}

bool Wifi::connect(const String& ssid, const String& password) {
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long start_time = millis();
    constexpr unsigned long timeout_ms = 10'000;  // 10 seconds

    // Wait for connection or timeout
    while (millis() - start_time < timeout_ms) {
        if (WiFi.status() == WL_CONNECTED) {
            return true;
        }
        delay(200);
    }
    WiFi.disconnect(true);
    return false;
}

bool Wifi::disconnect() {
    WiFi.disconnect();

    unsigned long start_time = millis();
    constexpr unsigned long timeout_ms = 5'000;  // 5 seconds

    // Wait until fully disconnected or timeout
    while (WiFi.status() != WL_DISCONNECTED && millis() - start_time < timeout_ms) {
        delay(100);
    }
    return (WiFi.status() == WL_DISCONNECTED);
}

std::vector<String> Wifi::get_available_networks() {
    int num_networks = WiFi.scanNetworks(/* async */ true, /* show_hidden */ true);
    while (num_networks == WIFI_SCAN_RUNNING) {
        delay(10);
        num_networks = WiFi.scanComplete();
    }

    std::vector<String> unique_ssid_list;

    if (num_networks > 0) {
        std::set<String> seen_ssids;

        unique_ssid_list.reserve(num_networks);

        for (int i = 0; i < num_networks; ++i) {
            String current_ssid = WiFi.SSID(i);
            if (!current_ssid.isEmpty() && seen_ssids.insert(current_ssid).second) {
                unique_ssid_list.push_back(current_ssid);
            }
        }
    }

    WiFi.scanDelete();

    return unique_ssid_list;
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
    char buf[18];
    sprintf(buf,
            "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2],
            mac[3], mac[4], mac[5]);
    return String(buf);
}