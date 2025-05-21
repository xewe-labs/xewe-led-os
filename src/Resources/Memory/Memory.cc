// memory.cc
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
    return -1;
}

Memory::Memory(size_t size) {
    EEPROM.begin(size);
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

void Memory::write_byte(const String& key, uint8_t value) {
    int addr = get_address(key);
    if (addr < 0) return;

    EEPROM.write(addr, value);
    EEPROM.commit();
}

uint8_t Memory::read_byte(const String& key) const {
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
