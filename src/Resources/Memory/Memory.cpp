#include "Memory.h"

// ~~~~~~~~~~~~~~~~~~ Memory Class Implementation ~~~~~~~~~~~~~~~~~~

Memory::Memory(SystemController& controller_ref) : ControllerModule(controller_ref) {
    DBG_PRINTLN(Memory, "Constructor called.");
}

void Memory::begin(void* context) {
    if (!context) {
        DBG_PRINTLN(Memory, "begin(): ERROR - Namespace context is null! Cannot initialize Memory.");
        return;
    }

    // The context is the namespace for the Preferences library.
    const char* nvs_namespace = static_cast<const char*>(context);

    if (preferences.begin(nvs_namespace, false)) {
        initialized = true;
        DBG_PRINTF(Memory, "begin(): Preferences initialized successfully with namespace '%s'.\n", nvs_namespace);
    } else {
        initialized = false;
        DBG_PRINTF(Memory, "begin(): ERROR - Failed to initialize Preferences with namespace '%s'.\n", nvs_namespace);
    }
}

void Memory::loop() {
    // If there are pending changes and the commit time has been reached, commit to NVS.
    if (dirty && millis() >= commit_time) {
        DBG_PRINTLN(Memory, "loop(): Commit timer elapsed. Writing changes to NVS.");
        preferences.end(); // Commits changes and closes the handle
        dirty = false;

        // Re-open the handle for subsequent operations
        if (!preferences.begin(preferences.getNamespace(), false)) {
            DBG_PRINTLN(Memory, "loop(): CRITICAL ERROR - Failed to re-open Preferences after commit.");
            initialized = false; // Mark as uninitialized to prevent further errors
        }
    }
}

void Memory::reset() {
    if (!initialized) {
        DBG_PRINTLN(Memory, "reset(): ERROR - Not initialized.");
        return;
    }
    DBG_PRINTLN(Memory, "reset(): Clearing all keys in the current namespace.");
    preferences.clear();
    schedule_commit(); // Ensure the clear operation is written to flash
}

void Memory::schedule_commit() {
    if (!initialized) return;
    dirty = true;
    commit_time = millis() + COMMIT_DELAY_MS;
}

// --- Generic NVS Read/Write Methods ---

void Memory::write_str(const char* key, const char* value) {
    if (!initialized) return;
    preferences.putString(key, value);
    schedule_commit();
}

String Memory::read_str(const char* key, const String& defaultValue) {
    if (!initialized) return defaultValue;
    return preferences.getString(key, defaultValue);
}

void Memory::write_uint8(const char* key, uint8_t value) {
    if (!initialized) return;
    preferences.putUChar(key, value);
    schedule_commit();
}

uint8_t Memory::read_uint8(const char* key, uint8_t defaultValue) {
    if (!initialized) return defaultValue;
    return preferences.getUChar(key, defaultValue);
}

void Memory::write_uint16(const char* key, uint16_t value) {
    if (!initialized) return;
    preferences.putUShort(key, value);
    schedule_commit();
}

uint16_t Memory::read_uint16(const char* key, uint16_t defaultValue) {
    if (!initialized) return defaultValue;
    return preferences.getUShort(key, defaultValue);
}

void Memory::write_bool(const char* key, bool value) {
    if (!initialized) return;
    preferences.putBool(key, value);
    schedule_commit();
}

bool Memory::read_bool(const char* key, bool defaultValue) {
    if (!initialized) return defaultValue;
    return preferences.getBool(key, defaultValue);
}

// ~~~~~~~~~~~~~~~~~~ Sync Methods (System -> Memory) ~~~~~~~~~~~~~~~~~~

void Memory::sync_color(std::array<uint8_t, 3> color) {
    if (!initialized) return;
    DBG_PRINTLN(Memory, "sync_color(): Syncing color to NVS.");
    preferences.putUChar("led_r", color[0]);
    preferences.putUChar("led_g", color[1]);
    preferences.putUChar("led_b", color[2]);
    schedule_commit();
}

void Memory::sync_brightness(uint8_t brightness) {
    if (!initialized) return;
    DBG_PRINTLN(Memory, "sync_brightness(): Syncing brightness to NVS.");
    preferences.putUChar("led_bri", brightness);
    schedule_commit();
}

void Memory::sync_state(bool state) {
    if (!initialized) return;
    DBG_PRINTLN(Memory, "sync_state(): Syncing state to NVS.");
    preferences.putBool("led_state", state);
    schedule_commit();
}

void Memory::sync_mode(uint8_t mode_id, String mode_name) {
    if (!initialized) return;
    DBG_PRINTLN(Memory, "sync_mode(): Syncing mode to NVS.");
    preferences.putUChar("led_mode", mode_id);
    preferences.putString("led_mname", mode_name);
    schedule_commit();
}

void Memory::sync_length(uint16_t length) {
    if (!initialized) return;
    DBG_PRINTLN(Memory, "sync_length(): Syncing length to NVS.");
    preferences.putUShort("led_len", length);
    schedule_commit();
}

void Memory::sync_all(std::array<uint8_t, 3> color, uint8_t brightness, bool state,
                      uint8_t mode_id, String mode_name, uint16_t length) {
    if (!initialized) {
        DBG_PRINTLN(Memory, "sync_all(): ERROR - Not initialized.");
        return;
    }
    DBG_PRINTLN(Memory, "sync_all(): Syncing all states to NVS.");
    preferences.putUChar("led_r", color[0]);
    preferences.putUChar("led_g", color[1]);
    preferences.putUChar("led_b", color[2]);
    preferences.putUChar("led_bri", brightness);
    preferences.putBool("led_state", state);
    preferences.putUChar("led_mode", mode_id);
    preferences.putString("led_mname", mode_name);
    preferences.putUShort("led_len", length);
    commit();
}
