#pragma once

/**
 * Web.h
 * ESP32 Async Web Server interface for LED control UI.
 *
 * Features:
 *  - Serves a mobile-first UI (index.html, styles.css, script.js) from PROGMEM.
 *  - API: /api/state, /api/modes, /api/update (0–255 on the wire).
 *  - SSE at /events for optional live pushes when sync_* is invoked by the controller.
 */

#include "../../Interface/Interface.h"
#include "../../../Debug.h"

#include <array>
#include <cstdint>

class AsyncWebServer;
class AsyncEventSource;
class AsyncWebServerRequest;

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
    // --- HTTP helpers ---
    void            setup_routes_               ();
    void            send_index_                 (AsyncWebServerRequest* req);
    void            send_css_                   (AsyncWebServerRequest* req);
    void            send_js_                    (AsyncWebServerRequest* req);
    void            send_state_json_            (AsyncWebServerRequest* req);
    void            send_modes_json_            (AsyncWebServerRequest* req);
    void            handle_update_body_         (AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total);

    // --- State helpers ---
    static uint8_t  clamp8_                     (int v);
    static uint16_t clamp16_                    (int v);
    static void     hsv_to_rgb255_              (uint8_t h255, uint8_t s255, uint8_t v255, uint8_t& r, uint8_t& g, uint8_t& b);
    static void     rgb255_to_hsv_              (uint8_t r, uint8_t g, uint8_t b, uint8_t& h255, uint8_t& s255, uint8_t& v255);
    void            recompute_color_from_hv_    ();
    void            recompute_hv_from_color_    ();
    void            normalize_power_            ();

    void            broadcast_state_sse_        (); // optional; safe if no clients connected
    void            build_state_json_string_    (String& out) const;

private:
    // HTTP
    AsyncWebServer*     server_     = nullptr;
    AsyncEventSource*   events_     = nullptr;
    uint16_t            port_       = 80;

    // Cached state (0–255 on wire; length fits 0–255 though kept uint16_t for controller)
    std::array<uint8_t,3> color_    {255, 0, 0};
    uint8_t                brightness_ = 128;   // 0..255
    uint8_t                state_      = 255;   // 0 or 255
    uint8_t                mode_       = 0;     // 0..255
    uint16_t               length_     = 128;   // controller-facing (UI clamps to <=255)
    uint8_t                hue_        = 0;     // 0..255; 0 and 255 are both red
};
