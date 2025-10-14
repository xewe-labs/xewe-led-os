// src/Modules/Software/Wifi/Wifi.h
#pragma once

#include "../../Module/Module.h"
#include "../../../Config.h"
#include "../../../Debug.h"

#include <WiFi.h>


struct WifiConfig : public ModuleConfig {};


class Wifi : public Module {
public:
    explicit                    Wifi              (SystemController& controller);

    // optional implementation
    void                begin_routines_required     (const ModuleConfig& cfg)       override;
    void                begin_routines_init         (const ModuleConfig& cfg)       override;
    void                begin_routines_regular      (const ModuleConfig& cfg)       override;
//    void                begin_routines_common       (const ModuleConfig& cfg)       override;
//
    void                loop                        ()                              override;
//
    void                reset                       (const bool verbose=false)      override;
//
    void                enable                      (const bool verbose=false)      override;
    void                disable                     (const bool verbose=false)      override;
//
    std::string         status                      (const bool verbose=false)      const override;
//    bool                is_enabled                  (const bool verbose=false)      const override;
//    bool                is_disabled                 (const bool verbose=false)      const override;
//    bool                init_setup_complete         (const bool verbose=false)      const override;

    // other methods
    bool                        connect                     (bool prompt_for_credentials);
    bool                        disconnect                  (bool verbose=false);
    bool                        is_connected                (bool verbose=false) const;
    bool                        is_disconnected             (bool verbose=false) const;

    std::string                 get_local_ip                () const;
    std::string                 get_ssid                    () const;
    std::string                 get_mac_address             () const;
private:
    std::vector<std::string>    scan                        (bool verbose);

    bool                        join                        (std::string_view ssid,
                                                             std::string_view password,
                                                             uint16_t timeout_ms=10000,
                                                             uint8_t retry_count=1);
    bool                        read_stored_credentials     (std::string& ssid,
                                                             std::string& password);
    uint8_t                     prompt_credentials          (std::string& ssid,
                                                             std::string& password);
};
