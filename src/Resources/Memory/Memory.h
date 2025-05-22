// memory.h
#ifndef MEMORY_H
#define MEMORY_H

#include <Arduino.h>
#include <EEPROM.h>

class Memory {
public:
    explicit Memory(size_t size = 512);

    void write_str(const String& key, const String& value);
    String read_str(const String& key) const;

    void write_bit(const String& key, uint8_t bit, bool value);
    bool read_bit(const String& key, uint8_t bit) const;

    void write_uint8(const String& key, uint8_t value);
    uint8_t read_uint8(const String& key) const;

    void write_uint16(const String& key, uint16_t value);
    uint16_t read_uint16(const String& key) const;

};

#endif // MEMORY_H
