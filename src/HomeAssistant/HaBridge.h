// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/src/HomeAssistant/HaBridge.h
// A template over the MQTT client, the led and the Os, so tests/unit/host_ha_discovery.cpp runs
// this exact code on the host with fakes.
#pragma once

#include <ArduinoJson.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <XeWeCore/Utils/Color.h>

#include "HaDiscovery.h"

namespace ha {

// a JSON value of the led's modes JSON written back as JSON text (`null` when absent)
inline std::string json_value(JsonVariantConst value) {
    std::string out;
    serializeJson(value, out);
    return out;
}

template <class MqttT, class LedT, class OsT> class Bridge {
public:
    // origin: passed to the led's sync calls (the module itself); sw_version: discovery
    // `sw_version`
    Bridge(MqttT& mqtt_ref, LedT& led_ref, OsT& os_ref, const void* origin_ptr,
        const char* sw_version_text)
        : mqtt(mqtt_ref), led(led_ref), os(os_ref), origin(origin_ptr),
          sw_version(sw_version_text) {}

    Topics topics;

    // right after the MQTT connection is up: online, discovery, current state, the running mode's
    // parameters, then the command subscriptions
    void announce() {
        mqtt.publish(topics.avail.c_str(), "online", true);
        publish_light_discovery();
        publish_mode_discovery();
        publish_light_state();
        publish_mode_state();
        reconcile_params();

        mqtt.subscribe(topics.light_cmd.c_str());
        mqtt.subscribe(topics.mode_cmd.c_str());
        mqtt.subscribe(param_subscription(topics).c_str());
    }

    // before a deliberate disconnect: empty retained discovery and state (HA drops the
    // entities), then offline
    void retract() {
        clear_all_retained();
        mqtt.publish(topics.avail.c_str(), "offline", true);
        mqtt.loop();
    }

    // ---- inbound (HA -> device)
    void on_message(char* topic, uint8_t* payload, unsigned int length) {
        // copies first: PubSubClient reuses its buffer for the publishes made from inside this
        // callback (the led echo)
        const std::string t(topic);
        const std::string msg(reinterpret_cast<char*>(payload), length);

        if (t == topics.light_cmd) {
            handle_light_command(msg);
            return;
        }
        if (t == topics.mode_cmd) {
            // the select carries the mode NAME; map it back to an id
            JsonDocument modes;
            if (deserializeJson(modes, led.get_all_modes_json()))
                return;
            for (JsonObjectConst m : modes.as<JsonArrayConst>()) {
                if (msg == (m["name"] | "")) {
                    led.sync_mode((uint8_t)(m["id"] | 0), origin);
                    break;
                }
            }
            return;
        }
        std::string key;
        if (param_key_from_topic(topics, t, key)) {
            const long value = strtol(msg.c_str(), nullptr, 10);
            led.set_mode_param(key, (uint16_t)value); // the led clamps
            publish_param_state(key, led.get_current_mode_param(key));
        }
    }

    void handle_light_command(const std::string& json) {
        JsonDocument doc;
        if (deserializeJson(doc, json))
            return;

        // order: state, brightness, colour; each change is echoed back to HA by the listener
        if (doc["state"].is<const char*>()) {
            led.sync_state(std::strcmp(doc["state"] | "", "ON") == 0, origin);
        }
        if (doc["brightness"].is<int>()) {
            const int b = doc["brightness"];
            led.sync_brightness((uint8_t)clamp_like_constrain(b, 0, 255), origin);
        }
        if (doc["color"].is<JsonObjectConst>()) {
            JsonObjectConst c = doc["color"];
            // HA hs: h 0-360, s 0-100, turned into a full-value colour; brightness travels apart
            std::array<uint8_t, 3> hsv = led.get_hsv(); // an absent field keeps the current value
            if (!c["h"].isNull())
                hsv[0] = deg_to_hue8(c["h"].as<float>());
            if (!c["s"].isNull())
                hsv[1] = pct_to_sat8(c["s"].as<float>());
            hsv[2] = 255;
            led.sync_color(xewe::color::hsv_to_rgb(hsv), origin);
        }
    }

    // ---- outbound (device -> HA)
    void publish_light_discovery() {
        if (!mqtt.connected())
            return;
        const std::string payload = light_discovery_json(topics, device_json());
        const bool ok = mqtt.publish(topics.light_discovery.c_str(), payload.c_str(), true);
        if (!ok)
            os.serial.print("[HomeAssistant] light discovery FAILED (raise MQTT_BUFFER_SIZE?)");
    }

    void publish_mode_discovery() {
        if (!mqtt.connected())
            return;
        std::vector<std::string> names;
        JsonDocument modes;
        if (!deserializeJson(modes, led.get_all_modes_json())) {
            for (JsonObjectConst m : modes.as<JsonArrayConst>())
                names.emplace_back(m["name"] | "");
        }
        const std::string payload = mode_discovery_json(topics, names, device_json());
        mqtt.publish(topics.mode_discovery.c_str(), payload.c_str(), true);
    }

    void publish_light_state() {
        if (!mqtt.connected())
            return;
        const std::array<uint8_t, 3> hsv = led.get_hsv();
        const std::string payload =
            light_state_json(led.get_state(), led.get_brightness(), hsv[0], hsv[1]);
        mqtt.publish(topics.light_state.c_str(), payload.c_str(), true);
    }

    void publish_mode_state() {
        if (!mqtt.connected())
            return;
        const std::string name(led.get_current_mode_name());
        mqtt.publish(topics.mode_state.c_str(), name.c_str(), true);
    }

    // publish the running mode's parameters, clear the ones of the previous mode
    void reconcile_params() {
        if (!mqtt.connected())
            return;

        JsonDocument modes;
        if (deserializeJson(modes, led.get_all_modes_json()))
            return;

        const uint8_t cur = led.get_current_mode_id();
        std::set<std::string> new_keys;

        for (JsonObjectConst m : modes.as<JsonArrayConst>()) {
            if ((uint8_t)(m["id"] | 0) != cur)
                continue;
            for (JsonObjectConst p : m["params"].as<JsonArrayConst>()) {
                const std::string key = std::string(p["key"] | "");
                if (key.empty())
                    continue;
                new_keys.insert(key);
                publish_param_discovery(p);
                publish_param_state(key, (uint16_t)(p["value"] | 0));
            }
        }

        // clear the number entities that belonged to the previous mode only
        for (const auto& k : published_param_keys)
            if (!new_keys.count(k))
                clear_param(k);
        published_param_keys = std::move(new_keys);
    }

    void publish_param_discovery(JsonObjectConst p) {
        if (!mqtt.connected())
            return;
        const std::string key = std::string(p["key"] | "");
        if (key.empty())
            return;

        // type 'a' (advanced) goes to the device's configuration category
        const char* type = p["type"] | "b";
        const std::string payload =
            param_discovery_json(topics, key, json_value(p["display_name"]), json_value(p["min"]),
                json_value(p["max"]), json_value(p["step"]), type[0] == 'a', device_json());
        mqtt.publish(param_discovery_topic(topics, key).c_str(), payload.c_str(), true);
    }

    void publish_param_state(const std::string& key, uint16_t value) {
        if (!mqtt.connected())
            return;
        mqtt.publish(param_state_topic(topics, key).c_str(), std::to_string(value).c_str(), true);
    }

    void clear_param(const std::string& key) {
        // an empty retained payload deletes the retained message, so HA drops the entity
        mqtt.publish(param_discovery_topic(topics, key).c_str(), "", true);
        mqtt.publish(param_state_topic(topics, key).c_str(), "", true);
    }

    void clear_all_retained() {
        mqtt.publish(topics.light_discovery.c_str(), "", true);
        mqtt.publish(topics.light_state.c_str(), "", true);
        mqtt.publish(topics.mode_discovery.c_str(), "", true);
        mqtt.publish(topics.mode_state.c_str(), "", true);
        for (const auto& k : published_param_keys)
            clear_param(k);
        published_param_keys.clear();
    }

    std::string device_json() const {
        return ha::device_json(topics, os.system.get_device_name(), sw_version);
    }

private:
    MqttT& mqtt;
    LedT& led;
    OsT& os;
    const void* origin;
    const char* sw_version;

    std::set<std::string> published_param_keys; // to clear on mode change / deprovision
};

} // namespace ha
