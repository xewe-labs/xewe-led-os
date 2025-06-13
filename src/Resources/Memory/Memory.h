#ifndef MEMORY_H
#define MEMORY_H

#include <Arduino.h>
#include <Preferences.h> // Use the Preferences library for NVS
#include "../../Debug.h" // Assuming this path is correct for your project

class Memory {
public:
    explicit Memory             ();
    ~Memory                     ();

    bool                begin           (const char* ns = "lsys_store");
    void                end             ();
    void                reset           ();
    bool                commit          ();

    void                write_str       (const String& key, const String& value);
    String              read_str        (const String& key, const String& defaultValue = "");
    void                write_bit       (const String& key, uint8_t bit, bool value);
    bool                read_bit        (const String& key, uint8_t bit);
    void                write_uint8     (const String& key, uint8_t value);
    uint8_t             read_uint8      (const String& key, uint8_t defaultValue = 0);
    void                write_uint16    (const String& key, uint16_t value);
    uint16_t            read_uint16     (const String& key, uint16_t defaultValue = 0);
    void                write_bool      (const String& key, bool value);
    bool                read_bool       (const String& key, bool defaultValue = false);

    bool                is_initialised  () const { return initialized; }

private:
    Preferences         preferences_;
    const char*         namespace_;
    bool                initialized = false;
};

#endif // MEMORY_H