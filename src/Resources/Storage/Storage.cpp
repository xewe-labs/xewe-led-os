// Storage.cpp

#include "Storage.h"

Storage::Storage() : is_nvs_initialized(false) {
    // (No need for a debug print here, since init() will print out anything important.)
}

bool Storage::init() {
    DBG_PRINTF(Storage, "init(): Attempting to open NVS namespace \"%s\" (read/write)\n", NVS_NAMESPACE);
    // Open (or create) the NVS namespace “storage” for read/write.
    is_nvs_initialized = prefs.begin(NVS_NAMESPACE, /*readOnly=*/false);
    if (!is_nvs_initialized) {
        DBG_PRINTF(Storage, "init(): prefs.begin(...) FAILED\n");
        Serial.println("Failed to initialize NVS (Preferences).");
    } else {
        DBG_PRINTF(Storage, "init(): prefs.begin(...) SUCCEEDED\n");
    }
    return is_nvs_initialized;
}

bool Storage::read_value(const char* key, String& data) {
    DBG_PRINTF(Storage, "read_value(): Called for key=\"%s\"\n", key);
    if (!is_nvs_initialized) {
        DBG_PRINTF(Storage, "read_value(): NVS not initialized → returning false\n");
        return false;
    }

    // Check if the key exists by asking for its byte length.
    size_t required = prefs.getBytesLength(key);
    if (required == 0) {
        DBG_PRINTF(Storage, "read_value(): Key \"%s\" not found (getBytesLength == 0)\n", key);
        return false;
    }

    // Now actually read it back as a String.
    data = prefs.getString(key, "");
    DBG_PRINTF(Storage, "read_value(): Found key \"%s\", value=\"%s\"\n", key, data.c_str());
    return true;
}

bool Storage::write_value(const char* key, const char* data) {
    DBG_PRINTF(Storage, "write_value(): Attempting to write key=\"%s\", value=\"%s\"\n", key, data);
    if (!is_nvs_initialized) {
        DBG_PRINTF(Storage, "write_value(): NVS not initialized → returning false\n");
        return false;
    }

    bool ok = prefs.putString(key, data);
    if (!ok) {
        DBG_PRINTF(Storage, "write_value(): putString(\"%s\") FAILED\n", key);
    } else {
        DBG_PRINTF(Storage, "write_value(): putString(\"%s\") SUCCEEDED\n", key);
    }
    return ok;
}

bool Storage::is_first_startup() {
    DBG_PRINTF(Storage, "is_first_startup(): Entry\n");
    if (!is_nvs_initialized) {
        DBG_PRINTF(Storage, "is_first_startup(): NVS not initialized → treating as first startup\n");
        return true;
    }

    String content;
    bool haveFlag = read_value(INIT_FLAG_KEY, content);

    if (!haveFlag) {
        // No flag in NVS yet ⇒ first startup. Create it with “1” and return true.
        DBG_PRINTF(Storage, "is_first_startup(): No existing flag → writing \"%s\" for key \"%s\"\n", "1", INIT_FLAG_KEY);
        if (!write_value(INIT_FLAG_KEY, "1")) {
            DBG_PRINTF(Storage, "is_first_startup(): Failed to create startup flag in NVS\n");
            Serial.println("Failed to create startup flag in NVS.");
        } else {
            DBG_PRINTF(Storage, "is_first_startup(): Successfully wrote first-time flag\n");
        }
        return true;
    }

    // We did read something, so sanitize & inspect:
    content.trim();
    DBG_PRINTF(Storage, "is_first_startup(): Read existing flag value=\"%s\"\n", content.c_str());

    if (content == "1") {
        DBG_PRINTF(Storage, "is_first_startup(): Value == \"1\" → returning true\n");
        return true;
    } else if (content == "0") {
        DBG_PRINTF(Storage, "is_first_startup(): Value == \"0\" → returning false\n");
        return false;
    } else {
        // Unexpected/invalid value ⇒ overwrite with “1” and treat as first startup.
        DBG_PRINTF(Storage, "is_first_startup(): Invalid flag \"%s\" → resetting to \"1\"\n", content.c_str());
        if (!write_value(INIT_FLAG_KEY, "1")) {
            DBG_PRINTF(Storage, "is_first_startup(): Failed to reset invalid startup flag\n");
            Serial.println("Failed to reset invalid startup flag in NVS.");
        } else {
            DBG_PRINTF(Storage, "is_first_startup(): Successfully reset invalid flag to \"1\"\n");
        }
        return true;
    }
}

bool Storage::reset_first_startup_flag() {
    DBG_PRINTF(Storage, "reset_first_startup_flag(): Entry\n");
    if (!is_nvs_initialized) {
        DBG_PRINTF(Storage, "reset_first_startup_flag(): NVS not initialized → returning false\n");
        return false;
    }
    DBG_PRINTF(Storage, "reset_first_startup_flag(): Writing \"0\" for key \"%s\"\n", INIT_FLAG_KEY);
    bool result = write_value(INIT_FLAG_KEY, "0");
    DBG_PRINTF(Storage, "reset_first_startup_flag(): Result = %s\n", result ? "true" : "false");
    return result;
}

bool Storage::set_first_startup_flag() {
    DBG_PRINTF(Storage, "set_first_startup_flag(): Entry\n");
    if (!is_nvs_initialized) {
        DBG_PRINTF(Storage, "set_first_startup_flag(): NVS not initialized → returning false\n");
        return false;
    }
    DBG_PRINTF(Storage, "set_first_startup_flag(): Writing \"1\" for key \"%s\"\n", INIT_FLAG_KEY);
    bool result = write_value(INIT_FLAG_KEY, "1");
    DBG_PRINTF(Storage, "set_first_startup_flag(): Result = %s\n", result ? "true" : "false");
    return result;
}
