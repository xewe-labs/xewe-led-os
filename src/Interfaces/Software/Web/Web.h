#pragma once

#include <WebServer.h>
#include <WebSocketsServer.h>
#include <array>

#include "../../Interface/Interface.h"
#include "../../../Debug.h"

class SystemController;

/** Config passed from SystemController (no server pointer). */
struct WebConfig : public ModuleConfig {
};

class Web : public Interface {
public:
    explicit Web(SystemController& controller);

    // Interface overrides
    void begin(const ModuleConfig& cfg) override;
    void loop() override;
    void reset(bool verbose = false) override;

    void sync_color(std::array<uint8_t,3> color) override;
    void sync_brightness(uint8_t brightness) override;
    void sync_state(uint8_t state) override;      // 0/1
    void sync_mode(uint8_t mode) override;        // mode id
    void sync_length(uint16_t length) override;
    void sync_all(std::array<uint8_t,3> color,
                  uint8_t brightness,
                  uint8_t state,
                  uint8_t mode,
                  uint16_t length) override;

    void status(); // optional diagnostics

private:
    // Web owns its own servers
    WebServer        httpServer{80};
    WebSocketsServer webSocket{81};

    uint8_t connected_clients = 0;
    bool    wifi_enabled_ = false;

    // HTTP handlers
    void serveMainPage();
    void handleSetRequest();
    void handleSetStateShortcut();
    void handleGetStateRequest();
    void handleGetModesRequest();
    void handleGetNameRequest();

    // WS handler
    void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);

    // Broadcast helper
    void broadcast(const char* payload, size_t length);

    // HTML assets
    static const char INDEX_HTML[] PROGMEM;
    static const char SET_STATE_HTML[] PROGMEM;

    uint32_t last_heartbeat_ms = 0;
    static constexpr uint32_t HEARTBEAT_INTERVAL_MS = 1000; // 1s
};
