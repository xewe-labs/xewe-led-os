// src/Modules/Software/Wifi/Wifi.h
#ifndef WIFI_H
#define WIFI_H

#include "../../Module.h"
#include "../../../Debug.h"

#include <Arduino.h>        // for Serial
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
    std::vector<std::string> get_available_networks();
    bool connect(std::string_view ssid, std::string_view password);
    bool disconnect();
    bool is_connected() const;
    std::string get_local_ip() const;
    std::string get_ssid() const;
    std::string get_mac_address() const;

    // CLI command handlers
    void wifi_reset(bool verbose);
    void wifi_status();
    void wifi_enable(bool silent, bool verbose);
    void wifi_disable(bool silent, bool verbose);
    void wifi_connect(bool verbose);
    void wifi_disconnect();
    void wifi_get_available_networks();

    const CommandsGroup& get_command_group() const { return commands_group; }

private:
    std::string hostname;
    Command     wifi_commands[8];
};

#endif // WIFI_H
