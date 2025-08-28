#pragma once

#include "../../Interface/Interface.h"
#include "../../../Debug.h"

#include <array>
#include <string>
#include <cstdint>

// ESP32 async web
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Ticker.h>

class SystemController;

struct WebConfig : public ModuleConfig {};

class Web : public Interface {
public:
    explicit Web(SystemController& controller);

    // Module interface
    void begin(const ModuleConfig& cfg) override;
    void loop() override;
    void reset(bool verbose=false) override;

    // Interface sync: push to UI
    void sync_color(std::array<uint8_t,3> color) override;
    void sync_brightness(uint8_t brightness) override;
    void sync_state(uint8_t state) override;
    void sync_mode(uint8_t mode) override;
    void sync_length(uint16_t length) override;
    void sync_all(std::array<uint8_t,3> color,
                  uint8_t brightness,
                  uint8_t state,
                  uint8_t mode,
                  uint16_t length) override;

private:
    // Canonical state cache
    struct Cache {
        std::array<uint8_t,3> rgb {0,0,0};
        uint8_t brightness_255 = 0;
        bool    power          = false;
        uint8_t mode_id        = 0;
    };

    // Helpers
    std::string render_index() const;
    void send_ok(AsyncWebServerRequest* req);
    void send_options(AsyncWebServerRequest* req);
    void push_patch(const std::string& json);
    void broadcast_state_with_meta();

    // heartbeat (Ticker sets flag, loop sends SSE safely)
    void arm_heartbeat();
    volatile bool heartbeat_due_ = false;
    uint32_t      last_heartbeat_ms_ = 0;

    // Members
    AsyncWebServer       server_{80};
    AsyncEventSource     events_ {"/events"};
    mutable Cache        cache_;
    SemaphoreHandle_t    cache_mutex_ {nullptr};
    Ticker               hb_ticker_;
};
