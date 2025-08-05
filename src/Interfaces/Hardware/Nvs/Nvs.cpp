// src/Interfaces/Hardware/Nvs/Nvs.cpp
#include "Nvs.h"
#include "../../../SystemController.h"
#include <string>

Nvs::Nvs(SystemController& controller_ref)
  : Interface(controller_ref, "nvs", "nvs", false, false)
{
    DBG_PRINTLN(Nvs, "Constructor called.");
}

void Nvs::begin(const ModuleConfig& /*cfg*/) {
    const char* ns = nvs_key.c_str();
    if (!ns) {
        DBG_PRINTLN(Nvs, "begin(): ERROR - nvs_key is null!");
        return;
    }
    if (preferences.begin(ns, false)) {
        initialized = true;
        DBG_PRINTF(Nvs, "begin(): Preferences initialized ('%s').\n", ns);
    } else {
        initialized = false;
        DBG_PRINTF(Nvs, "begin(): ERROR initializing '%s'.\n", ns);
    }
}

void Nvs::loop() {
    if (dirty && millis() >= commit_time) {
        commit();
    }
}

void Nvs::reset() {
    DBG_PRINTLN(Nvs, "reset(): Clearing all keys.");
    if (!initialized) {
        DBG_PRINTLN(Nvs, "reset(): ERROR - Not initialized.");
        return;
    }
    preferences.clear();
    schedule_commit();
    commit();
}

std::string_view Nvs::status() const {
    return initialized ? "initialized" : "uninitialized";
}

void Nvs::write_str(std::string_view key, std::string_view value) {
    std::string k{key};
    std::string v{value};
    DBG_PRINTF(Nvs, "write_str(): %s='%s'\n", k.c_str(), v.c_str());
    if (!initialized) return;
    preferences.putString(k.c_str(), String(v.c_str()));
    schedule_commit();
}

const char* Nvs::read_str(std::string_view key, std::string_view default_value) {
    std::string k{key};
    std::string def{default_value};
    if (!initialized) return def.c_str();
    String tmp = preferences.getString(k.c_str(), String(def.c_str()));
    static char buf[128];
    tmp.toCharArray(buf, sizeof(buf));
    return buf;
}

void Nvs::write_uint8(std::string_view key, uint8_t v) {
    std::string k{key};
    DBG_PRINTF(Nvs, "write_uint8(): %s=%u\n", k.c_str(), v);
    if (!initialized) return;
    preferences.putUChar(k.c_str(), v);
    schedule_commit();
}

uint8_t Nvs::read_uint8(std::string_view key, uint8_t default_value) {
    std::string k{key};
    if (!initialized) return default_value;
    return preferences.getUChar(k.c_str(), default_value);
}

void Nvs::write_uint16(std::string_view key, uint16_t v) {
    std::string k{key};
    DBG_PRINTF(Nvs, "write_uint16(): %s=%u\n", k.c_str(), v);
    if (!initialized) return;
    preferences.putUShort(k.c_str(), v);
    schedule_commit();
}

uint16_t Nvs::read_uint16(std::string_view key, uint16_t default_value) {
    std::string k{key};
    if (!initialized) return default_value;
    return preferences.getUShort(k.c_str(), default_value);
}

void Nvs::write_bool(std::string_view key, bool v) {
    std::string k{key};
    DBG_PRINTF(Nvs, "write_bool(): %s=%s\n", k.c_str(), v ? "true" : "false");
    if (!initialized) return;
    preferences.putBool(k.c_str(), v);
    schedule_commit();
}

bool Nvs::read_bool(std::string_view key, bool default_value) {
    std::string k{key};
    if (!initialized) return default_value;
    return preferences.getBool(k.c_str(), default_value);
}

void Nvs::remove(std::string_view key) {
    std::string k{key};
    if (!initialized) return;
    preferences.remove(k.c_str());
    schedule_commit();
    commit();
}

void Nvs::sync_color(std::array<uint8_t,3> c) {
    if (!initialized) return;
    preferences.putUChar("led_r", c[0]);
    preferences.putUChar("led_g", c[1]);
    preferences.putUChar("led_b", c[2]);
    schedule_commit();
}

void Nvs::sync_brightness(uint8_t b) {
    if (!initialized) return;
    preferences.putUChar("led_bri", b);
    schedule_commit();
}

void Nvs::sync_state(uint8_t s) {
    write_bool("led_state", s);
}

void Nvs::sync_mode(uint8_t m) {
    write_uint8("led_mode", m);
}

void Nvs::sync_length(uint16_t l) {
    if (!initialized) return;
    preferences.putUShort("led_len", l);
    schedule_commit();
}

void Nvs::sync_all(std::array<uint8_t,3> c, uint8_t b, uint8_t s, uint8_t m, uint16_t l) {
    if (!initialized) return;
    preferences.putUChar("led_r",   c[0]);
    preferences.putUChar("led_g",   c[1]);
    preferences.putUChar("led_b",   c[2]);
    preferences.putUChar("led_bri", b);
    preferences.putBool ("led_state", s);
    preferences.putUChar("led_mode", m);
    preferences.putUShort("led_len", l);
    schedule_commit();
    commit();
}

void Nvs::commit() {
    if (!initialized || !dirty) return;
    preferences.end();
    dirty = false;
    const char* ns = nvs_key.c_str();
    if (!preferences.begin(ns, false)) {
        initialized = false;
    }
}

void Nvs::schedule_commit() {
    dirty       = true;
    commit_time = millis() + COMMIT_DELAY_MS;
}
