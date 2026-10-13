// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/tests/unit/host_ha_discovery.cpp
//
// Host driver of src/HomeAssistant/HaDiscovery.h and HaBridge.h for test_home_assistant.py.
// 1. Prints one `name<TAB>value` line per topic, payload and conversion; the Python side compares
//    them with the 2.3.0 strings as exact text (key order and escaping included).
// 2. When ArduinoJson and XeWeCore are on the include path (build/libraries after setup):
//    - runs the 2.3.0 payload builders (ArduinoJson 7, copied from 2.3.0 HomeAssistant.cpp) against
//      the string builders over every mode parameter, many device names and the light states, and
//      prints `arduinojson<TAB>same:<checks>`, or `FAILED` with the first differences. The light
//      states: every hue x saturation byte at brightness 128 plus every brightness at three
//      colours, both states (`grid<TAB>fast`); with XEWE_TEST_HA_FULL_GRID=1 every hue x
//      saturation byte at five brightnesses, both states (`grid<TAB>full`, 655k states);
//    - runs ha::Bridge (the module's publish and command code) against a fake MQTT client, led and
//      Os, and prints each scenario's ordered event log (every publish with its topic, payload and
//      retain flag, every subscription, every led call) as a JSON array of strings.
//    It prints `arduinojson<TAB>absent` and `bridge<TAB>absent` otherwise.
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "../../src/HomeAssistant/HaDiscovery.h"

static void out(const char* name, const std::string& value) {
    std::printf("%s\t%s\n", name, value.c_str());
}

