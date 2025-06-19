#include "Nvs.h"
#include "../../SystemController/SystemController.h" // Required for controller reference

// ~~~~~~~~~~~~~~~~~~ Nvs Class Implementation ~~~~~~~~~~~~~~~~~~

Nvs::Nvs(SystemController& controller_ref) : ControllerModule(controller_ref) {
    DBG_PRINTLN(Nvs, "Constructor called.");
}

void Nvs::begin(void* context) {
    if (!context) {
        DBG_PRINTLN(Nvs, "begin(): ERROR - Namespace context is null! Cannot initialize Nvs.");
        return;
    }

    const char* nvs_namespace = static_cast<const char*>(context);
    this->nvsNamespace = nvs_namespace;

    if (preferences.begin(nvs_namespace, false)) {
        initialized = true;
        DBG_PRINTF(Nvs, "begin(): Preferences initialized successfully with namespace '%s'.\n", nvs_namespace);
    } else {
        initialized = false;
        DBG_PRINTF(Nvs, "begin(): ERROR - Failed to initialize Preferences with namespace '%s'.\n", nvs_namespace);
    }
}

void Nvs::loop() {
    if (dirty && millis() >= commit_time) {
        commit();
    }
}

void Nvs::commit(){
    DBG_PRINTLN(Nvs, "commit(): Writing changes to NVS.");
    if (!initialized) return;

    if (!dirty) {
        DBG_PRINTLN(Nvs, "Nothing to commit");
        return;
    }

    preferences.end(); // Commits changes and closes the handle
    dirty = false;

    // Re-open the handle for subsequent operations.
    // Note: nvsNamespace is stored from begin().
    if (!preferences.begin(nvsNamespace, false)) {
        // Corrected the debug message to reflect the correct function name.
        DBG_PRINTLN(Nvs, "commit(): CRITICAL ERROR - Failed to re-open Preferences after commit.");
        initialized = false;
    }
}

void Nvs::schedule_commit() {
    if (!initialized) return;
    DBG_PRINTLN(Nvs, "schedule_commit(): Change detected, scheduling a commit.");
    dirty = true;
    commit_time = millis() + COMMIT_DELAY_MS;
}

// --- Generic NVS Read/Write Methods ---

void Nvs::write_str(const char* key, const String& value) {
    DBG_PRINTF(Nvs, "write_str(): key='%s', value='%s'\n", key, value.c_str());
    if (!initialized) return;
    preferences.putString(key, value);
    schedule_commit();
}

String Nvs::read_str(const char* key, const String& defaultValue) {
    DBG_PRINTF(Nvs, "read_str(): key='%s', default='%s'\n", key, defaultValue.c_str());
    if (!initialized) return defaultValue;
    String value = preferences.getString(key, defaultValue);
    DBG_PRINTF(Nvs, " -> read_str() returned: '%s'\n", value.c_str());
    return value;
}

void Nvs::write_uint8(const char* key, uint8_t value) {
    DBG_PRINTF(Nvs, "write_uint8(): key='%s', value=%u\n", key, value);
    if (!initialized) return;
    preferences.putUChar(key, value);
    schedule_commit();
}

uint8_t Nvs::read_uint8(const char* key, uint8_t defaultValue) {
    DBG_PRINTF(Nvs, "read_uint8(): key='%s', default=%u\n", key, defaultValue);
    if (!initialized) return defaultValue;
    uint8_t value = preferences.getUChar(key, defaultValue);
    DBG_PRINTF(Nvs, " -> read_uint8() returned: %u\n", value);
    return value;
}

void Nvs::write_uint16(const char* key, uint16_t value) {
    DBG_PRINTF(Nvs, "write_uint16(): key='%s', value=%u\n", key, value);
    if (!initialized) return;
    preferences.putUShort(key, value);
    schedule_commit();
}

