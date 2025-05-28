#include "Storage.h"

Storage::Storage() {}

bool Storage::init() {
    is_spiffs_initialized = SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED);
    return is_spiffs_initialized;
}

bool Storage::read_file(const char* filename, String& data) {
    if (!is_spiffs_initialized) return false;

    File file = SPIFFS.open(filename, FILE_READ);
    if (!file) {
        return false;
    }
    data = file.readString();
    file.close();
    return true;
}

bool Storage::write_file(const char* filename, const char* data) {
    if (!is_spiffs_initialized) return false;

    File file = SPIFFS.open(filename, FILE_WRITE);
    if (!file) {
        return false;
    }
    bool success = file.print(data);
    file.close();
    return success;
}

bool Storage::is_first_startup() {
    if (!is_spiffs_initialized) return true; // Assume first startup if not initialized
    String content;

    if (!read_file(INIT_FILE_PATH, content)) {
        if (!write_file(INIT_FILE_PATH, "1")) {
            Serial.println("Failed to create startup flag file.");
        }
        return true;
    }

    content.trim();

    if (content == "1") {
        return true;
    } else if (content == "0") {
        return false;
    } else {
        if (!write_file(INIT_FILE_PATH, "1")) {
            Serial.println("Failed to reset invalid startup flag.");
        }
        return true;
    }
}

bool Storage::reset_first_startup_flag() {
    if (!is_spiffs_initialized) return false;
    bool result = write_file(INIT_FILE_PATH, "0");
    return result;
}