// the led's modes JSON after boot (Solid, hue 85 / sat 255 live; the others at their defaults),
// as the led module's own test pins it
static const char* const MODES_JSON =
    "[{\"id\":0,\"name\":\"Solid\",\"params\":["
    "{\"key\":\"hue\",\"display_name\":\"Hue\",\"min\":0,\"max\":255,\"step\":1,\"value\":85,"
    "\"type\":\"b\"},"
    "{\"key\":\"sat\",\"display_name\":\"Saturation\",\"min\":0,\"max\":255,\"step\":1,"
    "\"value\":255,\"type\":\"b\"}]},"
    "{\"id\":1,\"name\":\"Color Fade\",\"params\":["
    "{\"key\":\"hue\",\"display_name\":\"Hue\",\"min\":0,\"max\":255,\"step\":1,\"value\":195,"
    "\"type\":\"b\"},"
    "{\"key\":\"sat\",\"display_name\":\"Min Saturation\",\"min\":0,\"max\":245,\"step\":1,"
    "\"value\":245,\"type\":\"b\"},"
    "{\"key\":\"speed\",\"display_name\":\"Speed\",\"min\":1,\"max\":50,\"step\":1,\"value\":4,"
    "\"type\":\"a\"},"
    "{\"key\":\"fire_step\",\"display_name\":\"Density\",\"min\":1,\"max\":255,\"step\":1,"
    "\"value\":20,\"type\":\"a\"},"
    "{\"key\":\"h_gap\",\"display_name\":\"Color Variance\",\"min\":0,\"max\":65535,\"step\":100,"
    "\"value\":15000,\"type\":\"a\"},"
    "{\"key\":\"min_bright\",\"display_name\":\"Depth\",\"min\":0,\"max\":255,\"step\":1,"
    "\"value\":150,\"type\":\"a\"}]},"
    "{\"id\":2,\"name\":\"Color Fade Two Zone\",\"params\":["
    "{\"key\":\"hue\",\"display_name\":\"Hue A\",\"min\":0,\"max\":255,\"step\":1,\"value\":81,"
    "\"type\":\"b\"},"
    "{\"key\":\"hue_b\",\"display_name\":\"Hue B\",\"min\":0,\"max\":255,\"step\":1,"
    "\"value\":225,\"type\":\"b\"},"
    "{\"key\":\"blend\",\"display_name\":\"Blend\",\"min\":2,\"max\":255,\"step\":1,"
    "\"value\":150,\"type\":\"a\"},"
    "{\"key\":\"speed\",\"display_name\":\"Speed\",\"min\":1,\"max\":50,\"step\":1,\"value\":3,"
    "\"type\":\"a\"},"
    "{\"key\":\"fire_step\",\"display_name\":\"Density\",\"min\":1,\"max\":255,\"step\":1,"
    "\"value\":10,\"type\":\"a\"},"
    "{\"key\":\"min_bright\",\"display_name\":\"Depth\",\"min\":0,\"max\":255,\"step\":1,"
    "\"value\":245,\"type\":\"a\"},"
    "{\"key\":\"min_sat\",\"display_name\":\"Min Sat\",\"min\":0,\"max\":255,\"step\":1,"
    "\"value\":215,\"type\":\"a\"}]},"
    "{\"id\":3,\"name\":\"Brightness Fade\",\"params\":["
    "{\"key\":\"hue\",\"display_name\":\"Hue\",\"min\":0,\"max\":255,\"step\":1,\"value\":0,"
    "\"type\":\"b\"},"
    "{\"key\":\"sat\",\"display_name\":\"Saturation\",\"min\":0,\"max\":255,\"step\":1,"
    "\"value\":255,\"type\":\"b\"},"
    "{\"key\":\"speed\",\"display_name\":\"Speed\",\"min\":1,\"max\":50,\"step\":1,\"value\":5,"
    "\"type\":\"a\"},"
    "{\"key\":\"noise_step\",\"display_name\":\"Density\",\"min\":1,\"max\":255,\"step\":1,"
    "\"value\":10,\"type\":\"a\"},"
    "{\"key\":\"min_bright\",\"display_name\":\"Min Brightness\",\"min\":0,\"max\":255,"
    "\"step\":1,\"value\":10,\"type\":\"a\"}]},"
    "{\"id\":4,\"name\":\"Pulse\",\"params\":["
    "{\"key\":\"hue\",\"display_name\":\"Hue\",\"min\":0,\"max\":255,\"step\":1,\"value\":0,"
    "\"type\":\"b\"},"
    "{\"key\":\"sat\",\"display_name\":\"Saturation\",\"min\":0,\"max\":255,\"step\":1,"
    "\"value\":255,\"type\":\"b\"},"
    "{\"key\":\"speed\",\"display_name\":\"Speed\",\"min\":1,\"max\":255,\"step\":1,"
    "\"value\":30,\"type\":\"a\"}]},"
    "{\"id\":5,\"name\":\"Rainbow\",\"params\":["
    "{\"key\":\"speed\",\"display_name\":\"Speed\",\"min\":1,\"max\":20,\"step\":1,\"value\":5,"
    "\"type\":\"b\"},"
    "{\"key\":\"density\",\"display_name\":\"Density\",\"min\":1,\"max\":30,\"step\":1,"
    "\"value\":10,\"type\":\"a\"}]},"
    "{\"id\":6,\"name\":\"Christmas Lights\",\"params\":["
    "{\"key\":\"density\",\"display_name\":\"Density\",\"min\":1,\"max\":10,\"step\":1,"
    "\"value\":1,\"type\":\"b\"},"
    "{\"key\":\"speed\",\"display_name\":\"Flicker\",\"min\":0,\"max\":20,\"step\":1,"
    "\"value\":5,\"type\":\"a\"}]}]";

