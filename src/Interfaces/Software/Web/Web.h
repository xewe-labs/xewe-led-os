#pragma once
/**
 * Web.h
 * Lean ESP32 Async Web Server interface for LED control UI.
 *
 * API:
 *  - GET  /            -> index.html (UI)
 *  - GET  /styles.css  -> styles (immutable cached)
 *  - GET  /script.js   -> JS (immutable cached)
 *  - GET  /api/state   -> {brightness,state,mode,length,color:[r,g,b]}   // TARGET values
 *  - GET  /api/modes   -> {modes:[{id,name},...]}
 *  - POST /api/update  -> partial update: any of {brightness,state,mode,length,color:[r,g,b]}
 *                         responds with full canonical state (same shape as /api/state)
 *  - WS   /ws          -> on connect: server pushes full canonical state; on any change: broadcast
 *
 * Notes:
 *  - No HSV on backend. All conversions live in the frontend JS.
 *  - We always read TARGET getters to avoid "one update behind".
 *  - sync_*() overrides only broadcast via WebSocket.
 */

#include "../../Interface/Interface.h"
#include "../../../Debug.h"

#include <array>
#include <cstdint>

class AsyncWebServer;
class AsyncWebServerRequest;
class AsyncWebSocket;
class AsyncWebSocketClient;

struct WebConfig : public ModuleConfig {
    uint16_t port = 80;
};

class Web : public Interface {
public:
    explicit        Web                         (SystemController& controller);

    // Required Interface implementation
    void            begin                       (const ModuleConfig& cfg)       override;
    void            loop                        ()                              override;
    void            reset                       (bool verbose=false)            override;

    // Backend -> UI (Controller calls these; we broadcast WS)
    void            sync_color                  (std::array<uint8_t,3> color)   override;
    void            sync_brightness             (uint8_t brightness)            override;
    void            sync_state                  (uint8_t state)                 override; // 0/1
    void            sync_mode                   (uint8_t mode)                  override;
    void            sync_length                 (uint16_t length)               override;
    void            sync_all                    (std::array<uint8_t,3> color,
                                                 uint8_t brightness,
                                                 uint8_t state,
                                                 uint8_t mode,
                                                 uint16_t length)               override;

private:
    // HTTP routing
    void            setup_routes_               ();
    void            send_index_                 (AsyncWebServerRequest* req);
    void            send_css_                   (AsyncWebServerRequest* req);
    void            send_js_                    (AsyncWebServerRequest* req);
    void            send_state_json_            (AsyncWebServerRequest* req);
    void            send_modes_json_            (AsyncWebServerRequest* req);
    void            handle_update_body_         (AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total);

    // Helpers
    static uint8_t  clamp8_                     (int v);
    static uint16_t clamp16_                    (int v);
    void            build_state_json_string_    (String& out) const;
    void            broadcast_state_ws_         ();
    static void     add_no_cache_               (AsyncWebServerRequest* req, class AsyncWebServerResponse* res);
    static void     add_immutable_cache_        (AsyncWebServerRequest* req, class AsyncWebServerResponse* res);
    std::array<uint8_t, 5> make_flags_all_on_() const;

private:
    AsyncWebServer*     server_     = nullptr;
    AsyncWebSocket*     ws_         = nullptr;
    uint16_t            port_       = 80;
};
