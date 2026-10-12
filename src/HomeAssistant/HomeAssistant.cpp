// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/src/HomeAssistant/HomeAssistant.cpp

#include "HomeAssistant.h"

#include <ESPmDNS.h>
#include <XeWeBuildInfo.h>

namespace {
// mDNS TXT `fw` and the discovery `sw_version`: the firmware version of this build
constexpr const char* SW_VERSION = BUILD_VERSION;
} // namespace

HomeAssistant::HomeAssistant(
    xewe::Os& host, Wifi& wifi_ref, WebInterface& web_interface_ref, Led& led_ref)
    : xewe::Module(host,
          /* id                  */ "homeassistant",
          /* name                */ "Home_Assistant",
          /* description         */
          "Allows to control the LED from Home Assistant over MQTT auto-discovery.\n"
          "REQUIRES an MQTT broker (the Home Assistant Mosquitto add-on works).",
          /* requires_init_setup */ true,
          /* can_be_disabled     */ true,
          /* has_cli_cmds        */ true),
      wifi(wifi_ref), web_interface(web_interface_ref), led(led_ref),
      bridge(mqtt, led_ref, host, this, SW_VERSION) {
    // wifi directly: web_interface cannot be disabled, so `$wifi disable` reaches this module
    // only through its own requirement
    add_requirement(wifi);
    add_requirement(web_interface);
    add_requirement(led);
}

// ---- led listener (device changes -> HA) ---------------------------------------------------
// Called for every change from any source (CLI, buttons, schedules, web page, HomeKit, Alexa and
// this module itself). Publishing here, synchronously, is how HA sees local changes at once.
void HomeAssistant::on_color(std::array<uint8_t, 3> rgb, const void* origin) {
    (void)rgb;
    (void)origin;
    if (is_disabled())
        return;
    bridge.publish_light_state();
}

void HomeAssistant::on_brightness(uint8_t brightness, const void* origin) {
    (void)brightness;
    (void)origin;
    if (is_disabled())
        return;
    bridge.publish_light_state();
}

void HomeAssistant::on_state(bool on, const void* origin) {
    (void)on;
    (void)origin;
    if (is_disabled())
        return;
    bridge.publish_light_state();
}

void HomeAssistant::on_mode(uint8_t mode_id, const void* origin) {
    (void)mode_id;
    (void)origin;
    if (is_disabled())
        return;
    bridge.publish_mode_state();
    bridge.reconcile_params(); // swap the visible number entities to the new mode's parameters
    bridge.publish_light_state();
}

void HomeAssistant::on_length(uint16_t length, const void* origin) {
    (void)length;
    (void)origin;
    // the strip length is not part of the HA model
}

void HomeAssistant::on_all(std::array<uint8_t, 3> rgb, uint8_t brightness, bool on, uint8_t mode_id,
    uint16_t length, const void* origin) {
    (void)length;
    if (is_disabled())
        return;
    on_state(on, origin);
    on_brightness(brightness, origin);
    on_color(rgb, origin);
    on_mode(mode_id, origin);
}

// ---- lifecycle ------------------------------------------------------------------------------
void HomeAssistant::begin_routines_required() {
    build_topics();

    // registered here, not in the constructor: a disabled module never begins and so never
    // receives a change, and the listeners are told in begin order (led_web first)
    if (!led.listeners.add(this))
        os.serial.print("! Home_Assistant: no free led listener slot; LED changes are not "
                        "published (raise XEWE_MODULE_LED_LISTENERS)");

    // the credential handoff uses the web interface server instead of a second server
    WebServer& server = web_interface.get_server();
    server.on("/provision", HTTP_POST, [this] { handle_provision(); });
    server.on("/deprovision", HTTP_POST, [this] { handle_deprovision(); });
}

