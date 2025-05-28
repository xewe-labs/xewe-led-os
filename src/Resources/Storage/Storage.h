#ifndef STORAGE_H
#define STORAGE_H

#include "FS.h"
#include "SPIFFS.h"

#define FORMAT_SPIFFS_IF_FAILED true
#define INIT_FILE_PATH "/init_startup_flag.txt"

class Storage {
public:
    Storage();

    bool init();
    bool read_file(const char* filename, String& data);
    bool write_file(const char* filename, const char* data);

    bool is_first_startup();
    bool reset_first_startup_flag();
    bool set_first_startup_flag();

private:
    bool is_spiffs_initialized = false;
};

#endif // STORAGE_H
