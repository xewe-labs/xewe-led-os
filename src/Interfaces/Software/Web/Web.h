#pragma once
/**
 * Web.h
 * ESP32 Async Web Server interface for LED control UI.
 *
 * API:
 *  - GET  /            -> index.html (UI)
 *  - GET  /styles.css  -> styles
 *  - GET  /script.js   -> JS
 *  - GET  /api/state   -> {brightness,state,mode,length,color:[r,g,b]}   // TARGET values, 0..255 on wire
 *  - GET  /api/modes   -> {modes:[{id,name},...]}
 *  - POST /api/update  -> partial update: any of {brightness,state,mode,length,color:[r,g,b]}
 *                         responds with full canonical state (same shape as /api/state)
 *  - WS   /ws          -> pushes full canonical state JSON on connect and on any change
 *
 * Notes:
 *  - Backend is RGB-only; all HSV conversions happen in the UI.
 *  - STATE JSON always reflects TARGET getters to avoid "one update behind".
 *  - sync_*() only broadcasts (so other clients update immediately).
 */

#include "../../Interface/Interface.h"
#include "../../../Debug.h"

#include <array>
#include <cstdint>

class AsyncWebServer;
class AsyncWebServerRequest;
class AsyncWebSocket;   // forward decl
class AsyncWebSocketClient;

struct WebConfig : public ModuleConfig {
    uint16_t port = 80;  // -fno-rtti: ignore downcast; default 80
};

class Web : public Interface {
public:
    explicit        Web                         (SystemController& controller);

    // Required Interface implementation
    void            begin                       (const ModuleConfig& cfg)       override;
    void            loop                        ()                              override;
    void            reset                       (bool verbose=false)            override;

    void            sync_color                  (std::array<uint8_t,3> color)   override;
    void            sync_brightness             (uint8_t brightness)            override;
    void            sync_state                  (uint8_t state)                 override;
    void            sync_mode                   (uint8_t mode)                  override;
    void            sync_length                 (uint16_t length)               override;
    void            sync_all                    (std::array<uint8_t,3> color,
                                                 uint8_t brightness,
                                                 uint8_t state,
                                                 uint8_t mode,
                                                 uint16_t length)               override;

private:
    // HTTP helpers
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

private:
    AsyncWebServer*     server_     = nullptr;
    AsyncWebSocket*     ws_         = nullptr;  // primary realtime sync
    uint16_t            port_       = 80;
};