static void print_fixed_cases() {
    const std::string mac = ha::mac_hex("AA:BB:CC:0D:EE:F1");
    const ha::Topics t = ha::make_topics(mac);
    out("mac", mac);
    out("mac_dash", ha::mac_hex("AA-BB")); // only ':' is dropped
    out("mac_empty", ha::mac_hex(""));
    out("device_id", t.device_id);
    out("base", t.base);
    out("avail", t.avail);
    out("light_cmd", t.light_cmd);
    out("light_state", t.light_state);
    out("light_discovery", t.light_discovery);
    out("mode_cmd", t.mode_cmd);
    out("mode_state", t.mode_state);
    out("mode_discovery", t.mode_discovery);
    out("param_cmd", ha::param_cmd_topic(t, "speed"));
    out("param_state", ha::param_state_topic(t, "speed"));
    out("param_sub", ha::param_subscription(t));
    out("param_discovery", ha::param_discovery_topic(t, "speed"));
    out("mdns_host", ha::mdns_host(mac));
    out("mdns_host_short", ha::mdns_host("abc"));
    out("device_id_empty_mac", ha::make_topics(ha::mac_hex("")).device_id);

    std::string key;
    out("parse_ok",
        ha::param_key_from_topic(t, t.base + "/param/min_bright/set", key) ? key : "<false>");
    key.clear();
    out("parse_state",
        ha::param_key_from_topic(t, t.base + "/param/speed/state", key) ? key : "<false>");
    out("parse_empty", ha::param_key_from_topic(t, t.base + "/param//set", key) ? key : "<false>");
    out("parse_other",
        ha::param_key_from_topic(t, "xewe_led_os/other/param/speed/set", key) ? key : "<false>");
    out("parse_nested",
        ha::param_key_from_topic(t, t.base + "/param/a/b/set", key) ? key : "<false>");

    const std::string device = ha::device_json(t, "Desk", "3.0.0");
    out("device", device);
    out("light", ha::light_discovery_json(t, device));
    out("mode",
        ha::mode_discovery_json(t,
            {"Solid", "Color Fade", "Color Fade Two Zone", "Brightness Fade", "Pulse", "Rainbow",
                "Christmas Lights"},
            device));
    out("mode_empty", ha::mode_discovery_json(t, {}, device));
    out("param_b",
        ha::param_discovery_json(t, "hue", ha::json_string("Hue"), "0", "255", "1", false, device));
    out("param_a",
        ha::param_discovery_json(
            t, "speed", ha::json_string("Speed"), "1", "50", "1", true, device));
    out("param_gap",
        ha::param_discovery_json(
            t, "h_gap", ha::json_string("Color Variance"), "0", "65535", "100", true, device));
    out("param_null_name",
        ha::param_discovery_json(t, "x", "null", "null", "null", "null", false, device));

    // escaping: quote, backslash, \b \f \n \r \t, 0x01, 0x1f, '/', UTF-8, NUL
    out("escape",
        ha::json_string(
            std::string("A\"\\\b\f\n\r\t\x01\x1f/\xc3\xa9", 13) + std::string(1, '\0') + "z"));
    out("device_ctrl", ha::device_json(t, "Desk \"LED\"\n\b\f\x01", "3.0.0"));

    out("state_on", ha::light_state_json(true, 128, 85, 255));
    out("state_off", ha::light_state_json(false, 0, 0, 0));
    out("state_max", ha::light_state_json(true, 255, 255, 128));
    out("state_mid", ha::light_state_json(true, 50, 128, 1));

    // HA -> led conversions
    const float in_deg[] = {0.0f, 120.0f, 195.5294f, 360.0f, 400.0f, -5.0f, 0.7f};
    const float in_pct[] = {0.0f, 47.2549f, 50.0f, 100.0f, 150.0f, -1.0f, 0.19f};
    std::string hues, sats;
    for (const float d : in_deg)
        hues += (hues.empty() ? "" : ",") + std::to_string(ha::deg_to_hue8(d));
    for (const float p : in_pct)
        sats += (sats.empty() ? "" : ",") + std::to_string(ha::pct_to_sat8(p));
    out("deg_to_hue8", hues);
    out("pct_to_sat8", sats);
}

#if __has_include(<ArduinoJson.h>) && __has_include(<XeWeCore/Utils/Color.h>)
#define HAVE_ARDUINOJSON 1
#include "../../src/HomeAssistant/HaBridge.h"

