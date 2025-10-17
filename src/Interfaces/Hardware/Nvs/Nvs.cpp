/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/



// src/Interfaces/Nvs/Nvs.cpp

#include "Nvs.h"
#include "../../../SystemController/SystemController.h"

Nvs::Nvs(SystemController& controller)
      : Interface(controller,
               /* module_name         */ "Nvs",
               /* module_description  */ "Stores user settings even when the power is off",
               /* nvs_key             */ "nvs",
               /* requires_init_setup */ false,
               /* can_be_disabled     */ false,
               /* has_cli_cmds        */ true)
{}

void Nvs::sync_color(std::array<uint8_t,3> color) {
    DBG_PRINTF(Nvs, "sync_color(): R=%u, G=%u, B=%u\n", color[0], color[1], color[2]);
    write_uint8(nvs_key, "led_r", color[0]);
    write_uint8(nvs_key, "led_g", color[1]);
    write_uint8(nvs_key, "led_b", color[2]);
}

void Nvs::sync_brightness(uint8_t brightness) {
    DBG_PRINTF(Nvs, "sync_brightness(): brightness=%u\n", brightness);
    write_uint8(nvs_key, "led_bri", brightness);
}

void Nvs::sync_state(uint8_t state) {
    DBG_PRINTF(Nvs, "sync_state(): state=%s\n", static_cast<bool>(state) ? "ON" : "OFF");
    write_bool(nvs_key, "led_state", static_cast<bool>(state));
}

void Nvs::sync_mode(uint8_t mode) {
    DBG_PRINTF(Nvs, "sync_mode(): mode=%u\n", mode);
    write_uint8(nvs_key, "led_mode", mode);
}

void Nvs::sync_length(uint16_t length) {
    DBG_PRINTF(Nvs, "sync_length(): length=%u\n", length);
    write_uint16(nvs_key, "led_len", length);
}

void Nvs::reset (const bool verbose, const bool do_restart) {
    DBG_PRINTLN(Nvs, "reset(): Clearing all stored preferences.");
    if (!preferences.begin(nvs_key.c_str(), false)) {
        DBG_PRINTF(Nvs, "reset(): ERROR opening namespace '%s'.\n", nvs_key.c_str());
        return;
    }
    if (preferences.clear()) {
        DBG_PRINTLN(Nvs, "reset(): Successfully cleared preferences.");
    } else {
        DBG_PRINTLN(Nvs, "reset(): FAILED to clear preferences.");
    }
    preferences.end();
    Module::reset(verbose, do_restart);
}

void Nvs::sync_from_memory(std::array<uint8_t,5> sync_flags) {
    DBG_PRINTLN(Nvs, "sync_from_memory(): Reading all parameters from NVS and applying to controller.");
    controller.sync_all(
        { read_uint8(nvs_key, "led_r"), read_uint8(nvs_key, "led_g"), read_uint8(nvs_key, "led_b") },
        read_uint8(nvs_key, "led_bri"),
        read_bool(nvs_key, "led_state"),
        read_uint8(nvs_key, "led_mode"),
        read_uint16(nvs_key, "led_len", LED_STRIP_NUM_LEDS_MAX),
        sync_flags
    );
    DBG_PRINTLN(Nvs, "sync_from_memory(): Sync from memory complete.");
}

void Nvs::write_str(std::string_view ns, std::string_view key, std::string_view value) {
    DBG_PRINTF(Nvs, "write_str(): Attempting to write ns='%s', key='%s', value='%s'.\n", ns.data(), key.data(), value.data());
    std::string k = full_key(ns, key);
    if (!preferences.begin(nvs_key.c_str(), false)) {
        DBG_PRINTF(Nvs, "write_str(): ERROR opening namespace '%s'.\n", nvs_key.c_str());
        return;
    }
    DBG_PRINTF(Nvs, "write_str(): Writing to key '%s' value '%s'.\n", k.c_str(), value.data());
    size_t bytes_written = preferences.putString(k.c_str(), value.data());
    if (bytes_written > 0) {
        DBG_PRINTF(Nvs, "write_str(): Successfully wrote %zu bytes for key '%s'.\n", bytes_written, k.c_str());
    } else {
        DBG_PRINTF(Nvs, "write_str(): FAILED to write to key '%s'.\n", k.c_str());
    }
    preferences.end();
}