uint16_t Nvs::read_uint16(const char* key, uint16_t defaultValue) {
    DBG_PRINTF(Nvs, "read_uint16(): key='%s', default=%u\n", key, defaultValue);
    if (!initialized) return defaultValue;
    uint16_t value = preferences.getUShort(key, defaultValue);
    DBG_PRINTF(Nvs, " -> read_uint16() returned: %u\n", value);
    return value;
}

void Nvs::write_bool(const char* key, bool value) {
    DBG_PRINTF(Nvs, "write_bool(): key='%s', value=%s\n", key, value ? "true" : "false");
    if (!initialized) return;
    preferences.putBool(key, value);
    schedule_commit();
}

bool Nvs::read_bool(const char* key, bool defaultValue) {
    DBG_PRINTF(Nvs, "read_bool(): key='%s', default=%s\n", key, defaultValue ? "true" : "false");
    if (!initialized) return defaultValue;
    bool value = preferences.getBool(key, defaultValue);
    DBG_PRINTF(Nvs, " -> read_bool() returned: %s\n", value ? "true" : "false");
    return value;
}

// ~~~~~~~~~~~~~~~~~~ Sync Methods (System -> Nvs) ~~~~~~~~~~~~~~~~~~

void Nvs::sync_color(std::array<uint8_t, 3> color) {
    DBG_PRINTF(Nvs, "sync_color(): r=%u, g=%u, b=%u\n", color[0], color[1], color[2]);
    if (!initialized) return;
    preferences.putUChar("led_r", color[0]);
    preferences.putUChar("led_g", color[1]);
    preferences.putUChar("led_b", color[2]);
    schedule_commit();
}

void Nvs::sync_brightness(uint8_t brightness) {
    DBG_PRINTF(Nvs, "sync_brightness(): brightness=%u\n", brightness);
    if (!initialized) return;
    preferences.putUChar("led_bri", brightness);
    schedule_commit();
}

void Nvs::sync_state(bool state) {
    DBG_PRINTF(Nvs, "sync_state(): state=%s\n", state ? "true" : "false");
    if (!initialized) return;
    preferences.putBool("led_state", state);
    schedule_commit();
}

void Nvs::sync_mode(uint8_t mode_id, String mode_name) {
    DBG_PRINTF(Nvs, "sync_mode(): mode_id=%u, mode_name='%s'\n", mode_id, mode_name.c_str());
    if (!initialized) return;
    preferences.putUChar("led_mode", mode_id);
    preferences.putString("led_mname", mode_name);
    schedule_commit();
}

void Nvs::sync_length(uint16_t length) {
    DBG_PRINTF(Nvs, "sync_length(): length=%u\n", length);
    if (!initialized) return;
    preferences.putUShort("led_len", length);
    schedule_commit();
}

void Nvs::sync_all(std::array<uint8_t, 3> color, uint8_t brightness, bool state,
                      uint8_t mode_id, String mode_name, uint16_t length) {
    DBG_PRINTF(Nvs, "sync_all(): r=%u, g=%u, b=%u, brightness=%u, state=%s, mode_id=%u, mode_name='%s', length=%u\n",
        color[0], color[1], color[2], brightness, state ? "true" : "false", mode_id, mode_name.c_str(), length);

    if (!initialized) {
        DBG_PRINTLN(Nvs, "sync_all(): ERROR - Not initialized.");
        return;
    }

    preferences.putUChar("led_r", color[0]);
    preferences.putUChar("led_g", color[1]);
    preferences.putUChar("led_b", color[2]);
    preferences.putUChar("led_bri", brightness);
    preferences.putBool("led_state", state);
    preferences.putUChar("led_mode", mode_id);
    preferences.putString("led_mname", mode_name);
    preferences.putUShort("led_len", length);

    schedule_commit();
    commit();
}

void Nvs::status() {

}

void Nvs::reset() {
    DBG_PRINTLN(Nvs, "reset(): Clearing all keys in the current namespace.");
    if (!initialized) {
        DBG_PRINTLN(Nvs, "reset(): ERROR - Not initialized.");
        return;
    }
    preferences.clear();
    schedule_commit();
    commit();
}
