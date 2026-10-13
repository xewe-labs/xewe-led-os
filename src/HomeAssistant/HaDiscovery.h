// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/src/HomeAssistant/HaDiscovery.h
//
// MQTT topic names, Home Assistant discovery and state payloads, and the colour conversions of
// the Home Assistant module. Standard library only, so tests/unit/host_ha_discovery.cpp compiles
// it on the host. The payloads are byte for byte what ArduinoJson 7 writes for the same documents
// (key order and string escaping included), and the topic layout and unique ids stay fixed, so an
// existing Home Assistant installation and the HACS integration keep their entities.
#pragma once

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

namespace ha {

inline constexpr const char* DISCOVERY_PREFIX = "homeassistant";

// `parts` joined into one string, allocated once
inline std::string concat(std::initializer_list<std::string_view> parts) {
    std::size_t size = 0;
    for (const std::string_view part : parts)
        size += part.size();
    std::string out;
    out.reserve(size);
    for (const std::string_view part : parts)
        out.append(part.data(), part.size());
    return out;
}

struct Topics {
    std::string device_id; // xewe_led_os_<mac>
    std::string base; // xewe_led_os/<device_id>
    std::string avail; // <base>/avail: online / offline (LWT), retained
    std::string light_cmd; // <base>/light/set: JSON from HA
    std::string light_state; // <base>/light/state: JSON, retained
    std::string light_discovery; // homeassistant/light/<device_id>/config
    std::string mode_cmd; // <base>/mode/set: mode name
    std::string mode_state; // <base>/mode/state: mode name, retained
    std::string mode_discovery; // homeassistant/select/<device_id>/config
};

// "AA:BB:CC:DD:EE:FF" -> "aabbccddeeff" (only ':' is dropped)
inline std::string mac_hex(const std::string& mac) {
    std::string out;
    out.reserve(12);
    for (const char ch : mac) {
        if (ch == ':')
            continue;
        out += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return out;
}

inline Topics make_topics(const std::string& mac_hex_str) {
    Topics t;
    t.device_id = concat({"xewe_led_os_", mac_hex_str});
    t.base = concat({"xewe_led_os/", t.device_id});
    t.avail = concat({t.base, "/avail"});
    t.light_cmd = concat({t.base, "/light/set"});
    t.light_state = concat({t.base, "/light/state"});
    t.light_discovery = concat({DISCOVERY_PREFIX, "/light/", t.device_id, "/config"});
    t.mode_cmd = concat({t.base, "/mode/set"});
    t.mode_state = concat({t.base, "/mode/state"});
    t.mode_discovery = concat({DISCOVERY_PREFIX, "/select/", t.device_id, "/config"});
    return t;
}

inline std::string param_cmd_topic(const Topics& t, const std::string& key) {
    return concat({t.base, "/param/", key, "/set"});
}
inline std::string param_state_topic(const Topics& t, const std::string& key) {
    return concat({t.base, "/param/", key, "/state"});
}
inline std::string param_subscription(const Topics& t) {
    return concat({t.base, "/param/+/set"});
}
inline std::string param_discovery_topic(const Topics& t, const std::string& key) {
    return concat({DISCOVERY_PREFIX, "/number/", t.device_id, "/", key, "/config"});
}

// <base>/param/<key>/set -> key; false for any other topic (also an empty key). A key with a '/'
// is refused too; the `+` subscription never delivers one.
inline bool param_key_from_topic(const Topics& t, const std::string& topic, std::string& key) {
    const std::string prefix = concat({t.base, "/param/"});
    const std::string suffix = "/set";
    if (topic.size() <= prefix.size() + suffix.size())
        return false;
    if (topic.compare(0, prefix.size(), prefix) != 0)
        return false;
    if (topic.compare(topic.size() - suffix.size(), suffix.size(), suffix) != 0)
        return false;
    key = topic.substr(prefix.size(), topic.size() - prefix.size() - suffix.size());
    return key.find('/') == std::string::npos;
}

// A JSON string as ArduinoJson 7 writes it: '"', '\\', \b, \f, \n, \r and \t are escaped, NUL
// becomes \u0000, every other byte (other control bytes, '/', UTF-8) is copied as is.
inline std::string json_string(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 2);
    out += '"';
    for (const char c : text) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        case '\0':
            out += "\\u0000";
            break;
        default:
            out += c;
        }
    }
    out += '"';
    return out;
}

// "device" object shared by every entity (one HA device per LED controller)
inline std::string device_json(
    const Topics& t, const std::string& name, const std::string& sw_version) {
    return concat({"{\"identifiers\":[", json_string(t.device_id), "],\"name\":", json_string(name),
        ",\"manufacturer\":\"XeWe\",\"model\":\"LED\",\"sw_version\":", json_string(sw_version),
        "}"});
}

