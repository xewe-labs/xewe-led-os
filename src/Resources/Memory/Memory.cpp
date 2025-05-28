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
    if      (key == "wifi_flags")            return ADDR_WIFI_FLAGS;
    else if (key == "wifi_name")             return ADDR_WIFI_NAME;
    else if (key == "wifi_pass")             return ADDR_WIFI_PASS;
    else if (key == "led_strip_mode")        return ADDR_LED_STRIP_MODE;
    else if (key == "led_strip_r")           return ADDR_LED_STRIP_R;
    else if (key == "led_strip_g")           return ADDR_LED_STRIP_G;
    else if (key == "led_strip_b")           return ADDR_LED_STRIP_B;
    else if (key == "led_strip_brightness")  return ADDR_LED_STRIP_BRIGHTNESS;
    else if (key == "led_strip_state")       return ADDR_LED_STRIP_STATE;
    else if (key == "led_strip_length")      return ADDR_LED_STRIP_LENGTH;
    else if (key == "led_strip_pin")         return ADDR_LED_STRIP_PIN;
    return -1;
}

Memory::Memory(size_t size)
    : size_(size) {
    EEPROM.begin(size_);
}

void Memory::write_str(const String& key, const String& value) {
    int addr = get_address(key);
    if (addr < 0) return;
    uint8_t length = min(static_cast<int>(value.length()), static_cast<int>(WIFI_WORD_LEN_MAX));
    EEPROM.write(addr, length);
    for (uint8_t i = 0; i < length; ++i) {
        EEPROM.write(addr + 1 + i, value[i]);
    }
    EEPROM.commit();
}

String Memory::read_str(const String& key) const {
    int addr = get_address(key);
    if (addr < 0) return "";
    uint8_t length = EEPROM.read(addr);
    String result;
    for (uint8_t i = 0; i < length; ++i) {
        result += char(EEPROM.read(addr + 1 + i));
    }
    return result;
}

void Memory::write_uint8(const String& key, uint8_t value) {
    int addr = get_address(key);
    if (addr < 0) return;

    EEPROM.write(addr, value);
    EEPROM.commit();
}

uint8_t Memory::read_uint8(const String& key) const {
    int addr = get_address(key);
    if (addr < 0) return 0;

    return EEPROM.read(addr);
}

void Memory::write_bit(const String& key, uint8_t bit, bool value) {
    int addr = get_address(key);
    if (addr < 0) return;
    uint8_t byte = EEPROM.read(addr);
    if (value)      byte |=  (1 << bit);
    else            byte &= ~(1 << bit);
    EEPROM.write(addr, byte);
    EEPROM.commit();
}

bool Memory::read_bit(const String& key, uint8_t bit) const {
    int addr = get_address(key);
    if (addr < 0) return false;
    uint8_t byte = EEPROM.read(addr);
    return (byte >> bit) & 0x01;
}

void Memory::write_uint16(const String& key, uint16_t value) {
    int addr = get_address(key);
    if (addr < 0) return;
    // store little-endian: low byte then high byte
    EEPROM.write(addr, static_cast<uint8_t>(value & 0xFF));
    EEPROM.write(addr + 1, static_cast<uint8_t>((value >> 8) & 0xFF));
    EEPROM.commit();
}

uint16_t Memory::read_uint16(const String& key) const {
    int addr = get_address(key);
    if (addr < 0) return 0;
    uint8_t low  = EEPROM.read(addr);
    uint8_t high = EEPROM.read(addr + 1);
    return static_cast<uint16_t>(high << 8) | low;
}

// Reset entire EEPROM memory
void Memory::reset() {
    for (size_t i = 0; i < size_; ++i) {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
}