void Nvs::write_uint8(std::string_view ns, std::string_view key, uint8_t value) {
    DBG_PRINTF(Nvs, "write_uint8(): Attempting to write ns='%s', key='%s', value=%u.\n", ns.data(), key.data(), value);
    std::string k = full_key(ns, key);
    if (!preferences.begin(nvs_key.c_str(), false)) {
        DBG_PRINTF(Nvs, "write_uint8(): ERROR opening namespace '%s'.\n", nvs_key.c_str());
        return;
    }
    DBG_PRINTF(Nvs, "write_uint8(): Writing to key '%s' value %u.\n", k.c_str(), value);
    if (preferences.putUChar(k.c_str(), value)) {
        DBG_PRINTF(Nvs, "write_uint8(): Successfully wrote value for key '%s'.\n", k.c_str());
    } else {
        DBG_PRINTF(Nvs, "write_uint8(): FAILED to write to key '%s'.\n", k.c_str());
    }
    preferences.end();
}

void Nvs::write_uint16(std::string_view ns, std::string_view key, uint16_t value) {
    DBG_PRINTF(Nvs, "write_uint16(): Attempting to write ns='%s', key='%s', value=%u.\n", ns.data(), key.data(), value);
    std::string k = full_key(ns, key);
    if (!preferences.begin(nvs_key.c_str(), false)) {
        DBG_PRINTF(Nvs, "write_uint16(): ERROR opening namespace '%s'.\n", nvs_key.c_str());
        return;
    }
    DBG_PRINTF(Nvs, "write_uint16(): Writing to key '%s' value %u.\n", k.c_str(), value);
    if (preferences.putUShort(k.c_str(), value)) {
        DBG_PRINTF(Nvs, "write_uint16(): Successfully wrote value for key '%s'.\n", k.c_str());
    } else {
        DBG_PRINTF(Nvs, "write_uint16(): FAILED to write to key '%s'.\n", k.c_str());
    }
    preferences.end();
}

void Nvs::write_bool(std::string_view ns, std::string_view key, bool value) {
    DBG_PRINTF(Nvs, "write_bool(): Attempting to write ns='%s', key='%s', value=%s.\n", ns.data(), key.data(), value ? "true" : "false");
    std::string k = full_key(ns, key);
    if (!preferences.begin(nvs_key.c_str(), false)) {
        DBG_PRINTF(Nvs, "write_bool(): ERROR opening namespace '%s'.\n", nvs_key.c_str());
        return;
    }
    DBG_PRINTF(Nvs, "write_bool(): Writing to key '%s' value %s.\n", k.c_str(), value ? "true" : "false");
    if (preferences.putBool(k.c_str(), value)) {
        DBG_PRINTF(Nvs, "write_bool(): Successfully wrote value for key '%s'.\n", k.c_str());
    } else {
        DBG_PRINTF(Nvs, "write_bool(): FAILED to write to key '%s'.\n", k.c_str());
    }
    preferences.end();
}

void Nvs::remove(std::string_view ns, std::string_view key) {
    DBG_PRINTF(Nvs, "remove(): Attempting to remove ns='%s', key='%s'.\n", ns.data(), key.data());
    std::string k = full_key(ns, key);
    if (!preferences.begin(nvs_key.c_str(), false)) {
        DBG_PRINTF(Nvs, "remove(): ERROR opening namespace '%s'.\n", nvs_key.c_str());
        return;
    }
    DBG_PRINTF(Nvs, "remove(): Removing key '%s'.\n", k.c_str());
    if (preferences.remove(k.c_str())) {
        DBG_PRINTF(Nvs, "remove(): Successfully removed key '%s'.\n", k.c_str());
    } else {
        DBG_PRINTF(Nvs, "remove(): FAILED to remove key '%s'. Key might not exist.\n", k.c_str());
    }
    preferences.end();
}

