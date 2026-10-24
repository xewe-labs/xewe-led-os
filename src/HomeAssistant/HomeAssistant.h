// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/src/HomeAssistant/HomeAssistant.h
#pragma once

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <WiFi.h>

#include <array>
#include <cstdint>
#include <string>

#include <XeWeCore.h>

#include <Led/Led.h>
#include <WebInterface/WebInterface.h>
#include <Wifi/Wifi.h>

#include "HaBridge.h"
#include "HaDiscovery.h"

class HomeAssistant : public xewe::Module, public LedListener {
public:
    HomeAssistant(xewe::Os& host, Wifi& wifi_ref, WebInterface& web_interface_ref, Led& led_ref);

    // led listener: device changes -> MQTT. Changes made by this module are published too: the
    // echo is how Home Assistant sees the confirmed state.
    void on_color(std::array<uint8_t, 3> rgb, const void* origin) override;
    void on_brightness(uint8_t brightness, const void* origin) override;
    void on_state(bool on, const void* origin) override;
    void on_mode(uint8_t mode_id, const void* origin) override;
    void on_length(uint16_t length, const void* origin) override;
    // `$led reset`: state, brightness, colour, then mode
    void on_all(std::array<uint8_t, 3> rgb, uint8_t brightness, bool on, uint8_t mode_id,
        uint16_t length, const void* origin) override;

    // topics, led listener, POST /provision and /deprovision
    void begin_routines_required() override;
    // first boot: blocks until a broker is provisioned and MQTT connects (x aborts and disables)
    void begin_routines_init() override;
    // MQTT client setup and the first connect
    void begin_routines_common() override;
    // reconnect every 3 s while provisioned and Wi-Fi is up; MQTT keep-alive and inbound messages
    void loop() override;
    // clears the retained discovery and state, publishes offline, then the core reset (which
    // forgets the broker)
    void reset(const bool verbose = false, const bool do_restart = true,
        const bool keep_enabled = true) override;
    // broker line, then `Home_Assistant module enabled`; nothing printed when disabled
    std::string status(const bool verbose = false) const override;

    // store the broker (empty host = unprovisioned) and connect at once
    void set_broker(
        const std::string& host, uint16_t port, const std::string& user, const std::string& pass);
    // clear the retained entities, publish offline, disconnect and forget the broker
    void clear_broker();

private:
    // ---- connection
    void start_client();
    void connect_now();
    void connect();
    void retract_and_disconnect();

    // ---- inbound (HTTP)
    void handle_provision();
    void handle_deprovision();

    // ---- helpers
    void build_topics();
    void load_creds();
    void save_creds() const;

    Wifi& wifi;
    WebInterface& web_interface;
    Led& led;

    // status() is const but PubSubClient::connected() is not
    WiFiClient net;
    mutable PubSubClient mqtt{net};

    // publishes, applies commands; holds the topics and the published parameter keys
    ha::Bridge<PubSubClient, Led, xewe::Os> bridge;

    std::string mac_hex;

    std::string mqtt_host, mqtt_user, mqtt_pass;
    uint16_t mqtt_port = DEFAULT_PORT;
    bool provisioned = false;

    uint32_t last_reconnect_ms = 0;

    static constexpr uint16_t DEFAULT_PORT = 1883;
    static constexpr uint16_t MQTT_BUFFER_SIZE = 2048; // discovery JSON exceeds the 256 B default
    static constexpr uint32_t RECONNECT_INTERVAL_MS = 3000;
    static constexpr uint32_t STATUS_INTERVAL_MS = 3000; // first-boot wait status lines
    static constexpr uint16_t MDNS_SERVICE_PORT = 80; // advertised for HACS discovery
};