// JSON-schema light with hs colour; "name":null makes HA use the device name
inline std::string light_discovery_json(const Topics& t, const std::string& device) {
    return concat(
        {"{\"~\":", json_string(t.base), ",\"schema\":\"json\",\"name\":null,\"unique_id\":",
            json_string(concat({t.device_id, "_light"})),
            ",\"command_topic\":\"~/light/set\",\"state_topic\":\"~/light/state\""
            ",\"availability_topic\":\"~/avail\",\"payload_available\":\"online\""
            ",\"payload_not_available\":\"offline\",\"brightness\":true,\"color_mode\":true"
            ",\"supported_color_modes\":[\"hs\"],\"device\":",
            device, "}"});
}

// select entity; options are the mode names in the order of the led's modes JSON
inline std::string mode_discovery_json(
    const Topics& t, const std::vector<std::string>& mode_names, const std::string& device) {
    std::string options;
    for (const std::string& n : mode_names) {
        if (!options.empty())
            options += ',';
        options += json_string(n);
    }
    return concat({"{\"~\":", json_string(t.base),
        ",\"name\":\"Mode\",\"unique_id\":", json_string(concat({t.device_id, "_mode"})),
        ",\"command_topic\":\"~/mode/set\",\"state_topic\":\"~/mode/state\""
        ",\"availability_topic\":\"~/avail\",\"options\":[",
        options, "],\"device\":", device, "}"});
}

// One number entity per parameter of the current mode. name, min, max and step are JSON values
// copied from the led's modes JSON entry as they are written there (`null` when absent); a
// parameter of type 'a' (advanced) goes to the device's configuration category.
inline std::string param_discovery_json(const Topics& t, const std::string& key,
    const std::string& name_json, const std::string& min_json, const std::string& max_json,
    const std::string& step_json, bool config_category, const std::string& device) {
    return concat({"{\"~\":", json_string(t.base), ",\"name\":", name_json,
        ",\"unique_id\":", json_string(concat({t.device_id, "_", key})),
        ",\"command_topic\":", json_string(concat({"~/param/", key, "/set"})),
        ",\"state_topic\":", json_string(concat({"~/param/", key, "/state"})),
        ",\"availability_topic\":\"~/avail\",\"min\":", min_json, ",\"max\":", max_json,
        ",\"step\":", step_json, config_category ? ",\"entity_category\":\"config\"" : "",
        ",\"device\":", device, "}"});
}

// led hue byte -> HA degrees, led saturation byte -> HA percent (rounded half away from zero)
inline long hue8_to_deg(uint8_t hue8) {
    return lroundf(hue8 / 255.0f * 360.0f);
}
inline long sat8_to_pct(uint8_t sat8) {
    return lroundf(sat8 / 255.0f * 100.0f);
}

// The comparison of Arduino's constrain(): NaN fails both tests and is returned unclamped.
template <class T> inline T clamp_like_constrain(T value, T low, T high) {
    return value < low ? low : (value > high ? high : value);
}

// HA degrees -> led hue byte, HA percent -> led saturation byte, clamped first
inline uint8_t deg_to_hue8(float deg) {
    return static_cast<uint8_t>(lroundf(clamp_like_constrain(deg, 0.0f, 360.0f) / 360.0f * 255.0f));
}
inline uint8_t pct_to_sat8(float pct) {
    return static_cast<uint8_t>(lroundf(clamp_like_constrain(pct, 0.0f, 100.0f) / 100.0f * 255.0f));
}

// {"state":"ON","brightness":0-255,"color_mode":"hs","color":{"h":0-360,"s":0-100}}
inline std::string light_state_json(bool on, uint8_t brightness, uint8_t hue8, uint8_t sat8) {
    char buf[112];
    std::snprintf(buf, sizeof(buf),
        "{\"state\":\"%s\",\"brightness\":%u,\"color_mode\":\"hs\",\"color\":{\"h\":%ld,\"s\":%ld}"
        "}",
        on ? "ON" : "OFF", static_cast<unsigned>(brightness), hue8_to_deg(hue8), sat8_to_pct(sat8));
    return buf;
}

// mDNS host name while unprovisioned (HACS discovery): xewe-led-os-<last 6 hex of the MAC>
inline std::string mdns_host(const std::string& mac_hex_str) {
    const std::string_view mac = mac_hex_str;
    return concat({"xewe-led-os-", mac.size() > 6 ? mac.substr(mac.size() - 6) : mac});
}

} // namespace ha
