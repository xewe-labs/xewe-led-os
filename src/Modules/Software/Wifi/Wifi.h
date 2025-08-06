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
    explicit                    Wifi                (SystemController& controller);

    void                        begin               (const ModuleConfig& cfg)               override;
    void                        loop                ()                                      override;
    void                        enable              ()                                      override;
    void                        disable             ()                                      override;
    void                        reset               ()                                      override;

    std::string_view            status              (bool verbose=true)             const   override;
//    bool                        is_enabled          (bool verbose=true)             const   override;
//    bool                        is_disabled         (bool verbose=true)             const   override;

    bool                        connect             (bool prompt_for_credentials);
    bool                        disconnect          ();
    bool                        is_connected        (bool verbose = true)           const;
    bool                        is_disconnected     (bool verbose = true)           const;

    std::string                 get_local_ip        ()                              const;
    std::string                 get_ssid            ()                              const;
    std::string                 get_mac_address     ()                              const;

private:
    std::vector<std::string>    scan                        (bool verbose);

    bool                        join                        (std::string_view ssid,
                                                             std::string_view password);
    bool                        read_stored_credentials     (std::string& ssid,
                                                             std::string& password);
    uint8_t                     prompt_credentials          (std::string& ssid,
                                                             std::string& password);
};

#endif // WIFI_H
