// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Software/SmartHome/HomeAssistant/HomeAssistant.cpp

#include "HomeAssistant.h"
#include "../../../Module/ModuleController.h"
#include "../../../../Utils/XeWeColor.h"

#include <ESPmDNS.h>
#include <WebServer.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cmath>
#include <cstdlib>
#include <utility>

using namespace xewe::color;

namespace {

constexpr std::size_t HOME_ASSISTANT_SYNC_INDEX = 2;

std::array<uint8_t, SYNC_MODULES_COUNT> other_sync_modules() {
    static_assert(HOME_ASSISTANT_SYNC_INDEX < SYNC_MODULES_COUNT);

    std::array<uint8_t, SYNC_MODULES_COUNT> flags{};
    flags.fill(1);
    flags[HOME_ASSISTANT_SYNC_INDEX] = 0;
    return flags;
}

} // namespace


HomeAssistant::HomeAssistant(ModuleController& controller)
    : SyncModule(controller,
          /* id                  */ "homeassistant",
          /* name                */ "HomeAssistant",
          /* description         */ "Allows Home Assistant to control the LED strip through MQTT discovery.\nREQUIRES an MQTT broker.",
          /* requires_init_setup */ true,
          /* can_be_disabled     */ true,
          /* has_cli_cmds        */ true
    )
{}

// =============================================================================
// SyncModule implementation
// =============================================================================
void HomeAssistant::sync_color(std::array<uint8_t, 3> color) {
    (void)color;
    if (is_disabled()) return;
    publish_light_state();
}

void HomeAssistant::sync_brightness(uint8_t brightness) {
    (void)brightness;
    if (is_disabled()) return;
    publish_light_state();
}

void HomeAssistant::sync_state(bool state) {
    (void)state;
    if (is_disabled()) return;
    publish_light_state();
}

void HomeAssistant::sync_mode(uint8_t mode) {
    (void)mode;
    if (is_disabled()) return;

    publish_mode_state();
    reconcile_params();
    publish_light_state();
}

void HomeAssistant::sync_length(uint16_t length) {
    (void)length;
    // The Home Assistant model does not expose the LED strip length.
}

void HomeAssistant::sync_all(std::array<uint8_t, 3> color,
                             uint8_t brightness,
                             bool state,
                             uint8_t mode,
                             uint16_t length) {
    (void)color;
    (void)brightness;
    (void)state;
    (void)mode;
    (void)length;

    if (is_disabled()) return;

    publish_light_state();
    publish_mode_state();
    reconcile_params();
}

void HomeAssistant::sync_param(std::string_view key,
                               uint16_t value) {
    if (is_disabled()) return;
    publish_param_state(std::string(key), value);
}

// =============================================================================
// Module lifecycle
// =============================================================================
void HomeAssistant::begin_routines_required(const ModuleConfig& cfg) {
    (void)cfg;

    build_topics();

    mqtt.setBufferSize(MQTT_BUFFER_SIZE);
    mqtt.setCallback([this](char* topic, uint8_t* payload, unsigned int length) {
        on_message(topic, payload, length);
    });

    load_creds();
    if (provisioned) {
        mqtt.setServer(mqtt_host.c_str(), mqtt_port);
    }

    WebServer& server = controller.web_interface.get_server();
    server.on("/provision", HTTP_POST, [this] { handle_provision(); });
    server.on("/deprovision", HTTP_POST, [this] { handle_deprovision(); });
}