std::string Nvs::read_str(std::string_view ns, std::string_view key, std::string_view default_value) {
    DBG_PRINTF(Nvs, "read_str(): Attempting to read ns='%s', key='%s'.\n", ns.data(), key.data());
    // FIX: Changed 'true' to 'false' to allow namespace creation on first read
    if (!preferences.begin(nvs_key.c_str(), false)) {
        DBG_PRINTF(Nvs, "read_str(): ERROR opening namespace '%s'. Returning default value '%s'.\n", nvs_key.c_str(), default_value.data());
        return std::string(default_value);
    }
    std::string k = full_key(ns, key);
    String tmp = preferences.getString(k.c_str(), String(default_value.data()));
    std::string result(tmp.c_str());
    DBG_PRINTF(Nvs, "read_str(): Read key '%s', got value '%s'.\n", k.c_str(), result.c_str());
    preferences.end();
    return result;
}

uint8_t Nvs::read_uint8(std::string_view ns, std::string_view key, uint8_t default_value) {
    DBG_PRINTF(Nvs, "read_uint8(): Attempting to read ns='%s', key='%s'.\n", ns.data(), key.data());
    // FIX: Changed 'true' to 'false' to allow namespace creation on first read
    if (!preferences.begin(nvs_key.c_str(), false)) {
        DBG_PRINTF(Nvs, "read_uint8(): ERROR opening namespace '%s'. Returning default value %u.\n", nvs_key.c_str(), default_value);
        return default_value;
    }
    std::string k = full_key(ns, key);
    uint8_t v = preferences.getUChar(k.c_str(), default_value);
    DBG_PRINTF(Nvs, "read_uint8(): Read key '%s', got value %u.\n", k.c_str(), v);
    preferences.end();
    return v;
}

uint16_t Nvs::read_uint16(std::string_view ns, std::string_view key, uint16_t default_value) {
    DBG_PRINTF(Nvs, "read_uint16(): Attempting to read ns='%s', key='%s'.\n", ns.data(), key.data());
    // FIX: Changed 'true' to 'false' to allow namespace creation on first read
    if (!preferences.begin(nvs_key.c_str(), false)) {
        DBG_PRINTF(Nvs, "read_uint16(): ERROR opening namespace '%s'. Returning default value %u.\n", nvs_key.c_str(), default_value);
        return default_value;
    }
    std::string k = full_key(ns, key);
    uint16_t v = preferences.getUShort(k.c_str(), default_value);
    DBG_PRINTF(Nvs, "read_uint16(): Read key '%s', got value %u.\n", k.c_str(), v);
    preferences.end();
    return v;
}

bool Nvs::read_bool(std::string_view ns, std::string_view key, bool default_value) {
    DBG_PRINTF(Nvs, "read_bool(): Attempting to read ns='%s', key='%s'.\n", ns.data(), key.data());
    // FIX: Changed 'true' to 'false' to allow namespace creation on first read
    if (!preferences.begin(nvs_key.c_str(), false)) {
        DBG_PRINTF(Nvs, "read_bool(): ERROR opening namespace '%s'. Returning default value %s.\n", nvs_key.c_str(), default_value ? "true" : "false");
        return default_value;
    }
    std::string k = full_key(ns, key);
    bool v = preferences.getBool(k.c_str(), default_value);
    DBG_PRINTF(Nvs, "read_bool(): Read key '%s', got value %s.\n", k.c_str(), v ? "true" : "false");
    preferences.end();
    return v;
}

std::string Nvs::full_key(std::string_view ns, std::string_view key) const {
    DBG_PRINTF(Nvs, "full_key(): Generating key for ns='%s', key='%s'.\n", ns.data(), key.data());
    std::string combined = std::string(ns) + ":" + std::string(key);
    if (combined.length() > MAX_KEY_LEN) {
        DBG_PRINTF(Nvs, "full_key(): WARNING: key '%s' is too long (%u chars), truncating to %u\n",
                   combined.c_str(), (unsigned)combined.length(), (unsigned)MAX_KEY_LEN);
        combined.resize(MAX_KEY_LEN);
        DBG_PRINTF(Nvs, "full_key(): Truncated key is '%s'.\n", combined.c_str());
    }
    DBG_PRINTF(Nvs, "full_key(): Resulting key is '%s'.\n", combined.c_str());
    return combined;
}