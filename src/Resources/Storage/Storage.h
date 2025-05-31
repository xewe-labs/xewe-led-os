// Storage.h

#ifndef STORAGE_H
#define STORAGE_H

#include <Preferences.h>
#include <Arduino.h>
#include "../../Debug.h"

/// NVS “namespace” under which all keys will be stored
#define NVS_NAMESPACE      "storage"

/// The key we use for the “first startup” flag
#define INIT_FLAG_KEY      "init_startup_flag"

class Storage {
public:
    Storage();

    /// Call once at startup to initialize NVS.
    /// Returns true if NVS opened successfully.
    bool init();

    /// Read a string value from NVS.  Key is any null-terminated C string.
    /// On success, returns true and writes the stored value into `data`.
    /// If the key does not exist or NVS isn’t initialized, returns false.
    bool read_value(const char* key, String& data);

    /// Write a (C-string) value into NVS under `key`.
    /// Returns true if the write succeeded (and NVS is initialized).
    bool write_value(const char* key, const char* data);

    /// Returns true if this is the first time the device is starting up.
    /// Internally, it checks `INIT_FLAG_KEY` in NVS:
    ///   • if the key does not yet exist, it writes “1” and returns true.
    ///   • if the stored value is “1”, returns true.
    ///   • if the stored value is “0”, returns false.
    ///   • if the stored value is anything else, resets it to “1” and returns true.
    bool is_first_startup();

    /// Force the “first startup” flag to 0 (i.e. not first startup).
    /// Returns true on success.
    bool reset_first_startup_flag();

    /// Force the “first startup” flag to 1 (i.e. first startup).
    /// Returns true on success.
    bool set_first_startup_flag();

private:
    Preferences prefs;
    bool is_nvs_initialized = false;
};

#endif // STORAGE_H