void HomeAssistant::begin_routines_init(const ModuleConfig& cfg) {
    (void)cfg;

    if (provisioned) {
        last_reconnect_ms = 0;
        connect();
    }

    WebServer& server = controller.web_interface.get_server();

    bool        mdns_started = false;
    std::string mdns_host;

    if (!provisioned) {
        mdns_host = "xewe-led-os-";
        if (mac_hex.size() > 6) {
            mdns_host += mac_hex.substr(mac_hex.size() - 6);
        } else {
            mdns_host += mac_hex;
        }

        const std::string device_name = controller.system.get_device_name();

        if (MDNS.begin(mdns_host.c_str())) {
            MDNS.addService("_xewe-led-os", "_tcp", 80);
            MDNS.addServiceTxt("_xewe-led-os", "_tcp", "mac", mac_hex.c_str());
            MDNS.addServiceTxt("_xewe-led-os", "_tcp", "fw", BUILD_VERSION);
            MDNS.addServiceTxt("_xewe-led-os", "_tcp", "name", device_name.c_str());
            mdns_started = true;
        }
    }

    controller.serial_port.print(
        "\nHome Assistant setup required.\n"
        "\n"
        "Discovery mode is active. Open Home Assistant > Settings > Devices & Services.\n"
        "If the discovered card does not appear, send MQTT broker credentials to:\n"
        "  http://<device-ip>/provision\n"
        "\n"
        "JSON body:\n"
        "  {\"host\":\"<mqtt-host>\",\"port\":1883,\"user\":\"<mqtt-user>\",\"pass\":\"<mqtt-pass>\"}\n"
        "\n"
        "Example:\n"
        "  curl -X POST http://<device-ip>/provision \\\n"
        "    -H 'Content-Type: application/json' \\\n"
        "    -d '{\"host\":\"192.168.1.10\",\"port\":1883,\"user\":\"mqtt\",\"pass\":\"password\"}'\n"
        "\n"
        "Setup continues after MQTT connects and discovery is published.\n"
        "To abort and disable Home Assistant, press (x): "
    );

    if (mdns_started) {
        controller.serial_port.print("\n[HomeAssistant] mDNS discovery active: ");
        controller.serial_port.print(mdns_host.c_str());
        controller.serial_port.print("._xewe-led-os._tcp.local");
    } else if (!provisioned) {
        controller.serial_port.print(
            "\n[HomeAssistant] mDNS discovery failed. The /provision endpoint is still available."
        );
    }

    uint32_t last_status_ms = 0;

    while (!provisioned || !mqtt.connected()) {
        server.handleClient();
        controller.serial_port.loop();

        if (controller.serial_port.has_line()) {
            const std::string input = controller.serial_port.read_line();
            if (!input.empty() && input[0] == 'x') {
                if (mdns_started) MDNS.end();
                disable(false, true);
                return;
            }
            controller.serial_port.print("\n(x)?: ");
        }

        if (provisioned) {
            if (mdns_started) {
                MDNS.end();
                mdns_started = false;
            }

            if (mqtt.connected()) {
                mqtt.loop();
            } else {
                connect();
            }
        }

        const uint32_t now = millis();
        if (now - last_status_ms >= 3000) {
            last_status_ms = now;
            if (!provisioned) {
                controller.serial_port.print(
                    "\n[HomeAssistant] waiting for broker credentials..."
                );
            } else if (!mqtt.connected()) {
                controller.serial_port.print(
                    "\n[HomeAssistant] provisioned; waiting for the MQTT broker..."
                );
            }
        }

        delay(5);
    }

    if (mdns_started) MDNS.end();
    mqtt.loop();

    controller.serial_port.print(
        "\n[HomeAssistant] paired: MQTT connected and discovery published.\n"
        "The device is now available through Home Assistant MQTT discovery.\n"
    );
}

void HomeAssistant::begin_routines_regular(const ModuleConfig& cfg) {
    (void)cfg;

    if (!provisioned) return;

    last_reconnect_ms = 0;
    connect();
}

void HomeAssistant::loop() {
    if (is_disabled() || !provisioned) return;
    if (controller.wifi.is_disconnected()) return;

    if (mqtt.connected()) {
        mqtt.loop();
    } else {
        connect();
    }
}

void HomeAssistant::reset(const bool verbose,
                          const bool do_restart,
                          const bool keep_enabled) {
    if (mqtt.connected()) {
        clear_all_retained();
        mqtt.publish(avail_topic.c_str(), "offline", true);
        mqtt.loop();
        delay(100);
        mqtt.disconnect();
    }

    mqtt_host.clear();
    mqtt_user.clear();
    mqtt_pass.clear();
    mqtt_port          = 1883;
    provisioned        = false;
    last_reconnect_ms  = 0;
    published_param_keys.clear();

    controller.nvs.reset_ns(LEGACY_NVS_NAMESPACE);
    Module::reset(verbose, do_restart, keep_enabled);
}

std::string HomeAssistant::status(const bool verbose) const {
    if (is_disabled()) return Module::status(verbose);

    std::string status_string = "Home Assistant: ";
    if (!provisioned) {
        status_string += "not provisioned (POST /provision to pair)";
    } else {
        status_string += "broker " + mqtt_host + ":" + std::to_string(mqtt_port);
        status_string += mqtt.connected() ? " [connected]" : " [disconnected]";
    }

    if (verbose) controller.serial_port.print(status_string);
    return status_string;
}

