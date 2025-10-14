// src/Modules/Software/Wifi/Wifi.h

#ifndef WIFI_H
#define WIFI_H

#include "../../Module.h"
#include "../../../Debug.h"

#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include <string>
#include <string_view>
#include <set>

struct WifiConfig : public ModuleConfig {
    std::string hostname = "ESP32-C3-Device";
};

class Wifi : public Module {
public:
    explicit Wifi(SystemController& controller);

    void begin(const ModuleConfig& cfg) override;
    void loop() override;
    void enable() override;
    void disable() override;
    void reset() override;
    std::string_view status() const override;

    // Core WiFi operations
    bool connect(bool prompt_for_credentials = true);
    bool disconnect();
    bool join(const std::string& ssid, const std::string& password);
    bool is_connected() const;

    // Info getters
    std::string get_local_ip() const;
    std::string get_ssid() const;
    std::string get_mac_address() const;

private:
    // Scan networks; if print_results==true, prints them to serial
    std::vector<std::string> get_available_networks(bool print_results = false);

    // Read/store NVS credentials
    bool read_stored_credentials(std::string& ssid, std::string& password);

    // Prompt user over serial to pick SSID & enter password
    uint8_t prompt_for_credentials(std::string& ssid, std::string& password);

    std::string   hostname;
};

#endif // WIFI_H
