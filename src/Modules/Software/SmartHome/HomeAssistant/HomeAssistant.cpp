// src/Interfaces/Software/HomeAssistant/HomeAssistant.cpp

#include "HomeAssistant.h"
#include "../../../SystemController/SystemController.h"

#include <WebServer.h>
#include <ESPmDNS.h>
#include <cmath>
#include <cctype>
#include <cstdlib>

namespace {
// Fan-out flags that touch every interface (including this one, so the
// confirmed state echoes straight back to HA). Drop-in scalable: no hardcoded
// index, unlike HomeKit's {1,1,1,0,1}.
std::array<uint8_t, INTERFACE_COUNT> all_flags() {
    std::array<uint8_t, INTERFACE_COUNT> f;
    f.fill(1);
    return f;
}
}  // namespace

HomeAssistant::HomeAssistant(SystemController& controller)
    : Interface(controller,
                /* module_name         */ "Home_Assistant",
                /* module_description  */ "Allows to control the LED from Home Assistant over MQTT auto-discovery.\nREQUIRES an MQTT broker (the Home Assistant Mosquitto add-on works).",
                /* nvs_key             */ "ha",
                /* requires_init_setup */ true,
                /* can_be_disabled     */ true,
                /* has_cli_cmds        */ true)
{}

// ---- lifecycle --------------------------------------------------------------
void HomeAssistant::begin_routines_required(const ModuleConfig& cfg) {
    add_requirement(controller.wifi);
    add_requirement(controller.web_interface);
    add_requirement(controller.led_strip);

    build_topics();

    // Reuse the WebInterface's HTTP server for the credential handoff instead
    // of standing up a second server. WebInterface begins before this module
    // (interfaces vector order), so the server already exists here.
    WebServer& server = controller.web_interface.get_server();
    server.on("/provision",   HTTP_POST, [this] { handle_provision(); });
    server.on("/deprovision", HTTP_POST, [this] { handle_deprovision(); });
}

