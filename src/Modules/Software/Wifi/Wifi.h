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

    // String-based APIs
    bool connect(bool prompt_for_credentials);
    bool disconnect();
    bool join(std::string_view ssid, std::string_view password);
    bool is_connected() const;

    std::string get_local_ip() const;
    std::string get_ssid() const;
    std::string get_mac_address() const;
private:
    std::vector<std::string> get_available_networks(bool print_result);
    std::string hostname;
    bool read_stored_credentials(std::string ssid, std::string password);
    uint8_t prompt_credentials(std::string, std::string);
};

#endif // WIFI_H