namespace v230 {
// 2.3.0 HomeAssistant.cpp builders, with the module's members turned into arguments
struct Ctx {
    std::string base_topic, device_id, device_name, sw_version;
};

void add_device(const Ctx& c, JsonDocument& doc) {
    JsonObject dev = doc["device"].to<JsonObject>();
    dev["identifiers"][0] = c.device_id;
    dev["name"] = c.device_name;
    dev["manufacturer"] = "XeWe";
    dev["model"] = "LED";
    dev["sw_version"] = c.sw_version;
}

std::string light_discovery(const Ctx& c) {
    JsonDocument doc;
    doc["~"] = c.base_topic;
    doc["schema"] = "json";
    doc["name"] = nullptr;
    doc["unique_id"] = c.device_id + "_light";
    doc["command_topic"] = "~/light/set";
    doc["state_topic"] = "~/light/state";
    doc["availability_topic"] = "~/avail";
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";
    doc["brightness"] = true;
    doc["color_mode"] = true;
    doc["supported_color_modes"][0] = "hs";
    add_device(c, doc);
    std::string payload;
    serializeJson(doc, payload);
    return payload;
}

std::string mode_discovery(const Ctx& c, const std::string& modes_json) {
    JsonDocument doc;
    doc["~"] = c.base_topic;
    doc["name"] = "Mode";
    doc["unique_id"] = c.device_id + "_mode";
    doc["command_topic"] = "~/mode/set";
    doc["state_topic"] = "~/mode/state";
    doc["availability_topic"] = "~/avail";
    JsonArray opts = doc["options"].to<JsonArray>();
    JsonDocument modes;
    if (!deserializeJson(modes, modes_json)) {
        for (JsonObjectConst m : modes.as<JsonArrayConst>())
            opts.add(std::string(m["name"] | ""));
    }
    add_device(c, doc);
    std::string payload;
    serializeJson(doc, payload);
    return payload;
}

std::string param_discovery(const Ctx& c, JsonObjectConst p) {
    std::string key = std::string(p["key"] | "");
    JsonDocument doc;
    doc["~"] = c.base_topic;
    doc["name"] = p["display_name"];
    doc["unique_id"] = c.device_id + "_" + key;
    doc["command_topic"] = "~/param/" + key + "/set";
    doc["state_topic"] = "~/param/" + key + "/state";
    doc["availability_topic"] = "~/avail";
    doc["min"] = p["min"];
    doc["max"] = p["max"];
    doc["step"] = p["step"];
    const char* type = p["type"] | "b";
    if (type[0] == 'a')
        doc["entity_category"] = "config";
    add_device(c, doc);
    std::string payload;
    serializeJson(doc, payload);
    return payload;
}

std::string light_state(bool on, uint8_t brightness, uint8_t hue8, uint8_t sat8) {
    JsonDocument doc;
    doc["state"] = on ? "ON" : "OFF";
    doc["brightness"] = brightness;
    doc["color_mode"] = "hs";
    JsonObject color = doc["color"].to<JsonObject>();
    color["h"] = lroundf(hue8 / 255.0f * 360.0f);
    color["s"] = lroundf(sat8 / 255.0f * 100.0f);
    std::string payload;
    serializeJson(doc, payload);
    return payload;
}

#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
uint8_t deg_to_hue8(float h) {
    return (uint8_t)lroundf(constrain(h, 0.0f, 360.0f) / 360.0f * 255.0f);
}
uint8_t pct_to_sat8(float s) {
    return (uint8_t)lroundf(constrain(s, 0.0f, 100.0f) / 100.0f * 255.0f);
}
#undef constrain
} // namespace v230

using ha::json_value;

static int failures = 0;
static long checks = 0;

static void same(const char* what, const std::string& want, const std::string& got) {
    ++checks;
    if (want == got || failures > 5)
        return;
    ++failures;
    std::printf("diff %d\t%s | 2.3.0: %s | port: %s\n", failures, what, want.c_str(), got.c_str());
}

