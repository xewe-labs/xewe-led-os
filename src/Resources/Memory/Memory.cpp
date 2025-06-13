#include "memory.h"

Memory::Memory()
    : initialized(false) {
    DBG_PRINTF(Memory, "Memory::Memory() created.\n");
}

Memory::~Memory() {
    DBG_PRINTF(Memory, "Memory::~Memory() - Destructor called for namespace \"%s\".\n", namespace_);
    if (initialized) {
        preferences_.end();
        DBG_PRINTLN(Memory, "  Preferences closed on destruction.");
        initialized = false;
    }
}

bool Memory::begin(const char* ns) {
    namespace_ = ns;
    if (initialized) {
        DBG_PRINTF(Memory, "Memory::begin() - Preferences for namespace \"%s\" already initialized.\n", namespace_);
        return true;
    }

    DBG_PRINTF(Memory, "Memory::begin() - Initializing preferences with namespace: %s\n", namespace_);
    if (preferences_.begin(namespace_, false)) {
        initialized = true;
        DBG_PRINTLN(Memory, "Memory::begin - Preferences initialized successfully.");
        return true;
    } else {
        DBG_PRINTLN(Memory, "Memory::begin - Failed to initialize preferences!");
        initialized = false;
        return false;
    }
}

void Memory::end() {
    if (initialized) {
        DBG_PRINTF(Memory, "Memory::end() - Closing preferences for namespace \"%s\".\n", namespace_);
        preferences_.end();
        initialized = false;
    } else {
        DBG_PRINTLN(Memory, "Memory::end() - Not initialized, nothing to close.");
    }
}

bool Memory::commit() {
    if (!initialized) {
        DBG_PRINTLN(Memory, "Error: Preferences not initialized. Cannot commit.");
        return false;
    }
    DBG_PRINTF(Memory, "Memory::commit() - Committing changes for namespace \"%s\" to NVS.\n", namespace_);
    
    // preferences_.end() performs the commit.
    // We need to re-open immediately if we intend to continue using the instance.
    preferences_.end(); // This closes the NVS handle and flushes buffered data to flash
    
    DBG_PRINTLN(Memory, "  Changes committed successfully.");
    // Re-open the preferences handle immediately so subsequent calls to write/read work
    if (preferences_.begin(namespace_, false)) {
        initialized = true; // Ensure state is correct
        DBG_PRINTLN(Memory, "  Preferences re-initialized successfully after commit.");
        return true;
    } else {
        DBG_PRINTLN(Memory, "Error: Failed to re-initialize preferences after commit!");
        initialized = false; // Mark as uninitialized to prevent further errors
        return false; // Critical failure
    }
}

void Memory::write_str(const String& key, const String& value) {
    if (!initialized) {
        DBG_PRINTLN(Memory, "Error: Preferences not initialized. Call begin() first.");
        return;
    }
    DBG_PRINTF(Memory, "write_str key=\"%s\", value=\"%s\"\n", key.c_str(), value.c_str());
    if (!preferences_.putString(key.c_str(), value)) {
        DBG_PRINTF(Memory, "Error: Failed to write String key \"%s\".\n", key.c_str());
    }
}

String Memory::read_str(const String& key, const String& defaultValue) {
    if (!initialized) {
        DBG_PRINTLN(Memory, "Error: Preferences not initialized. Call begin() first.");
        return defaultValue;
    }
    String value = preferences_.getString(key.c_str(), defaultValue);
    DBG_PRINTF(Memory, "read_str key=\"%s\" -> \"%s\"\n", key.c_str(), value.c_str());
    return value;
}

void Memory::write_uint8(const String& key, uint8_t value) {
    if (!initialized) {
        DBG_PRINTLN(Memory, "Error: Preferences not initialized. Call begin() first.");
        return;
    }
    DBG_PRINTF(Memory, "write_uint8 key=\"%s\", value=%u\n", key.c_str(), value);
    if (!preferences_.putUChar(key.c_str(), value)) {
        DBG_PRINTF(Memory, "Error: Failed to write uint8 key \"%s\".\n", key.c_str());
    }
}

uint8_t Memory::read_uint8(const String& key, uint8_t defaultValue) {
    if (!initialized) {
        DBG_PRINTLN(Memory, "Error: Preferences not initialized. Call begin() first.");
        return defaultValue;
    }
    uint8_t value = preferences_.getUChar(key.c_str(), defaultValue);
    DBG_PRINTF(Memory, "read_uint8 key=\"%s\" -> %u\n", key.c_str(), value);
    return value;
}