// =============================================================================
// MQTT connection
// =============================================================================
void HomeAssistant::connect() {
    if (mqtt.connected()) return;
    if (!provisioned || mqtt_host.empty()) return;
    if (controller.wifi.is_disconnected()) return;

    if (last_reconnect_ms != 0 &&
        millis() - last_reconnect_ms < RECONNECT_INTERVAL_MS) {
        return;
    }
    last_reconnect_ms = millis();

    const char* user = mqtt_user.empty() ? nullptr : mqtt_user.c_str();
    const char* pass = mqtt_pass.empty() ? nullptr : mqtt_pass.c_str();

    if (!mqtt.connect(device_id.c_str(),
                      user,
                      pass,
                      avail_topic.c_str(),
                      1,
                      true,
                      "offline")) {
        controller.serial_port.print(
            "[HomeAssistant] MQTT connection failed. A new attempt will follow."
        );
        return;
    }

    mqtt.publish(avail_topic.c_str(), "online", true);

    publish_light_discovery();
    publish_mode_discovery();
    publish_light_state();
    publish_mode_state();
    reconcile_params();

    mqtt.subscribe(light_cmd_topic.c_str());
    mqtt.subscribe(mode_cmd_topic.c_str());
    mqtt.subscribe((base_topic + "/param/+/set").c_str());

    controller.serial_port.print(
        "[HomeAssistant] MQTT connected; discovery published"
    );
}

// =============================================================================
// MQTT inbound
// =============================================================================
void HomeAssistant::on_message(char* topic,
                               uint8_t* payload,
                               unsigned int length) {
    if (topic == nullptr || payload == nullptr) return;

    const std::string topic_string(topic);
    const std::string message(reinterpret_cast<char*>(payload), length);

    if (topic_string == light_cmd_topic) {
        handle_light_command(message);
        return;
    }

    if (topic_string == mode_cmd_topic) {
        JsonDocument modes;
        if (deserializeJson(modes, controller.led_strip.get_all_modes_json())) return;

        for (JsonObjectConst item : modes.as<JsonArrayConst>()) {
            if (message != (item["name"] | "")) continue;

            const uint8_t mode = static_cast<uint8_t>(item["id"] | 0);
            controller.sync_mode(mode, other_sync_modules());

            publish_mode_state();
            reconcile_params();
            publish_light_state();
            break;
        }
        return;
    }

    const std::string prefix = base_topic + "/param/";
    const std::string suffix = "/set";

    if (topic_string.rfind(prefix, 0) != 0) return;
    if (topic_string.size() <= prefix.size() + suffix.size()) return;
    if (topic_string.compare(topic_string.size() - suffix.size(),
                             suffix.size(),
                             suffix) != 0) {
        return;
    }

    const std::string key = topic_string.substr(
        prefix.size(),
        topic_string.size() - prefix.size() - suffix.size()
    );

    char* end = nullptr;
    const long parsed = std::strtol(message.c_str(), &end, 10);
    if (end == message.c_str()) return;
    if (end != message.c_str() + message.size()) return;

    const long constrained = std::clamp<long>(parsed, 0, 65535);
    controller.led_strip.set_mode_param(
        key,
        static_cast<uint16_t>(constrained)
    );

    publish_param_state(
        key,
        controller.led_strip.get_current_mode_param(key)
    );
}

void HomeAssistant::handle_light_command(const std::string& json) {
    JsonDocument doc;
    if (deserializeJson(doc, json)) return;

    const auto flags = other_sync_modules();
    bool       changed = false;

    if (doc["state"].is<const char*>()) {
        const std::string state = doc["state"] | "";
        if (state == "ON") {
            controller.sync_state(true, flags);
            changed = true;
        } else if (state == "OFF") {
            controller.sync_state(false, flags);
            changed = true;
        }
    }

    if (doc["brightness"].is<int>()) {
        const int brightness = std::clamp(doc["brightness"].as<int>(), 0, 255);
        controller.sync_brightness(static_cast<uint8_t>(brightness), flags);
        changed = true;
    }

    if (doc["color"].is<JsonObjectConst>()) {
        const JsonObjectConst color = doc["color"];
        std::array<uint8_t, 3> hsv = controller.led_strip.get_hsv();

        if (!color["h"].isNull()) {
            const float hue = std::clamp(color["h"].as<float>(), 0.0f, 360.0f);
            hsv[0] = static_cast<uint8_t>(std::lround(hue / 360.0f * 255.0f));
        }

        if (!color["s"].isNull()) {
            const float saturation = std::clamp(color["s"].as<float>(), 0.0f, 100.0f);
            hsv[1] = static_cast<uint8_t>(std::lround(saturation / 100.0f * 255.0f));
        }

        hsv[2] = 255;
        controller.sync_color(hsv_to_rgb(hsv), flags);
        changed = true;
    }

    if (changed) publish_light_state();
}

