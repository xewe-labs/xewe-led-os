#include "Memory.h"
#include "../../SystemController/SystemController.h" // Required for controller reference

// ~~~~~~~~~~~~~~~~~~ Memory Class Implementation ~~~~~~~~~~~~~~~~~~

Memory::Memory(SystemController& controller_ref) : ControllerModule(controller_ref) {
    DBG_PRINTLN(Memory, "Constructor called.");
}

void Memory::begin(void* context) {
    if (!context) {
        DBG_PRINTLN(Memory, "begin(): ERROR - Namespace context is null! Cannot initialize Memory.");
        return;
    }

    const char* nvs_namespace = static_cast<const char*>(context);
    this->nvsNamespace = nvs_namespace;

    if (preferences.begin(nvs_namespace, false)) {
        initialized = true;
        DBG_PRINTF(Memory, "begin(): Preferences initialized successfully with namespace '%s'.\n", nvs_namespace);
    } else {
        initialized = false;
        DBG_PRINTF(Memory, "begin(): ERROR - Failed to initialize Preferences with namespace '%s'.\n", nvs_namespace);
    }
}

void Memory::loop() {
    if (dirty && millis() >= commit_time) {
        commit();
    }
}

void Memory::reset() {
    DBG_PRINTLN(Memory, "reset(): Clearing all keys in the current namespace.");
    if (!initialized) {
        DBG_PRINTLN(Memory, "reset(): ERROR - Not initialized.");
        return;
    }
    preferences.clear();
    schedule_commit();
}

void Memory::commit(){
    DBG_PRINTLN(Memory, "commit(): Writing changes to NVS.");
    preferences.end(); // Commits changes and closes the handle
    dirty = false;

    // Re-open the handle for subsequent operations.
    // Note: nvsNamespace is stored from begin().
    if (!preferences.begin(nvsNamespace, false)) {
        // Corrected the debug message to reflect the correct function name.
        DBG_PRINTLN(Memory, "commit(): CRITICAL ERROR - Failed to re-open Preferences after commit.");
        initialized = false;
    }
}

void Memory::schedule_commit() {
    if (!initialized) return;
    DBG_PRINTLN(Memory, "schedule_commit(): Change detected, scheduling a commit.");
    dirty = true;
    commit_time = millis() + COMMIT_DELAY_MS;
}

// --- Generic NVS Read/Write Methods ---

void Memory::write_str(const char* key, const String& value) {
    DBG_PRINTF(Memory, "write_str(): key='%s', value='%s'\n", key, value.c_str());
    if (!initialized) return;
    preferences.putString(key, value);
    schedule_commit();
}

String Memory::read_str(const char* key, const String& defaultValue) {
    DBG_PRINTF(Memory, "read_str(): key='%s', default='%s'\n", key, defaultValue.c_str());
    if (!initialized) return defaultValue;
    String value = preferences.getString(key, defaultValue);
    DBG_PRINTF(Memory, " -> read_str() returned: '%s'\n", value.c_str());
    return value;
}

void Memory::write_uint8(const char* key, uint8_t value) {
    DBG_PRINTF(Memory, "write_uint8(): key='%s', value=%u\n", key, value);
    if (!initialized) return;
    preferences.putUChar(key, value);
    schedule_commit();
}

uint8_t Memory::read_uint8(const char* key, uint8_t defaultValue) {
    DBG_PRINTF(Memory, "read_uint8(): key='%s', default=%u\n", key, defaultValue);
    if (!initialized) return defaultValue;
    uint8_t value = preferences.getUChar(key, defaultValue);
    DBG_PRINTF(Memory, " -> read_uint8() returned: %u\n", value);
    return value;
}

void Memory::write_uint16(const char* key, uint16_t value) {
    DBG_PRINTF(Memory, "write_uint16(): key='%s', value=%u\n", key, value);
    if (!initialized) return;
    preferences.putUShort(key, value);
    schedule_commit();
}

uint16_t Memory::read_uint16(const char* key, uint16_t defaultValue) {
    DBG_PRINTF(Memory, "read_uint16(): key='%s', default=%u\n", key, defaultValue);
    if (!initialized) return defaultValue;
    uint16_t value = preferences.getUShort(key, defaultValue);
    DBG_PRINTF(Memory, " -> read_uint16() returned: %u\n", value);
    return value;
}

void Memory::write_bool(const char* key, bool value) {
    DBG_PRINTF(Memory, "write_bool(): key='%s', value=%s\n", key, value ? "true" : "false");
    if (!initialized) return;
    preferences.putBool(key, value);
    schedule_commit();
}

bool Memory::read_bool(const char* key, bool defaultValue) {
    DBG_PRINTF(Memory, "read_bool(): key='%s', default=%s\n", key, defaultValue ? "true" : "false");
    if (!initialized) return defaultValue;
    bool value = preferences.getBool(key, defaultValue);
    DBG_PRINTF(Memory, " -> read_bool() returned: %s\n", value ? "true" : "false");
    return value;
}

// ~~~~~~~~~~~~~~~~~~ Sync Methods (System -> Memory) ~~~~~~~~~~~~~~~~~~

void Memory::sync_color(std::array<uint8_t, 3> color) {
    DBG_PRINTF(Memory, "sync_color(): r=%u, g=%u, b=%u\n", color[0], color[1], color[2]);
    if (!initialized) return;
    preferences.putUChar("led_r", color[0]);
    preferences.putUChar("led_g", color[1]);
    preferences.putUChar("led_b", color[2]);
    schedule_commit();
}

void Memory::sync_brightness(uint8_t brightness) {
    DBG_PRINTF(Memory, "sync_brightness(): brightness=%u\n", brightness);
    if (!initialized) return;
    preferences.putUChar("led_bri", brightness);
    schedule_commit();
}

void Memory::sync_state(bool state) {
    DBG_PRINTF(Memory, "sync_state(): state=%s\n", state ? "true" : "false");
    if (!initialized) return;
    preferences.putBool("led_state", state);
    schedule_commit();
}

void Memory::sync_mode(uint8_t mode_id, String mode_name) {
    DBG_PRINTF(Memory, "sync_mode(): mode_id=%u, mode_name='%s'\n", mode_id, mode_name.c_str());
    if (!initialized) return;
    preferences.putUChar("led_mode", mode_id);
    preferences.putString("led_mname", mode_name);
    schedule_commit();
}

void Memory::sync_length(uint16_t length) {
    DBG_PRINTF(Memory, "sync_length(): length=%u\n", length);
    if (!initialized) return;
    preferences.putUShort("led_len", length);
    schedule_commit();
}

void Memory::sync_all(std::array<uint8_t, 3> color, uint8_t brightness, bool state,
                      uint8_t mode_id, String mode_name, uint16_t length) {
    DBG_PRINTF(Memory, "sync_all(): r=%u, g=%u, b=%u, brightness=%u, state=%s, mode_id=%u, mode_name='%s', length=%u\n",
        color[0], color[1], color[2], brightness, state ? "true" : "false", mode_id, mode_name.c_str(), length);

    if (!initialized) {
        DBG_PRINTLN(Memory, "sync_all(): ERROR - Not initialized.");
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
    commit(); // sync_all performs an immediate commit
}