void HomeAssistant::begin_routines_init() {
    if (is_disabled())
        return;

    mqtt.setBufferSize(MQTT_BUFFER_SIZE);
    mqtt.setCallback([this](char* t, uint8_t* p, unsigned int l) { bridge.on_message(t, p, l); });

    load_creds();
    if (provisioned) {
        mqtt.setServer(mqtt_host.c_str(), mqtt_port);
        last_reconnect_ms = 0;
        connect();
    }

    WebServer& server = web_interface.get_server();

    bool mdns_started = false;
    std::string mdns_host;

    // advertise the device while it waits for broker credentials: this is what lets Home
    // Assistant (HACS integration) find it before MQTT is set up
    if (!provisioned) {
        mdns_host = ha::mdns_host(mac_hex);

        const std::string device_name = os.system.get_device_name();

        if (MDNS.begin(mdns_host.c_str())) {
            MDNS.addService("_xewe-led-os", "_tcp", MDNS_SERVICE_PORT);
            MDNS.addServiceTxt("_xewe-led-os", "_tcp", "mac", mac_hex.c_str());
            MDNS.addServiceTxt("_xewe-led-os", "_tcp", "fw", SW_VERSION);
            MDNS.addServiceTxt("_xewe-led-os", "_tcp", "name", device_name.c_str());
            mdns_started = true;
        }
    }

    // The web interface starts its HTTP server only on its regular boots, so on its own setup
    // boot nothing would answer /provision. begin() without a port listens on the server's
    // port (80 on that boot); on a regular boot it reopens the configured port.
    server.begin();

    os.serial.print("\nHome Assistant setup required.\n"
                    "\n"
                    "Discovery mode is active. Open Home Assistant > Settings > Devices & "
                    "Services.\n"
                    "If the discovered card does not appear, provision manually by POSTing MQTT "
                    "broker credentials to:\n"
                    "  http://<device-ip>/provision\n"
                    "\n"
                    "JSON body:\n"
                    "  {\"host\":\"<mqtt-host>\",\"port\":1883,\"user\":\"<mqtt-user>\",\"pass\":"
                    "\"<mqtt-pass>\"}\n"
                    "\n"
                    "Example:\n"
                    "  curl -X POST http://<device-ip>/provision \\\n"
                    "    -H 'Content-Type: application/json' \\\n"
                    "    -d "
                    "'{\"host\":\"192.168.1.10\",\"port\":1883,\"user\":\"mqtt\",\"pass\":"
                    "\"password\"}'\n"
                    "\n"
                    "Setup continues automatically after MQTT connects and discovery is "
                    "published.\n"
                    "To abort and disable Home Assistant, press (x): ");

    if (mdns_started) {
        os.serial.print("\n[HomeAssistant] mDNS discovery active: ");
        os.serial.print(mdns_host.c_str());
        os.serial.print("._xewe-led-os._tcp.local");
    } else if (!provisioned) {
        os.serial.print(
            "\n[HomeAssistant] mDNS discovery FAILED; /provision is still available manually.");
    }

    uint32_t last_status_ms = 0;

    // MQTT discovery has no final pairing callback: wait until broker credentials arrive, MQTT
    // connects and connect() has published the retained discovery and current state
    while (!provisioned || !mqtt.connected()) {
        server.handleClient();
        os.serial.loop();

        if (os.serial.has_line()) {
            const std::string input = os.serial.read_line();
            if (!input.empty() && input[0] == 'x') {
                if (mdns_started)
                    MDNS.end();
                disable(false, true); // reset without output, then restart
                return;
            }
            os.serial.print("\n(x)?: ");
        }

        if (provisioned) {
            if (mdns_started) {
                MDNS.end();
                mdns_started = false;
            }
            if (!mqtt.connected()) {
                connect();
            } else {
                mqtt.loop();
            }
        }

        const uint32_t now = millis();
        if (now - last_status_ms >= STATUS_INTERVAL_MS) {
            last_status_ms = now;
            if (!provisioned) {
                os.serial.print("\n[HomeAssistant] waiting for Home Assistant discovery card or "
                                "/provision...");
            } else if (!mqtt.connected()) {
                os.serial.print("\n[HomeAssistant] provisioned; waiting for MQTT connection...");
            }
        }

        delay(5);
    }

    if (mdns_started)
        MDNS.end();
    mqtt.loop();
    os.serial.print("\n[HomeAssistant] paired: MQTT connected, discovery published, state "
                    "retained.\n"
                    "Device should now appear in Home Assistant via MQTT discovery.\n");
}

void HomeAssistant::begin_routines_common() {
    if (is_disabled())
        return;

    mqtt.setBufferSize(MQTT_BUFFER_SIZE);
    mqtt.setCallback([this](char* t, uint8_t* p, unsigned int l) { bridge.on_message(t, p, l); });

    load_creds();
    if (provisioned) {
        mqtt.setServer(mqtt_host.c_str(), mqtt_port);
        last_reconnect_ms = 0;
        connect(); // nothing to do when the first-boot wait already connected
    }
}

void HomeAssistant::loop() {
    if (is_disabled() || !provisioned)
        return;
    if (wifi.is_disconnected())
        return;

    if (!mqtt.connected())
        connect();
    else
        mqtt.loop();
}

void HomeAssistant::reset(const bool verbose, const bool do_restart, const bool keep_enabled) {
    // clear the retained discovery so HA drops the entities before the broker is forgotten (the
    // core reset wipes the whole namespace, credentials included)
    if (mqtt.connected()) {
        bridge.retract();
        delay(100);
        mqtt.disconnect();
    }
    provisioned = false;
    xewe::Module::reset(verbose, do_restart, keep_enabled);
}

