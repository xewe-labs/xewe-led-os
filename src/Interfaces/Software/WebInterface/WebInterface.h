// src/Interfaces/Software/WebInterface/WebInterface.h

#pragma once

#include "../../Interface/Interface.h"

#include <WebServer.h>
#include <WebSocketsServer.h>
#include <functional>
#include <string>
#include <string_view>
#include <sstream>
#include <iomanip>

struct WebInterfaceConfig : public ModuleConfig {};


class WebInterface : public Interface {
public:
    explicit                    WebInterface                (SystemController& controller);

    // required implementation
    void                        sync_color                  (std::array<uint8_t,3> color)   override;
    void                        sync_brightness             (uint8_t brightness)            override;
    void                        sync_state                  (uint8_t state)                 override;
    void                        sync_mode                   (uint8_t mode)                  override;
    void                        sync_length                 (uint16_t length)               override;

    // optional implementation
    void                        sync_all                    (std::array<uint8_t,3> color,
                                                             uint8_t brightness,
                                                             uint8_t state,
                                                             uint8_t mode,
                                                             uint16_t length)               override;
    void                        begin_routines_required     (const ModuleConfig& cfg)       override;
    void                        begin_routines_regular      (const ModuleConfig& cfg)       override;
    void                        begin_routines_common       (const ModuleConfig& cfg)       override;
    void                        loop                        ()                              override;
    void                        reset                       (const bool verbose=false,
                                                             const bool do_restart=true,
                                                             const bool keep_enabled=true)  override;

    std::string                 status                      (const bool verbose=false)      const override;

    WebServer&                  get_server                  ()                              { return httpServer; }
    void                        sync_param                  (std::string_view key, uint16_t value);

private:
    WebServer                   httpServer                  {80};
    WebSocketsServer            webSocket                   {81};

    uint8_t                     connected_clients           = 0;

    void                        serveMainPage               ();
    void                        handleSetRequest            ();
    void                        handleGetStateRequest       ();
    void                        handleGetModesRequest       ();
    void                        handleGetNameRequest        ();

    void                        webSocketEvent              (uint8_t num,
                                                             WStype_t type,
                                                             uint8_t* payload,
                                                             size_t length);

    void                        broadcast                   (const char* payload, size_t length);

    static const char           INDEX_HTML                  [] PROGMEM;

    uint32_t                    last_heartbeat_ms           = 0;
    static constexpr uint32_t   HEARTBEAT_INTERVAL_MS       = 1000;
};