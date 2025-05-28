// memory.cpp
#include "memory.h"

static constexpr uint8_t WIFI_WORD_LEN_MAX       = 32;

// EEPROM address map (bytes)
static constexpr int ADDR_WIFI_FLAGS             = 0;
static constexpr int ADDR_WIFI_NAME              = ADDR_WIFI_FLAGS + 1;
static constexpr int ADDR_WIFI_PASS              = ADDR_WIFI_NAME + 1 + WIFI_WORD_LEN_MAX;

// LED strip settings (1 byte each)
static constexpr int ADDR_LED_STRIP_MODE         = ADDR_WIFI_PASS + WIFI_WORD_LEN_MAX + 1;
static constexpr int ADDR_LED_STRIP_R            = ADDR_LED_STRIP_MODE + 1;
static constexpr int ADDR_LED_STRIP_G            = ADDR_LED_STRIP_R + 1;
static constexpr int ADDR_LED_STRIP_B            = ADDR_LED_STRIP_G + 1;
static constexpr int ADDR_LED_STRIP_BRIGHTNESS   = ADDR_LED_STRIP_B + 1;
static constexpr int ADDR_LED_STRIP_STATE        = ADDR_LED_STRIP_BRIGHTNESS + 1;

// New: 16-bit length and 8-bit pin
static constexpr int ADDR_LED_STRIP_LENGTH       = ADDR_LED_STRIP_STATE   + 1;
static constexpr int ADDR_LED_STRIP_PIN          = ADDR_LED_STRIP_LENGTH  + sizeof(uint16_t);

static int get_address(const String& key) {
    DBG_PRINTF(Memory, "get_address(\"%s\") called\n", key.c_str());
    int addr;
         if (key == "wifi_flags")            addr = ADDR_WIFI_FLAGS;
    else if (key == "wifi_name")             addr = ADDR_WIFI_NAME;
    else if (key == "wifi_pass")             addr = ADDR_WIFI_PASS;
    else if (key == "led_strip_mode")        addr = ADDR_LED_STRIP_MODE;
    else if (key == "led_strip_r")           addr = ADDR_LED_STRIP_R;
    else if (key == "led_strip_g")           addr = ADDR_LED_STRIP_G;
    else if (key == "led_strip_b")           addr = ADDR_LED_STRIP_B;
    else if (key == "led_strip_brightness")  addr = ADDR_LED_STRIP_BRIGHTNESS;
    else if (key == "led_strip_state")       addr = ADDR_LED_STRIP_STATE;
    else if (key == "led_strip_length")      addr = ADDR_LED_STRIP_LENGTH;
    else if (key == "led_strip_pin")         addr = ADDR_LED_STRIP_PIN;
    else                                     addr = -1;
    DBG_PRINTF(Memory, "get_address -> %d\n", addr);
    return addr;
}

Memory::Memory(size_t size)
    : size_(size) {
    DBG_PRINTF(Memory, "Memory::Memory(size=%u) - EEPROM.begin\n", size_);
    EEPROM.begin(size_);
    DBG_PRINTLN(Memory, "Memory::Memory - EEPROM.begin done");
}

void Memory::write_str(const String& key, const String& value) {
    DBG_PRINTF(Memory, "write_str key=\"%s\", value=\"%s\"\n", key.c_str(), value.c_str());
    int addr = get_address(key);
    DBG_PRINTF(Memory, "  resolved addr=%d\n", addr);
    if (addr < 0) {
        DBG_PRINTLN(Memory, "  invalid address, abort write_str");
        return;
    }
    uint8_t length = min(static_cast<int>(value.length()), static_cast<int>(WIFI_WORD_LEN_MAX));
    DBG_PRINTF(Memory, "  length=%u\n", length);

    EEPROM.write(addr, length);
    DBG_PRINTF(Memory, "  EEPROM.write(addr=%d, length=%u)\n", addr, length);
    for (uint8_t i = 0; i < length; ++i) {
        EEPROM.write(addr + 1 + i, value[i]);
        DBG_PRINTF(Memory, "    EEPROM.write(addr=%d, value[%u]=%c)\n",
                   addr + 1 + i, i, value[i]);
    }
    EEPROM.commit();
    DBG_PRINTLN(Memory, "  EEPROM.commit()");
}

String Memory::read_str(const String& key) const {
    DBG_PRINTF(Memory, "read_str key=\"%s\"\n", key.c_str());
    int addr = get_address(key);
    DBG_PRINTF(Memory, "  resolved addr=%d\n", addr);
    if (addr < 0) {
        DBG_PRINTLN(Memory, "  invalid address, returning empty string");
        return "";
    }
    uint8_t length = EEPROM.read(addr);
    DBG_PRINTF(Memory, "  length=%u\n", length);
    String result;
    for (uint8_t i = 0; i < length; ++i) {
        char c = char(EEPROM.read(addr + 1 + i));
        result += c;
        DBG_PRINTF(Memory, "    EEPROM.read(addr=%d) -> %c\n", addr + 1 + i, c);
    }
    DBG_PRINTF(Memory, "  read_str result=\"%s\"\n", result.c_str());
    return result;
}

