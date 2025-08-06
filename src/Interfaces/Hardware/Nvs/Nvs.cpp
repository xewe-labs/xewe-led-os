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
    // No initialization needed
}

void Nvs::loop() {
    // Nothing to do; writes happen immediately
}

void Nvs::reset() {
    DBG_PRINTLN(Nvs, "reset(): Clearing all stored preferences.");
    if (!preferences.begin(nvs_key.c_str(), false)) {
        DBG_PRINTLN(Nvs, "reset(): ERROR opening namespace");
        return;
    }
    preferences.clear();
    preferences.end();
}

std::string_view Nvs::status(bool print) const {
    if (print) {
        controller.serial_port.println("ready");
    }
    return "ready";
}

// Sync methods
void Nvs::sync_color(std::array<uint8_t, 3> color) {
    write_uint8(nvs_key, "led_r", color[0]);
    write_uint8(nvs_key, "led_g", color[1]);
    write_uint8(nvs_key, "led_b", color[2]);
}

void Nvs::sync_brightness(uint8_t brightness) {
    write_uint8(nvs_key, "led_bri", brightness);
}

void Nvs::sync_state(uint8_t state) {
    write_bool(nvs_key, "led_state", static_cast<bool>(state));
}

void Nvs::sync_mode(uint8_t mode) {
    write_uint8(nvs_key, "led_mode", mode);
}

void Nvs::sync_length(uint16_t length) {
    write_uint16(nvs_key, "led_len", length);
}

void Nvs::sync_all(std::array<uint8_t, 3> color,
                  uint8_t brightness,
                  uint8_t state,
                  uint8_t mode,
                  uint16_t length) {
    sync_color(color);
    sync_brightness(brightness);
    sync_state(state);
    sync_mode(mode);
    sync_length(length);
}

// Write implementations
void Nvs::write_str(std::string_view ns, std::string_view key, std::string_view value) {
    std::string k = full_key(ns, key);
    if (!preferences.begin(nvs_key.c_str(), false)) {
        DBG_PRINTF(Nvs, "write_str(): ERROR opening namespace '%s'.\n", nvs_key.c_str());
        return;
    }
    DBG_PRINTF(Nvs, "write_str(): %s='%s'\n", k.c_str(), value.data());
    preferences.putString(k.c_str(), value.data());
    preferences.end();
}

void Nvs::write_uint8(std::string_view ns, std::string_view key, uint8_t value) {
    std::string k = full_key(ns, key);
    if (!preferences.begin(nvs_key.c_str(), false)) {
        DBG_PRINTF(Nvs, "write_uint8(): ERROR opening namespace '%s'.\n", nvs_key.c_str());
        return;
    }
    DBG_PRINTF(Nvs, "write_uint8(): %s=%u\n", k.c_str(), value);
    preferences.putUChar(k.c_str(), value);
    preferences.end();
}

void Nvs::write_uint16(std::string_view ns, std::string_view key, uint16_t value) {
    std::string k = full_key(ns, key);
    if (!preferences.begin(nvs_key.c_str(), false)) {
        DBG_PRINTF(Nvs, "write_uint16(): ERROR opening namespace '%s'.\n", nvs_key.c_str());
        return;
    }
    DBG_PRINTF(Nvs, "write_uint16(): %s=%u\n", k.c_str(), value);
    preferences.putUShort(k.c_str(), value);
    preferences.end();
}

void Nvs::write_bool(std::string_view ns, std::string_view key, bool value) {
    std::string k = full_key(ns, key);
    if (!preferences.begin(nvs_key.c_str(), false)) {
        DBG_PRINTF(Nvs, "write_bool(): ERROR opening namespace '%s'.\n", nvs_key.c_str());
        return;
    }
    DBG_PRINTF(Nvs, "write_bool(): %s=%s\n", k.c_str(), value ? "true" : "false");
    preferences.putBool(k.c_str(), value);
    preferences.end();
}

void Nvs::remove(std::string_view ns, std::string_view key) {
    std::string k = full_key(ns, key);
    if (!preferences.begin(nvs_key.c_str(), false)) {
        DBG_PRINTF(Nvs, "remove(): ERROR opening namespace '%s'.\n", nvs_key.c_str());
        return;
    }
    DBG_PRINTF(Nvs, "remove(): %s\n", k.c_str());
    preferences.remove(k.c_str());
    preferences.end();
}

// Read implementations
std::string Nvs::read_str(std::string_view ns, std::string_view key, std::string_view default_value) {
    if (!preferences.begin(nvs_key.c_str(), true)) return std::string(default_value);
    std::string k = full_key(ns, key);
    String tmp = preferences.getString(k.c_str(), String(default_value.data()));
    std::string result(tmp.c_str());
    preferences.end();
    return result;
}

uint8_t Nvs::read_uint8(std::string_view ns, std::string_view key, uint8_t default_value) {
    if (!preferences.begin(nvs_key.c_str(), true)) return default_value;
    std::string k = full_key(ns, key);
    uint8_t v = preferences.getUChar(k.c_str(), default_value);
    preferences.end();
    return v;
}

uint16_t Nvs::read_uint16(std::string_view ns, std::string_view key, uint16_t default_value) {
    if (!preferences.begin(nvs_key.c_str(), true)) return default_value;
    std::string k = full_key(ns, key);
    uint16_t v = preferences.getUShort(k.c_str(), default_value);
    preferences.end();
    return v;
}

bool Nvs::read_bool(std::string_view ns, std::string_view key, bool default_value) {
    if (!preferences.begin(nvs_key.c_str(), true)) return default_value;
    std::string k = full_key(ns, key);
    bool v = preferences.getBool(k.c_str(), default_value);
    preferences.end();
    return v;
}

// Helper
std::string Nvs::full_key(std::string_view ns, std::string_view key) const {
    std::string combined = std::string(ns) + ":" + std::string(key);
    if (combined.length() > MAX_KEY_LEN) {
        DBG_PRINTF(Nvs, "full_key(): key '%s' too long (%u chars), truncating to %u\n",
                   combined.c_str(), (unsigned)combined.length(), (unsigned)MAX_KEY_LEN);
        combined.resize(MAX_KEY_LEN);
    }
    return combined;
}