static void compare_with_arduinojson() {
    const std::vector<std::string> names = {"Desk", "", "XeWe LED", "Desk \"LED\"\n", "tab\there\r",
        "ctl\b\f\x01\x02\x1f\x7f end", "back\\slash/slash", "\xc3\xbcn\xc3\xaf",
        std::string("nul\0x", 5), "{\"json\":[1]}"};
    const std::vector<std::string> macs = {"AA:BB:CC:0D:EE:F1", "", "12:34:56:78:9A:BC"};
    const std::vector<std::string> versions = {"3.0.0", "0.0.0", "3.1.0-rc.1"};

    JsonDocument modes;
    if (deserializeJson(modes, MODES_JSON)) {
        std::printf("diff\tmodes json does not parse\n");
        ++failures;
        return;
    }

    for (const std::string& mac : macs) {
        const ha::Topics t = ha::make_topics(ha::mac_hex(mac));
        for (const std::string& name : names) {
            for (const std::string& ver : versions) {
                const v230::Ctx c{t.base, t.device_id, name, ver};
                const std::string device = ha::device_json(t, name, ver);
                same("light discovery", v230::light_discovery(c),
                    ha::light_discovery_json(t, device));

                std::vector<std::string> mode_names;
                for (JsonObjectConst m : modes.as<JsonArrayConst>())
                    mode_names.emplace_back(m["name"] | "");
                same("mode discovery", v230::mode_discovery(c, MODES_JSON),
                    ha::mode_discovery_json(t, mode_names, device));
                same("mode discovery, bad json", v230::mode_discovery(c, "not json"),
                    ha::mode_discovery_json(t, {}, device));

                for (JsonObjectConst m : modes.as<JsonArrayConst>()) {
                    for (JsonObjectConst p : m["params"].as<JsonArrayConst>()) {
                        const std::string key = std::string(p["key"] | "");
                        const char* type = p["type"] | "b";
                        same("param discovery", v230::param_discovery(c, p),
                            ha::param_discovery_json(t, key, json_value(p["display_name"]),
                                json_value(p["min"]), json_value(p["max"]), json_value(p["step"]),
                                type[0] == 'a', device));
                    }
                }
            }
        }
    }

    // a parameter entry without display_name / min / max / step / type: copied as null
    JsonDocument sparse;
    deserializeJson(sparse, "{\"key\":\"k\"}");
    {
        const ha::Topics t = ha::make_topics("aabbcc0deef1");
        const v230::Ctx c{t.base, t.device_id, "Desk", "3.0.0"};
        JsonObjectConst p = sparse.as<JsonObjectConst>();
        same("param discovery, sparse entry", v230::param_discovery(c, p),
            ha::param_discovery_json(t, "k", json_value(p["display_name"]), json_value(p["min"]),
                json_value(p["max"]), json_value(p["step"]), false,
                ha::device_json(t, "Desk", "3.0.0")));
    }

    const char* grid = std::getenv("XEWE_TEST_HA_FULL_GRID");
    const bool full_grid = grid != nullptr && std::string(grid) == "1";
    out("grid", full_grid ? "full" : "fast");
    for (int on = 0; on < 2; ++on) {
        for (int b : {0, 1, 50, 128, 255}) {
            if (!full_grid && b != 128)
                continue;
            for (int h = 0; h < 256; ++h)
                for (int s = 0; s < 256; ++s)
                    same("light state", v230::light_state(on, b, h, s),
                        ha::light_state_json(on, b, h, s));
        }
        if (full_grid)
            continue;
        for (int b = 0; b < 256; ++b)
            for (const std::array<int, 2> hs : {std::array<int, 2>{0, 0}, {255, 255}, {128, 77}})
                same("light state", v230::light_state(on, b, hs[0], hs[1]),
                    ha::light_state_json(on, b, hs[0], hs[1]));
    }

    for (long i = -20000; i <= 4200000; ++i) {
        const float x = i / 10000.0f;
        if (ha::deg_to_hue8(x) != v230::deg_to_hue8(x))
            same("deg_to_hue8", std::to_string(v230::deg_to_hue8(x)),
                std::to_string(ha::deg_to_hue8(x)) + " at " + std::to_string(x));
        if (ha::pct_to_sat8(x) != v230::pct_to_sat8(x))
            same("pct_to_sat8", std::to_string(v230::pct_to_sat8(x)),
                std::to_string(ha::pct_to_sat8(x)) + " at " + std::to_string(x));
        checks += 2;
    }
}

// ---- ha::Bridge with fakes ------------------------------------------------------------------
namespace fake {
std::vector<std::string> events;

// PubSubClient: records publishes / subscriptions; every publish overwrites the shared buffer the
// way PubSubClient reuses its buffer, so a callback that kept pointers into it would read garbage
struct Mqtt {
    bool up = true;
    bool publish_ok = true;
    char buffer[512] = {};
    bool connected() {
        return up;
    }
    bool publish(const char* topic, const char* payload, bool retained) {
        events.push_back(std::string("pub ") + (retained ? "R " : "- ") + topic + " " + payload);
        for (char& c : buffer)
            c = '#';
        return publish_ok;
    }
    bool subscribe(const char* topic) {
        events.push_back(std::string("sub ") + topic);
        return true;
    }
    bool loop() {
        events.push_back("loop");
        return true;
    }
};

// the led: getters from fields, every sync call recorded and then echoed the way the module's
// listener hooks do (`echo`)
struct Led {
    const void* module = nullptr;
    bool state = true;
    uint8_t brightness = 128;
    std::array<uint8_t, 3> hsv{85, 255, 255};
    uint8_t mode = 0;
    std::vector<std::string> names = {"Solid", "Color Fade", "Color Fade Two Zone",
        "Brightness Fade", "Pulse", "Rainbow", "Christmas Lights"};
    std::string modes_json = MODES_JSON;
    std::map<std::string, uint16_t> params;
    std::function<void(bool mode_changed)> echo;