void HomeAssistant::handle_provision() {
    WebServer& server = controller.web_interface.get_server();

    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"missing_body\"}");
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain"))) {
        server.send(400, "application/json", "{\"error\":\"bad_json\"}");
        return;
    }

    const std::string host = doc["host"] | "";
    if (host.empty()) {
        server.send(400, "application/json", "{\"error\":\"missing_host\"}");
        return;
    }

    const long port_value = doc["port"].isNull() ? 1883 : doc["port"].as<long>();
    if (port_value < 1 || port_value > 65535) {
        server.send(400, "application/json", "{\"error\":\"bad_port\"}");
        return;
    }

    set_broker(
        host,
        static_cast<uint16_t>(port_value),
        std::string(doc["user"] | ""),
        std::string(doc["pass"] | "")
    );

    server.send(200, "application/json", "{\"ok\":true}");
    controller.serial_port.print(
        "[HomeAssistant] provisioned through /provision"
    );
}

void HomeAssistant::handle_deprovision() {
    WebServer& server = controller.web_interface.get_server();
    server.send(200, "application/json", "{\"ok\":true}");

    controller.serial_port.print(
        "[HomeAssistant] /deprovision: clearing credentials and retained discovery"
    );
    clear_broker();
}

// =============================================================================
// Provisioning state
// =============================================================================
void HomeAssistant::set_broker(const std::string& host,
                               uint16_t port,
                               const std::string& user,
                               const std::string& pass) {
    if (host.empty()) {
        clear_broker();
        return;
    }

    if (mqtt.connected()) {
        clear_all_retained();
        mqtt.publish(avail_topic.c_str(), "offline", true);
        mqtt.loop();
        delay(100);
        mqtt.disconnect();
    }

    mqtt_host   = host;
    mqtt_port   = port == 0 ? 1883 : port;
    mqtt_user   = user;
    mqtt_pass   = pass;
    provisioned = true;

    save_creds();

    mqtt.setServer(mqtt_host.c_str(), mqtt_port);
    last_reconnect_ms = 0;
    connect();
}

void HomeAssistant::clear_broker() {
    if (mqtt.connected()) {
        clear_all_retained();
        mqtt.publish(avail_topic.c_str(), "offline", true);
        mqtt.loop();
        delay(100);
        mqtt.disconnect();
    }

    controller.nvs.remove(id, "host");
    controller.nvs.remove(id, "port");
    controller.nvs.remove(id, "user");
    controller.nvs.remove(id, "pass");
    controller.nvs.reset_ns(LEGACY_NVS_NAMESPACE);

    mqtt_host.clear();
    mqtt_user.clear();
    mqtt_pass.clear();
    mqtt_port          = 1883;
    provisioned        = false;
    last_reconnect_ms  = 0;
    published_param_keys.clear();
}

// =============================================================================
// MQTT outbound
// =============================================================================
void HomeAssistant::add_device(JsonDocument& doc) const {
    JsonObject device = doc["device"].to<JsonObject>();
    device["identifiers"][0] = device_id;
    device["name"]           = controller.system.get_device_name();
    device["manufacturer"]   = "XeWe";
    device["model"]          = "LED";
    device["sw_version"]     = BUILD_VERSION;
}

void HomeAssistant::publish_light_discovery() {
    if (!mqtt.connected()) return;

    JsonDocument doc;
    doc["~"]                        = base_topic;
    doc["schema"]                   = "json";
    doc["name"]                     = nullptr;
    doc["unique_id"]                = device_id + "_light";
    doc["command_topic"]            = "~/light/set";
    doc["state_topic"]              = "~/light/state";
    doc["availability_topic"]       = "~/avail";
    doc["payload_available"]        = "online";
    doc["payload_not_available"]    = "offline";
    doc["brightness"]               = true;
    doc["color_mode"]               = true;
    doc["supported_color_modes"][0] = "hs";
    add_device(doc);

    std::string payload;
    serializeJson(doc, payload);

    if (!mqtt.publish(light_discovery_topic.c_str(), payload.c_str(), true)) {
        controller.serial_port.print(
            "[HomeAssistant] light discovery publish failed"
        );
    }
}

