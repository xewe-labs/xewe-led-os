// src/Modules/Software/Wifi/Wifi.h
#ifndef WIFI_H
#define WIFI_H

#include "../../Module.h"
#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include <set>

struct WifiConfig : public ModuleConfig {
    const char* hostname = "ESP32-C3-Device";
};

class Wifi : public Module {
public:
    explicit Wifi(SystemController& controller);

    // Module interface
    void begin(const ModuleConfig& cfg) override;
    void loop(const std::string& args) override;
    void enable() override;
    void disable() override;
    void reset() override;
    std::string_view status() const override;

    // Existing methods preserved
    std::vector<String> get_available_networks();
    bool connect(const String& ssid, const String& password);
    bool disconnect();
    bool is_connected() const;
    String get_local_ip() const;
    String get_ssid() const;
    String get_mac_address() const;

    // Methods for commands
    void wifi_reset(bool verbose);
    void wifi_status();
    void wifi_enable(bool silent, bool verbose);
    void wifi_disable(bool silent, bool verbose);
    void wifi_connect(bool verbose);
    void wifi_disconnect();
    void wifi_get_available_networks();

    // Accessor to command group
    const CommandsGroup& get_command_group() const { return commands_group; }

private:
    String hostname_;

    // Commands storage
    Command wifi_commands[8];
};

#endif // WIFI_H