    std::string who(const void* origin) const {
        return origin == module ? " by=ha" : " by=?";
    }
    void sync_state(bool on, const void* origin) {
        events.push_back("led state " + std::to_string(on) + who(origin));
        state = on;
        echo(false);
    }
    void sync_brightness(uint8_t b, const void* origin) {
        events.push_back("led brightness " + std::to_string(b) + who(origin));
        brightness = b;
        echo(false);
    }
    void sync_color(std::array<uint8_t, 3> rgb, const void* origin) {
        events.push_back("led color " + std::to_string(rgb[0]) + "," + std::to_string(rgb[1]) +
            "," + std::to_string(rgb[2]) + who(origin));
        echo(false);
    }
    void sync_mode(uint8_t id, const void* origin) {
        events.push_back("led mode " + std::to_string(id) + who(origin));
        mode = id;
        echo(true);
    }
    void set_mode_param(std::string_view key, uint16_t value) {
        events.push_back("led param " + std::string(key) + " " + std::to_string(value));
        params[std::string(key)] = value > 50 ? 50 : value; // clamps like a 1..50 parameter
    }
    std::array<uint8_t, 3> get_hsv() const {
        return hsv;
    }
    bool get_state() const {
        return state;
    }
    uint8_t get_brightness() const {
        return brightness;
    }
    uint8_t get_current_mode_id() const {
        return mode;
    }
    std::string_view get_current_mode_name() const {
        return names.at(mode);
    }
    uint16_t get_current_mode_param(std::string_view key) const {
        const auto it = params.find(std::string(key));
        return it == params.end() ? 0 : it->second;
    }
    std::string get_all_modes_json() const {
        return modes_json;
    }
};

struct Serial {
    void print(std::string_view text) {
        events.push_back("print " + std::string(text));
    }
};
struct System {
    std::string name = "Desk";
    std::string get_device_name() {
        return name;
    }
};
struct Os {
    Serial serial;
    System system;
};
} // namespace fake

using TestBridge = ha::Bridge<fake::Mqtt, fake::Led, fake::Os>;

static void flush(const char* name) {
    std::string list;
    for (const std::string& e : fake::events)
        list += (list.empty() ? "" : ",") + ha::json_string(e);
    out(name, "[" + list + "]");
    fake::events.clear();
}

// an inbound message delivered the way PubSubClient does: topic and payload in its buffer
static void deliver(
    fake::Mqtt& mqtt, TestBridge& bridge, const std::string& topic, const std::string& payload) {
    std::snprintf(mqtt.buffer, sizeof(mqtt.buffer), "%s", topic.c_str());
    char* p = mqtt.buffer + topic.size() + 1;
    for (size_t i = 0; i < payload.size(); ++i)
        p[i] = payload[i];
    bridge.on_message(
        mqtt.buffer, reinterpret_cast<uint8_t*>(p), static_cast<unsigned int>(payload.size()));
}