void HomeAssistant::publish_mode_discovery() {
    if (!mqtt.connected()) return;

    JsonDocument doc;
    doc["~"]                  = base_topic;
    doc["name"]               = "Mode";
    doc["unique_id"]          = device_id + "_mode";
    doc["command_topic"]      = "~/mode/set";
    doc["state_topic"]        = "~/mode/state";
    doc["availability_topic"] = "~/avail";

    JsonArray    options = doc["options"].to<JsonArray>();
    JsonDocument modes;
    if (!deserializeJson(modes, controller.led_strip.get_all_modes_json())) {
        for (JsonObjectConst item : modes.as<JsonArrayConst>()) {
            options.add(std::string(item["name"] | ""));
        }
    }

    add_device(doc);

    std::string payload;
    serializeJson(doc, payload);
    mqtt.publish(mode_discovery_topic.c_str(), payload.c_str(), true);
}

void HomeAssistant::publish_light_state() {
    if (!mqtt.connected()) return;

    const std::array<uint8_t, 3> hsv = controller.led_strip.get_hsv();

    JsonDocument doc;
    doc["state"]      = controller.led_strip.get_state() ? "ON" : "OFF";
    doc["brightness"] = controller.led_strip.get_brightness();
    doc["color_mode"] = "hs";

    JsonObject color = doc["color"].to<JsonObject>();
    color["h"] = std::lround(hsv[0] / 255.0f * 360.0f);
    color["s"] = std::lround(hsv[1] / 255.0f * 100.0f);

    std::string payload;
    serializeJson(doc, payload);
    mqtt.publish(light_state_topic.c_str(), payload.c_str(), true);
}

void HomeAssistant::publish_mode_state() {
    if (!mqtt.connected()) return;

    const std::string mode_name(controller.led_strip.get_current_mode_name());
    mqtt.publish(mode_state_topic.c_str(), mode_name.c_str(), true);
}

void HomeAssistant::reconcile_params() {
    if (!mqtt.connected()) return;

    JsonDocument modes;
    if (deserializeJson(modes, controller.led_strip.get_all_modes_json())) return;

    const uint8_t current_mode = controller.led_strip.get_current_mode_id();
    std::set<std::string> all_keys;
    std::set<std::string> current_keys;

    for (JsonObjectConst mode : modes.as<JsonArrayConst>()) {
        const bool is_current = static_cast<uint8_t>(mode["id"] | 0) == current_mode;

        for (JsonObjectConst param : mode["params"].as<JsonArrayConst>()) {
            const std::string key = std::string(param["key"] | "");
            if (key.empty()) continue;

            all_keys.insert(key);
            if (!is_current) continue;

            current_keys.insert(key);
            publish_param_discovery(param);
            publish_param_state(
                key,
                controller.led_strip.get_current_mode_param(key)
            );
        }
    }

    all_keys.insert(published_param_keys.begin(), published_param_keys.end());
    for (const std::string& key : all_keys) {
        if (current_keys.count(key) == 0) clear_param(key);
    }

    published_param_keys = std::move(current_keys);
}

void HomeAssistant::publish_param_discovery(JsonObjectConst param) {
    if (!mqtt.connected()) return;

    const std::string key = std::string(param["key"] | "");
    if (key.empty()) return;

    JsonDocument doc;
    doc["~"]                  = base_topic;
    doc["name"]               = param["display_name"];
    doc["unique_id"]          = device_id + "_" + key;
    doc["command_topic"]      = "~/param/" + key + "/set";
    doc["state_topic"]        = "~/param/" + key + "/state";
    doc["availability_topic"] = "~/avail";
    doc["min"]                = param["min"];
    doc["max"]                = param["max"];
    doc["step"]               = param["step"];

    const char* type = param["type"] | "b";
    if (type[0] == 'a') doc["entity_category"] = "config";

    add_device(doc);

    std::string payload;
    serializeJson(doc, payload);
    mqtt.publish(param_discovery_topic(key).c_str(), payload.c_str(), true);
}

void HomeAssistant::publish_param_state(const std::string& key,
                                        uint16_t value) {
    if (!mqtt.connected()) return;

    const std::string payload = std::to_string(value);
    mqtt.publish(param_state_topic(key).c_str(), payload.c_str(), true);
}