std::string HomeAssistant::status(const bool verbose) const {
    if (is_disabled())
        return std::string("Home Assistant module disabled");

    std::string line = "Home Assistant: ";
    if (!provisioned) {
        line += "not provisioned (POST /provision to pair)";
    } else {
        line += "broker " + mqtt_host + ":" + std::to_string(mqtt_port);
        line += mqtt.connected() ? " [connected]" : " [disconnected]";
    }
    os.serial.print(line);
    return xewe::Module::status(verbose);
}

// ---- provisioning state ---------------------------------------------------------------------
void HomeAssistant::set_broker(
    const std::string& host, uint16_t port, const std::string& user, const std::string& pass) {
    mqtt_host = host;
    mqtt_port = port;
    mqtt_user = user;
    mqtt_pass = pass;
    provisioned = !mqtt_host.empty();
    save_creds();
    if (provisioned) {
        mqtt.setServer(mqtt_host.c_str(), mqtt_port);
        last_reconnect_ms = 0; // connect immediately
        connect();
    }
}

void HomeAssistant::clear_broker() {
    if (mqtt.connected()) {
        bridge.retract();
        delay(100);
        mqtt.disconnect();
    }
    os.nvs.remove(id, "host");
    os.nvs.remove(id, "port");
    os.nvs.remove(id, "user");
    os.nvs.remove(id, "pass");
    mqtt_host.clear();
    mqtt_user.clear();
    mqtt_pass.clear();
    mqtt_port = DEFAULT_PORT;
    provisioned = false;
}

// ---- connection -----------------------------------------------------------------------------
void HomeAssistant::connect() {
    if (mqtt.connected())
        return;
    if (!provisioned || mqtt_host.empty())
        return;

    if (last_reconnect_ms != 0 && millis() - last_reconnect_ms < RECONNECT_INTERVAL_MS)
        return;
    last_reconnect_ms = millis();

    const char* user = mqtt_user.empty() ? nullptr : mqtt_user.c_str();
    const char* pass = mqtt_pass.empty() ? nullptr : mqtt_pass.c_str();

    // last will: the broker publishes "offline" to the avail topic when the device drops off
    if (!mqtt.connect(bridge.topics.device_id.c_str(), user, pass, bridge.topics.avail.c_str(), 1,
            true, "offline")) {
        os.serial.print("[HomeAssistant] MQTT connect failed, will retry");
        return;
    }

    bridge.announce();
    os.serial.print("[HomeAssistant] MQTT connected; discovery published");
}

// ---- inbound (HTTP) -------------------------------------------------------------------------
void HomeAssistant::handle_provision() {
    WebServer& server = web_interface.get_server();
    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain"))) {
        server.send(400, "application/json", "{\"error\":\"bad_json\"}");
        return;
    }
    // `| 1883` reads the port as an int, so a number above 65535 wraps in the uint16_t cast
    set_broker(std::string(doc["host"] | ""), (uint16_t)(doc["port"] | 1883),
        std::string(doc["user"] | ""), std::string(doc["pass"] | ""));
    server.send(200, "application/json", "{\"ok\":true}");
    os.serial.print("[HomeAssistant] provisioned via /provision");
}

void HomeAssistant::handle_deprovision() {
    WebServer& server = web_interface.get_server();
    server.send(200, "application/json", "{\"ok\":true}");
    os.serial.print("[HomeAssistant] /deprovision: clearing creds + retained discovery");
    clear_broker();
}

// ---- helpers --------------------------------------------------------------------------------
void HomeAssistant::build_topics() {
    mac_hex = ha::mac_hex(wifi.get_mac_address()); // "AA:BB:..", empty while disconnected
    bridge.topics = ha::make_topics(mac_hex);
}

void HomeAssistant::load_creds() {
    mqtt_host = os.nvs.read<std::string>(id, "host", "");
    mqtt_port = os.nvs.read<uint16_t>(id, "port", DEFAULT_PORT);
    mqtt_user = os.nvs.read<std::string>(id, "user", "");
    mqtt_pass = os.nvs.read<std::string>(id, "pass", "");
    provisioned = !mqtt_host.empty();
}

void HomeAssistant::save_creds() const {
    os.nvs.write<std::string>(id, "host", mqtt_host);
    os.nvs.write<uint16_t>(id, "port", mqtt_port);
    os.nvs.write<std::string>(id, "user", mqtt_user);
    os.nvs.write<std::string>(id, "pass", mqtt_pass);
}