void HomeAssistant::begin_routines_init(const ModuleConfig& cfg) {
    if (is_disabled()) return;

    mqtt.setBufferSize(MQTT_BUFFER_SIZE);   // discovery JSON exceeds the 256B default
    mqtt.setCallback([this](char* t, uint8_t* p, unsigned int l) { on_message(t, p, l); });

    load_creds();
    if (provisioned) {
        mqtt.setServer(mqtt_host.c_str(), mqtt_port);
        last_reconnect_ms = 0;
        connect();
    }

    WebServer& server = controller.web_interface.get_server();

    bool mdns_started = false;
    std::string mdns_host;

    // Mirror the known-good sketch: advertise a pairing/discovery service while
    // waiting for broker credentials. This is what makes Home Assistant see the
    // device before MQTT credentials have been posted.
    if (!provisioned) {
        mdns_host = "xewe-led-os-";
        if (mac_hex.size() > 6) mdns_host += mac_hex.substr(mac_hex.size() - 6);
        else                    mdns_host += mac_hex;

        std::string device_name = controller.system.get_device_name();

        if (MDNS.begin(mdns_host.c_str())) {
            MDNS.addService("_xewe-led-os", "_tcp", 80);
            MDNS.addServiceTxt("_xewe-led-os", "_tcp", "mac", mac_hex.c_str());
            MDNS.addServiceTxt("_xewe-led-os", "_tcp", "fw", SW_VERSION);
            MDNS.addServiceTxt("_xewe-led-os", "_tcp", "name", device_name.c_str());
            mdns_started = true;
        }
    }

    controller.serial_port.print(
        "\nHome Assistant setup required.\n"
        "\n"
        "Discovery mode is active. Open Home Assistant > Settings > Devices & Services.\n"
        "If the discovered card does not appear, provision manually by POSTing MQTT broker credentials to:\n"
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
        "Setup continues automatically after MQTT connects and discovery is published.\n"
        "To abort and disable Home Assistant, press (x): "
    );

    if (mdns_started) {
        controller.serial_port.print("\n[HomeAssistant] mDNS discovery active: ");
        controller.serial_port.print(mdns_host.c_str());
        controller.serial_port.print("._xewe-led-os._tcp.local");
    } else if (!provisioned) {
        controller.serial_port.print("\n[HomeAssistant] mDNS discovery FAILED; /provision is still available manually.");
    }

    uint32_t last_status_ms = 0;

    // HA MQTT discovery has no HomeKit-style final pairing callback. This blocks
    // until broker credentials are received, MQTT connects, and connect()
    // publishes retained discovery + retained current state.
    while (!provisioned || !mqtt.connected()) {
        server.handleClient();
        controller.serial_port.loop();

        if (controller.serial_port.has_line()) {
            std::string input = controller.serial_port.read_line();
            if (!input.empty() && input[0] == 'x') {
                if (mdns_started) MDNS.end();
                disable(false, true);  // reset with no verbose and restart
                return;
            }
            controller.serial_port.print("\n(x)?: ");
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
        if (now - last_status_ms >= 3000) {
            last_status_ms = now;
            if (!provisioned) {
                controller.serial_port.print("\n[HomeAssistant] waiting for Home Assistant discovery card or /provision...");
            } else if (!mqtt.connected()) {
                controller.serial_port.print("\n[HomeAssistant] provisioned; waiting for MQTT connection...");
            }
        }

        delay(5);
    }

    if (mdns_started) MDNS.end();
    mqtt.loop();
    controller.serial_port.print(
        "\n[HomeAssistant] paired: MQTT connected, discovery published, state retained.\n"
        "Device should now appear in Home Assistant via MQTT discovery.\n"
    );
}

void HomeAssistant::begin_routines_common(const ModuleConfig& cfg) {
    if (is_disabled()) return;

    mqtt.setBufferSize(MQTT_BUFFER_SIZE);   // discovery JSON exceeds the 256B default
    mqtt.setCallback([this](char* t, uint8_t* p, unsigned int l) { on_message(t, p, l); });

    load_creds();
    if (provisioned) {
        mqtt.setServer(mqtt_host.c_str(), mqtt_port);
        last_reconnect_ms = 0;
        connect();
    }
}

void HomeAssistant::loop() {
    if (is_disabled() || !provisioned) return;
    if (controller.wifi.is_disconnected()) return;

    if (!mqtt.connected()) connect();
    else                   mqtt.loop();
}

void HomeAssistant::reset(const bool verbose, const bool do_restart, const bool keep_enabled) {
    // Clear retained discovery so HA drops the entities before we forget the
    // broker (Module::reset wipes the whole nvs_key namespace, creds included).
    if (mqtt.connected()) {
        clear_all_retained();
        mqtt.publish(avail_topic.c_str(), "offline", true);
        mqtt.loop();
        delay(100);
        mqtt.disconnect();
    }
    provisioned = false;
    Module::reset(verbose, do_restart, keep_enabled);
}

std::string HomeAssistant::status(const bool verbose) const {
    if (is_disabled()) return std::string("Home Assistant module disabled");

    std::string line = "Home Assistant: ";
    if (!provisioned) {
        line += "not provisioned (POST /provision to pair)";
    } else {
        line += "broker " + mqtt_host + ":" + std::to_string(mqtt_port);
        line += mqtt.connected() ? " [connected]" : " [disconnected]";
    }
    controller.serial_port.print(line);
    return Module::status(verbose);
}

// ---- Interface sync_ hooks (device-originated changes -> HA) -----------------
// Called by SystemController when ANY source (physical button, HomeKit, Alexa,
// scheduler, ...) changes state. Publishing here is the whole point of MQTT:
// HA sees local changes immediately instead of polling.
void HomeAssistant::sync_color(std::array<uint8_t,3> color) {
    if (is_disabled()) return;
    publish_light_state();
}

void HomeAssistant::sync_brightness(uint8_t brightness) {
    if (is_disabled()) return;
    publish_light_state();
}

void HomeAssistant::sync_state(uint8_t state) {
    if (is_disabled()) return;
    publish_light_state();
}

void HomeAssistant::sync_mode(uint8_t mode) {
    if (is_disabled()) return;
    publish_mode_state();
    reconcile_params();   // swap the visible number entities to the new mode's params
    publish_light_state();
}

void HomeAssistant::sync_length(uint16_t length) {
    if (is_disabled()) return;  // not represented in the HA model
}

void HomeAssistant::sync_all(std::array<uint8_t,3> color,
                             uint8_t brightness,
                             uint8_t state,
                             uint8_t mode,
                             uint16_t length) {
    if (is_disabled()) return;
    sync_state(state);
    sync_brightness(brightness);
    sync_color(color);
    sync_mode(mode);
}

void HomeAssistant::sync_param(std::string_view key, uint16_t value) {
    if (is_disabled()) return;
    publish_param_state(std::string(key), value);
}

// ---- connection -------------------------------------------------------------
void HomeAssistant::connect() {
    if (mqtt.connected()) return;
    if (!provisioned || mqtt_host.empty()) return;

    if (last_reconnect_ms != 0 && millis() - last_reconnect_ms < RECONNECT_INTERVAL_MS) {
        return;
    }
    last_reconnect_ms = millis();

    const char* user = mqtt_user.empty() ? nullptr : mqtt_user.c_str();
    const char* pass = mqtt_pass.empty() ? nullptr : mqtt_pass.c_str();

    // LWT: the broker publishes "offline" to avail_topic on ungraceful drop.
    if (!mqtt.connect(device_id.c_str(), user, pass,
                      avail_topic.c_str(), 1, true, "offline")) {
        controller.serial_port.print("[HomeAssistant] MQTT connect failed, will retry");
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
    controller.serial_port.print("[HomeAssistant] MQTT connected; discovery published");
}

// ---- inbound (HA -> device) -------------------------------------------------
void HomeAssistant::on_message(char* topic, uint8_t* payload, unsigned int length) {
    std::string t(topic);
    std::string msg(reinterpret_cast<char*>(payload), length);

    if (t == light_cmd_topic) {
        handle_light_command(msg);
        return;
    }
    if (t == mode_cmd_topic) {
        // The select carries the mode NAME; map it back to an id.
        JsonDocument modes;
        if (deserializeJson(modes, controller.led_strip.get_all_modes_json())) return;
        for (JsonObjectConst m : modes.as<JsonArrayConst>()) {
            if (msg == (m["name"] | "")) {
                controller.sync_mode((uint8_t)(m["id"] | 0), all_flags());
                break;
            }
        }
        return;
    }
    const std::string prefix = base_topic + "/param/";
    const std::string suffix = "/set";
    if (t.rfind(prefix, 0) == 0 &&
        t.size() > prefix.size() + suffix.size() &&
        t.compare(t.size() - suffix.size(), suffix.size(), suffix) == 0) {
        std::string key = t.substr(prefix.size(), t.size() - prefix.size() - suffix.size());
        long value = strtol(msg.c_str(), nullptr, 10);
        controller.led_strip.set_mode_param(key, (uint16_t)value);  // LedStrip clamps
        publish_param_state(key, controller.led_strip.get_current_mode_param(key));
    }
}

void HomeAssistant::handle_light_command(const std::string& json) {
    JsonDocument doc;
    if (deserializeJson(doc, json)) return;

    auto flags = all_flags();

    if (doc["state"].is<const char*>()) {
        controller.sync_state(std::string(doc["state"] | "") == "ON" ? 1 : 0, flags);
    }
    if (doc["brightness"].is<int>()) {
        int b = doc["brightness"];
        controller.sync_brightness((uint8_t)constrain(b, 0, 255), flags);
    }
    if (doc["color"].is<JsonObjectConst>()) {
        JsonObjectConst c = doc["color"];
        // HA HS: h 0-360, s 0-100. Convert to a full-value RGB; brightness is
        // carried separately (mirrors HomeKit::NeoPixel_RGB::update()).
        std::array<uint8_t,3> hsv = controller.led_strip.get_hsv();  // keep current if a field is absent
        if (!c["h"].isNull()) hsv[0] = (uint8_t)lroundf(constrain(c["h"].as<float>(), 0.0f, 360.0f) / 360.0f * 255.0f);
        if (!c["s"].isNull()) hsv[1] = (uint8_t)lroundf(constrain(c["s"].as<float>(), 0.0f, 100.0f) / 100.0f * 255.0f);
        hsv[2] = 255;
        controller.sync_color(hsv_to_rgb(hsv), flags);
    }
    // The controller.sync_* fan-out above includes this interface, so our own
    // sync_ hooks already echoed the confirmed state back to HA.
}

void HomeAssistant::handle_provision() {
    WebServer& server = controller.web_interface.get_server();
    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain"))) {
        server.send(400, "application/json", "{\"error\":\"bad_json\"}");
        return;
    }
    set_broker(std::string(doc["host"] | ""),
               (uint16_t)(doc["port"] | 1883),
               std::string(doc["user"] | ""),
               std::string(doc["pass"] | ""));
    server.send(200, "application/json", "{\"ok\":true}");
    controller.serial_port.print("[HomeAssistant] provisioned via /provision");
}

void HomeAssistant::handle_deprovision() {
    WebServer& server = controller.web_interface.get_server();
    server.send(200, "application/json", "{\"ok\":true}");
    controller.serial_port.print("[HomeAssistant] /deprovision: clearing creds + retained discovery");
    clear_broker();
}

// ---- provisioning state -----------------------------------------------------
void HomeAssistant::set_broker(const std::string& host, uint16_t port,
                               const std::string& user, const std::string& pass) {
    mqtt_host = host;
    mqtt_port = port;
    mqtt_user = user;
    mqtt_pass = pass;
    provisioned = !mqtt_host.empty();
    save_creds();
    if (provisioned) {
        mqtt.setServer(mqtt_host.c_str(), mqtt_port);
        last_reconnect_ms = 0;   // connect immediately
        connect();
    }
}

void HomeAssistant::clear_broker() {
    if (mqtt.connected()) {
        clear_all_retained();
        mqtt.publish(avail_topic.c_str(), "offline", true);
        mqtt.loop();
        delay(100);
        mqtt.disconnect();
    }
    controller.nvs.remove(nvs_key, "host");
    controller.nvs.remove(nvs_key, "port");
    controller.nvs.remove(nvs_key, "user");
    controller.nvs.remove(nvs_key, "pass");
    mqtt_host.clear();
    mqtt_user.clear();
    mqtt_pass.clear();
    mqtt_port = 1883;
    provisioned = false;
}

// ---- outbound (device -> HA) ------------------------------------------------
void HomeAssistant::add_device(JsonDocument& doc) const {
    JsonObject dev = doc["device"].to<JsonObject>();
    dev["identifiers"][0] = device_id;
    dev["name"]           = controller.system.get_device_name();
    dev["manufacturer"]   = "XeWe";
    dev["model"]          = "LED";
    dev["sw_version"]     = SW_VERSION;
}

void HomeAssistant::publish_light_discovery() {
    if (!mqtt.connected()) return;
    JsonDocument doc;
    doc["~"]                     = base_topic;
    doc["schema"]                = "json";
    doc["name"]                  = nullptr;   // inherit the device name
    doc["unique_id"]             = device_id + "_light";
    doc["command_topic"]         = "~/light/set";
    doc["state_topic"]           = "~/light/state";
    doc["availability_topic"]    = "~/avail";
    doc["payload_available"]     = "online";
    doc["payload_not_available"] = "offline";
    doc["brightness"]            = true;
    doc["color_mode"]            = true;
    doc["supported_color_modes"][0] = "hs";
    add_device(doc);

    std::string payload;
    serializeJson(doc, payload);
    bool ok = mqtt.publish(light_discovery_topic.c_str(), payload.c_str(), true);
    if (!ok) controller.serial_port.print("[HomeAssistant] light discovery FAILED (raise MQTT_BUFFER_SIZE?)");
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

    JsonArray opts = doc["options"].to<JsonArray>();
    JsonDocument modes;
    if (!deserializeJson(modes, controller.led_strip.get_all_modes_json())) {
        for (JsonObjectConst m : modes.as<JsonArrayConst>())
            opts.add(std::string(m["name"] | ""));
    }
    add_device(doc);

    std::string payload;
    serializeJson(doc, payload);
    mqtt.publish(mode_discovery_topic.c_str(), payload.c_str(), true);
}

void HomeAssistant::publish_light_state() {
    if (!mqtt.connected()) return;
    std::array<uint8_t,3> hsv = controller.led_strip.get_hsv();

    JsonDocument doc;
    doc["state"]      = controller.led_strip.get_state() ? "ON" : "OFF";
    doc["brightness"] = controller.led_strip.get_brightness();
    doc["color_mode"] = "hs";
    JsonObject color  = doc["color"].to<JsonObject>();
    color["h"]        = lroundf(hsv[0] / 255.0f * 360.0f);
    color["s"]        = lroundf(hsv[1] / 255.0f * 100.0f);

    std::string payload;
    serializeJson(doc, payload);
    mqtt.publish(light_state_topic.c_str(), payload.c_str(), true);
}

void HomeAssistant::publish_mode_state() {
    if (!mqtt.connected()) return;
    std::string name(controller.led_strip.get_current_mode_name());
    mqtt.publish(mode_state_topic.c_str(), name.c_str(), true);
}

void HomeAssistant::reconcile_params() {
    if (!mqtt.connected()) return;

    JsonDocument modes;
    if (deserializeJson(modes, controller.led_strip.get_all_modes_json())) return;

    const uint8_t cur = controller.led_strip.get_current_mode_id();
    std::set<std::string> new_keys;

    for (JsonObjectConst m : modes.as<JsonArrayConst>()) {
        if ((uint8_t)(m["id"] | 0) != cur) continue;
        for (JsonObjectConst p : m["params"].as<JsonArrayConst>()) {
            std::string key = std::string(p["key"] | "");
            if (key.empty()) continue;
            new_keys.insert(key);
            publish_param_discovery(p);
            publish_param_state(key, (uint16_t)(p["value"] | 0));
        }
    }

    // Clear number entities that belonged to the previous mode only.
    for (const auto& k : published_param_keys)
        if (!new_keys.count(k)) clear_param(k);
    published_param_keys = std::move(new_keys);
}

void HomeAssistant::publish_param_discovery(JsonObjectConst p) {
    if (!mqtt.connected()) return;
    std::string key = std::string(p["key"] | "");
    if (key.empty()) return;

    JsonDocument doc;
    doc["~"]                  = base_topic;
    doc["name"]               = p["display_name"];
    doc["unique_id"]          = device_id + "_" + key;
    doc["command_topic"]      = "~/param/" + key + "/set";
    doc["state_topic"]        = "~/param/" + key + "/state";
    doc["availability_topic"] = "~/avail";
    doc["min"]                = p["min"];
    doc["max"]                = p["max"];
    doc["step"]               = p["step"];
    // type 'a' = additional/advanced -> tuck under HA's device "Configuration".
    const char* type = p["type"] | "b";
    if (type[0] == 'a') doc["entity_category"] = "config";
    add_device(doc);

    std::string payload;
    serializeJson(doc, payload);
    mqtt.publish(param_discovery_topic(key).c_str(), payload.c_str(), true);
}

void HomeAssistant::publish_param_state(const std::string& key, uint16_t value) {
    if (!mqtt.connected()) return;
    mqtt.publish(param_state_topic(key).c_str(), std::to_string(value).c_str(), true);
}

void HomeAssistant::clear_param(const std::string& key) {
    // Empty retained payload deletes the retained message -> HA drops the entity.
    mqtt.publish(param_discovery_topic(key).c_str(), "", true);
    mqtt.publish(param_state_topic(key).c_str(), "", true);
}

void HomeAssistant::clear_all_retained() {
    mqtt.publish(light_discovery_topic.c_str(), "", true);
    mqtt.publish(light_state_topic.c_str(), "", true);
    mqtt.publish(mode_discovery_topic.c_str(), "", true);
    mqtt.publish(mode_state_topic.c_str(), "", true);
    for (const auto& k : published_param_keys) clear_param(k);
    published_param_keys.clear();
}

// ---- helpers ----------------------------------------------------------------
void HomeAssistant::build_topics() {
    mac_hex   = mac_to_hex();
    device_id = "xewe_led_os_" + mac_hex;
    base_topic = "xewe_led_os/" + device_id;

    avail_topic           = base_topic + "/avail";
    light_cmd_topic       = base_topic + "/light/set";
    light_state_topic     = base_topic + "/light/state";
    light_discovery_topic = std::string(DISCOVERY_PREFIX) + "/light/" + device_id + "/config";
    mode_cmd_topic        = base_topic + "/mode/set";
    mode_state_topic      = base_topic + "/mode/state";
    mode_discovery_topic  = std::string(DISCOVERY_PREFIX) + "/select/" + device_id + "/config";
}

std::string HomeAssistant::mac_to_hex() const {
    std::string mac = controller.wifi.get_mac_address();  // "AA:BB:CC:DD:EE:FF"
    std::string out;
    out.reserve(12);
    for (char ch : mac) {
        if (ch == ':') continue;
        out += (char)tolower((unsigned char)ch);
    }
    return out;
}

std::string HomeAssistant::param_state_topic(const std::string& key) const {
    return base_topic + "/param/" + key + "/state";
}

std::string HomeAssistant::param_discovery_topic(const std::string& key) const {
    return std::string(DISCOVERY_PREFIX) + "/number/" + device_id + "/" + key + "/config";
}

void HomeAssistant::load_creds() {
    mqtt_host = controller.nvs.read_str(nvs_key, "host", "");
    mqtt_port = controller.nvs.read_uint16(nvs_key, "port", 1883);
    mqtt_user = controller.nvs.read_str(nvs_key, "user", "");
    mqtt_pass = controller.nvs.read_str(nvs_key, "pass", "");
    provisioned = !mqtt_host.empty();
}

void HomeAssistant::save_creds() const {
    controller.nvs.write_str(nvs_key, "host", mqtt_host);
    controller.nvs.write_uint16(nvs_key, "port", mqtt_port);
    controller.nvs.write_str(nvs_key, "user", mqtt_user);
    controller.nvs.write_str(nvs_key, "pass", mqtt_pass);
}