void HomeAssistant::clear_param(const std::string& key) {
    if (!mqtt.connected()) return;

    mqtt.publish(param_discovery_topic(key).c_str(), "", true);
    mqtt.publish(param_state_topic(key).c_str(), "", true);
}

void HomeAssistant::clear_all_retained() {
    if (!mqtt.connected()) return;

    mqtt.publish(light_discovery_topic.c_str(), "", true);
    mqtt.publish(light_state_topic.c_str(), "", true);
    mqtt.publish(mode_discovery_topic.c_str(), "", true);
    mqtt.publish(mode_state_topic.c_str(), "", true);

    std::set<std::string> keys = all_param_keys();
    keys.insert(published_param_keys.begin(), published_param_keys.end());

    for (const std::string& key : keys) clear_param(key);
    published_param_keys.clear();
}

// =============================================================================
// Helpers
// =============================================================================
void HomeAssistant::build_topics() {
    mac_hex    = mac_to_hex();
    device_id  = "xewe_led_os_" + mac_hex;
    base_topic = "xewe_led_os/" + device_id;

    avail_topic           = base_topic + "/avail";
    light_cmd_topic       = base_topic + "/light/set";
    light_state_topic     = base_topic + "/light/state";
    light_discovery_topic = std::string(DISCOVERY_PREFIX) +
                            "/light/" + device_id + "/config";
    mode_cmd_topic        = base_topic + "/mode/set";
    mode_state_topic      = base_topic + "/mode/state";
    mode_discovery_topic  = std::string(DISCOVERY_PREFIX) +
                            "/select/" + device_id + "/config";
}

std::string HomeAssistant::mac_to_hex() const {
    const std::string mac = controller.wifi.get_mac_address();

    std::string result;
    result.reserve(12);

    for (char character : mac) {
        if (character == ':') continue;
        result += static_cast<char>(
            std::tolower(static_cast<unsigned char>(character))
        );
    }

    return result;
}

std::string HomeAssistant::param_state_topic(const std::string& key) const {
    return base_topic + "/param/" + key + "/state";
}

std::string HomeAssistant::param_discovery_topic(const std::string& key) const {
    return std::string(DISCOVERY_PREFIX) +
           "/number/" + device_id + "/" + key + "/config";
}

std::set<std::string> HomeAssistant::all_param_keys() const {
    std::set<std::string> keys;

    JsonDocument modes;
    if (deserializeJson(modes, controller.led_strip.get_all_modes_json())) {
        return keys;
    }

    for (JsonObjectConst mode : modes.as<JsonArrayConst>()) {
        for (JsonObjectConst param : mode["params"].as<JsonArrayConst>()) {
            const std::string key = std::string(param["key"] | "");
            if (!key.empty()) keys.insert(key);
        }
    }

    return keys;
}

void HomeAssistant::load_creds() {
    mqtt_host = controller.nvs.read<std::string>(id, "host", std::string{});
    mqtt_port = controller.nvs.read<uint16_t>(id, "port", 1883);
    mqtt_user = controller.nvs.read<std::string>(id, "user", std::string{});
    mqtt_pass = controller.nvs.read<std::string>(id, "pass", std::string{});

    if (!mqtt_host.empty()) {
        provisioned = true;
        controller.nvs.reset_ns(LEGACY_NVS_NAMESPACE);
        return;
    }

    const std::string legacy_host = controller.nvs.read<std::string>(
        LEGACY_NVS_NAMESPACE,
        "host",
        std::string{}
    );

    if (legacy_host.empty()) {
        provisioned = false;
        return;
    }

    mqtt_host = legacy_host;
    mqtt_port = controller.nvs.read<uint16_t>(
        LEGACY_NVS_NAMESPACE,
        "port",
        1883
    );
    mqtt_user = controller.nvs.read<std::string>(
        LEGACY_NVS_NAMESPACE,
        "user",
        std::string{}
    );
    mqtt_pass = controller.nvs.read<std::string>(
        LEGACY_NVS_NAMESPACE,
        "pass",
        std::string{}
    );
    provisioned = true;

    save_creds();
    controller.nvs.reset_ns(LEGACY_NVS_NAMESPACE);
}

void HomeAssistant::save_creds() const {
    controller.nvs.write<std::string>(id, "host", mqtt_host);
    controller.nvs.write<uint16_t>(id, "port", mqtt_port);
    controller.nvs.write<std::string>(id, "user", mqtt_user);
    controller.nvs.write<std::string>(id, "pass", mqtt_pass);
}