void Memory::write_bit(const String& key, uint8_t bit, bool value) {
    if (!initialized) {
        DBG_PRINTLN(Memory, "Error: Preferences not initialized. Call begin() first.");
        return;
    }
    if (bit >= 8) {
        DBG_PRINTLN(Memory, "Error: Bit position must be between 0 and 7.");
        return;
    }
    DBG_PRINTF(Memory, "write_bit key=\"%s\", bit=%u, value=%s\n", key.c_str(), bit, value ? "true" : "false");

    // NVS doesn't have native bit-level access, so we read-modify-write the byte.
    // Read the existing byte (or 0 if not found)
    uint8_t byte = preferences_.getUChar(key.c_str(), 0); 
    DBG_PRINTF(Memory, "  read existing byte -> 0x%02X\n", byte);

    if (value) {
        byte |= (1 << bit); // Set the bit
    } else {
        byte &= ~(1 << bit); // Clear the bit
    }
    DBG_PRINTF(Memory, "  modified byte -> 0x%02X\n", byte);

    if (!preferences_.putUChar(key.c_str(), byte)) {
        DBG_PRINTF(Memory, "Error: Failed to write modified byte for key \"%s\" (bit operation).\n", key.c_str());
    }
}

bool Memory::read_bit(const String& key, uint8_t bit) {
    if (!initialized) {
        DBG_PRINTLN(Memory, "Error: Preferences not initialized. Call begin() first.");
        return false;
    }
    if (bit >= 8) {
        DBG_PRINTLN(Memory, "Error: Bit position must be between 0 and 7.");
        return false;
    }
    uint8_t byte = preferences_.getUChar(key.c_str(), 0);
    bool bit_val = (byte >> bit) & 0x01;
    DBG_PRINTF(Memory, "read_bit key=\"%s\", bit=%u -> byte=0x%02X, returning %s\n", key.c_str(), bit, byte, bit_val ? "true" : "false");
    return bit_val;
}

void Memory::write_uint16(const String& key, uint16_t value) {
    if (!initialized) {
        DBG_PRINTLN(Memory, "Error: Preferences not initialized. Call begin() first.");
        return;
    }
    DBG_PRINTF(Memory, "write_uint16 key=\"%s\", value=%u\n", key.c_str(), value);
    if (!preferences_.putUShort(key.c_str(), value)) {
        DBG_PRINTF(Memory, "Error: Failed to write uint16 key \"%s\".\n", key.c_str());
    }
}

uint16_t Memory::read_uint16(const String& key, uint16_t defaultValue) {
    if (!initialized) {
        DBG_PRINTLN(Memory, "Error: Preferences not initialized. Call begin() first.");
        return defaultValue;
    }
    uint16_t value = preferences_.getUShort(key.c_str(), defaultValue);
    DBG_PRINTF(Memory, "read_uint16 key=\"%s\" -> %u\n", key.c_str(), value);
    return value;
}

void Memory::write_bool(const String& key, bool value) {
    if (!initialized) {
        DBG_PRINTLN(Memory, "Error: Preferences not initialized. Call begin() first.");
        return;
    }
    DBG_PRINTF(Memory, "write_bool key=\"%s\", value=%s\n", key.c_str(), value ? "true" : "false");
    if (!preferences_.putBool(key.c_str(), value)) {
        DBG_PRINTF(Memory, "Error: Failed to write bool key \"%s\".\n", key.c_str());
    }
}

bool Memory::read_bool(const String& key, bool defaultValue) {
    if (!initialized) {
        DBG_PRINTLN(Memory, "Error: Preferences not initialized. Call begin() first.");
        return defaultValue;
    }
    bool value = preferences_.getBool(key.c_str(), defaultValue);
    DBG_PRINTF(Memory, "read_bool key=\"%s\" -> %s\n", key.c_str(), value ? "true" : "false");
    return value;
}

// Clears all keys within the initialized namespace
void Memory::reset() {
    if (!initialized) {
        DBG_PRINTLN(Memory, "Error: Preferences not initialized. Cannot reset.");
        return;
    }
    DBG_PRINTF(Memory, "reset() - clearing all keys in namespace '%s'\n", namespace_);
    if (preferences_.clear()) {
        DBG_PRINTLN(Memory, "  Namespace cleared successfully.");
        // After clearing, we should immediately commit to persist the erase
        commit(); // This also re-opens the preferences.
    } else {
        DBG_PRINTLN(Memory, "Error: Failed to clear namespace.");
    }
}