void Memory::write_uint8(const String& key, uint8_t value) {
    DBG_PRINTF(Memory, "write_uint8 key=\"%s\", value=%u\n", key.c_str(), value);
    int addr = get_address(key);
    DBG_PRINTF(Memory, "  resolved addr=%d\n", addr);
    if (addr < 0) {
        DBG_PRINTLN(Memory, "  invalid address, abort write_uint8");
        return;
    }

    EEPROM.write(addr, value);
    DBG_PRINTF(Memory, "  EEPROM.write(addr=%d, value=%u)\n", addr, value);
    EEPROM.commit();
    DBG_PRINTLN(Memory, "  EEPROM.commit()");
}

uint8_t Memory::read_uint8(const String& key) const {
    DBG_PRINTF(Memory, "read_uint8 key=\"%s\"\n", key.c_str());
    int addr = get_address(key);
    DBG_PRINTF(Memory, "  resolved addr=%d\n", addr);
    if (addr < 0) {
        DBG_PRINTLN(Memory, "  invalid address, returning 0");
        return 0;
    }

    uint8_t value = EEPROM.read(addr);
    DBG_PRINTF(Memory, "  EEPROM.read(addr=%d) -> %u\n", addr, value);
    return value;
}

void Memory::write_bit(const String& key, uint8_t bit, bool value) {
    DBG_PRINTF(Memory, "write_bit key=\"%s\", bit=%u, value=%s\n",
               key.c_str(), bit, value ? "true" : "false");
    int addr = get_address(key);
    DBG_PRINTF(Memory, "  resolved addr=%d\n", addr);
    if (addr < 0) {
        DBG_PRINTLN(Memory, "  invalid address, abort write_bit");
        return;
    }
    uint8_t byte = EEPROM.read(addr);
    DBG_PRINTF(Memory, "  EEPROM.read(addr=%d) -> byte=0x%02X\n", addr, byte);
    if (value)      byte |=  (1 << bit);
    else            byte &= ~(1 << bit);
    DBG_PRINTF(Memory, "  modified byte=0x%02X\n", byte);
    EEPROM.write(addr, byte);
    DBG_PRINTF(Memory, "  EEPROM.write(addr=%d, byte=0x%02X)\n", addr, byte);
    EEPROM.commit();
    DBG_PRINTLN(Memory, "  EEPROM.commit()");
}

bool Memory::read_bit(const String& key, uint8_t bit) const {
    DBG_PRINTF(Memory, "read_bit key=\"%s\", bit=%u\n", key.c_str(), bit);
    int addr = get_address(key);
    DBG_PRINTF(Memory, "  resolved addr=%d\n", addr);
    if (addr < 0) {
        DBG_PRINTLN(Memory, "  invalid address, returning false");
        return false;
    }
    uint8_t byte = EEPROM.read(addr);
    bool    bit_val = (byte >> bit) & 0x01;
    DBG_PRINTF(Memory, "  EEPROM.read(addr=%d) -> byte=0x%02X, returning %s\n",
               addr, byte, bit_val ? "true" : "false");
    return bit_val;
}

void Memory::write_uint16(const String& key, uint16_t value) {
    DBG_PRINTF(Memory, "write_uint16 key=\"%s\", value=%u\n", key.c_str(), value);
    int addr = get_address(key);
    DBG_PRINTF(Memory, "  resolved addr=%d\n", addr);
    if (addr < 0) {
        DBG_PRINTLN(Memory, "  invalid address, abort write_uint16");
        return;
    }
    uint8_t low  = static_cast<uint8_t>(value & 0xFF);
    uint8_t high = static_cast<uint8_t>((value >> 8) & 0xFF);
    DBG_PRINTF(Memory, "  low=0x%02X, high=0x%02X\n", low, high);
    // store little-endian: low byte then high byte
    EEPROM.write(addr,     low);
    DBG_PRINTF(Memory, "  EEPROM.write(addr=%d, low=0x%02X)\n", addr, low);
    EEPROM.write(addr + 1, high);
    DBG_PRINTF(Memory, "  EEPROM.write(addr=%d, high=0x%02X)\n", addr + 1, high);
    EEPROM.commit();
    DBG_PRINTLN(Memory, "  EEPROM.commit()");
}

uint16_t Memory::read_uint16(const String& key) const {
    DBG_PRINTF(Memory, "read_uint16 key=\"%s\"\n", key.c_str());
    int addr = get_address(key);
    DBG_PRINTF(Memory, "  resolved addr=%d\n", addr);
    if (addr < 0) {
        DBG_PRINTLN(Memory, "  invalid address, returning 0");
        return 0;
    }
    uint8_t low  = EEPROM.read(addr);
    uint8_t high = EEPROM.read(addr + 1);
    uint16_t value = static_cast<uint16_t>(high << 8) | low;
    DBG_PRINTF(Memory, "  EEPROM.read low=0x%02X, high=0x%02X -> value=%u\n",
               low, high, value);
    return value;
}

// Reset entire EEPROM memory
void Memory::reset() {
    DBG_PRINTLN(Memory, "reset() - clearing entire EEPROM");
    for (size_t i = 0; i < size_; ++i) {
        EEPROM.write(i, 0);
        DBG_PRINTF(Memory, "  EEPROM.write(addr=%u, 0)\n", (unsigned)i);
    }
    EEPROM.commit();
    DBG_PRINTLN(Memory, "  EEPROM.commit()");
}