static void run_bridge_scenarios() {
    fake::Mqtt mqtt;
    fake::Led led;
    fake::Os os;
    int marker = 0; // stands for the module object passed as origin
    led.module = &marker;
    TestBridge bridge(mqtt, led, os, &marker, "3.0.0");
    bridge.topics = ha::make_topics(ha::mac_hex("AA:BB:CC:0D:EE:F1"));
    // the module's listener hooks: light -> light state; mode -> mode state, params, light state
    led.echo = [&](bool mode_changed) {
        if (mode_changed) {
            bridge.publish_mode_state();
            bridge.reconcile_params();
        }
        bridge.publish_light_state();
    };

    out("bridge_device", bridge.device_json());

    // connect() on Solid: online, discovery, state, Solid's parameters, subscriptions
    bridge.announce();
    flush("bridge_announce");

    // listener on_mode: Solid -> Color Fade keeps hue/sat, adds four; -> Rainbow clears five
    led.mode = 1;
    led.echo(true);
    flush("bridge_mode_fade");
    led.mode = 5;
    led.echo(true);
    flush("bridge_mode_rainbow");

    // inbound light commands (each sync is echoed as the listener does)
    const std::string light = bridge.topics.light_cmd;
    deliver(mqtt, bridge, light,
        "{\"state\":\"ON\",\"brightness\":300,\"color\":{\"h\":120,\"s\":50}}");
    flush("bridge_light_full");
    deliver(mqtt, bridge, light, "{\"state\":\"on\",\"brightness\":-5}");
    flush("bridge_light_off_clamp");
    led.hsv = {10, 20, 30};
    deliver(mqtt, bridge, light, "{\"color\":{\"s\":100}}");
    flush("bridge_light_keep_hue");
    deliver(mqtt, bridge, light, "{\"color\":{\"h\":null,\"s\":null}}");
    flush("bridge_light_keep_both");
    deliver(mqtt, bridge, light, "{\"color\":{\"h\":400,\"s\":-3}}");
    flush("bridge_light_clamp_colour");
    led.hsv = {85, 255, 255};
    deliver(mqtt, bridge, light, "{\"state\":1,\"brightness\":\"7\",\"color\":[1]}");
    deliver(mqtt, bridge, light, "{\"brightness\":7.5}");
    deliver(mqtt, bridge, light, "not json");
    deliver(mqtt, bridge, light, "");
    flush("bridge_light_ignored");

    // inbound mode by name (exact, case-sensitive, first match)
    const std::string mode = bridge.topics.mode_cmd;
    deliver(mqtt, bridge, mode, "rainbow");
    deliver(mqtt, bridge, mode, "");
    flush("bridge_mode_unknown");
    deliver(mqtt, bridge, mode, "Solid");
    flush("bridge_mode_solid");

    // inbound parameters: strtol base 10, uint16_t cast, the led clamps, the result published
    const std::string base = bridge.topics.base;
    deliver(mqtt, bridge, base + "/param/hue/set", "12");
    deliver(mqtt, bridge, base + "/param/speed/set", "99");
    deliver(mqtt, bridge, base + "/param/speed/set", "-1");
    deliver(mqtt, bridge, base + "/param/speed/set", "0x10");
    deliver(mqtt, bridge, base + "/param/speed/set", "abc");
    flush("bridge_param");
    deliver(mqtt, bridge, base + "/param/a/b/set", "1");
    deliver(mqtt, bridge, base + "/param//set", "1");
    deliver(mqtt, bridge, "xewe_led_os/other/param/speed/set", "1");
    deliver(mqtt, bridge, base + "/param/speed/state", "1");
    deliver(mqtt, bridge, bridge.topics.light_state, "{\"state\":\"OFF\"}");
    flush("bridge_other_topics");

    // deprovision / reset while connected (published keys: Solid's hue and sat)
    bridge.retract();
    flush("bridge_retract");
    bridge.retract();
    flush("bridge_retract_again");

    // nothing is published while disconnected
    mqtt.up = false;
    bridge.publish_light_discovery();
    bridge.publish_mode_discovery();
    bridge.publish_light_state();
    bridge.publish_mode_state();
    bridge.reconcile_params();
    bridge.publish_param_state("speed", 3);
    deliver(mqtt, bridge, light, "{\"brightness\":9}");
    flush("bridge_disconnected");
    mqtt.up = true;

    // a refused light discovery (payload over the buffer) prints the hint; mode discovery does not
    mqtt.publish_ok = false;
    bridge.publish_light_discovery();
    bridge.publish_mode_discovery();
    flush("bridge_publish_refused");
    mqtt.publish_ok = true;

    // modes JSON that does not parse: empty options, no parameter change, mode commands ignored
    led.modes_json = "not json";
    bridge.publish_mode_discovery();
    bridge.reconcile_params();
    deliver(mqtt, bridge, mode, "Solid");
    flush("bridge_bad_modes");

    // odd modes JSON entries: JSON values copied as written (compact), missing ones null,
    // empty keys skipped, a value over 65535 wraps in the uint16_t cast, a nameless mode gives ""
    led.modes_json =
        "[{\"id\":0,\"name\":\"Odd\",\"params\":[{\"key\":\"k\",\"display_name\":{\"a\":[1, 2]},"
        "\"min\":-1.5,\"max\":\"x\",\"step\":null,\"value\":70000,\"type\":\"a\"},"
        "{\"key\":\"\",\"value\":1},{\"display_name\":\"no key\"},{\"key\":\"n\"}]},"
        "{\"id\":1,\"params\":[]}]";
    led.names = {"Odd", ""};
    led.mode = 0;
    bridge.publish_mode_discovery();
    bridge.reconcile_params();
    flush("bridge_odd_modes");

    os.system.name = "Kitchen \"2\"";
    out("bridge_device_renamed", bridge.device_json());
    out("bridge", "ran");
}
#endif

int main() {
    print_fixed_cases();
#ifdef HAVE_ARDUINOJSON
    compare_with_arduinojson();
    out("arduinojson", failures ? "FAILED" : "same:" + std::to_string(checks));
    run_bridge_scenarios();
#else
    out("arduinojson", "absent");
    out("bridge", "absent");
#endif
    return 0;
}
