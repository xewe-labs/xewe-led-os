// src/Interfaces/Hardware/Nvs/Nvs.h
#ifndef NVS_H
#define NVS_H

#include <Preferences.h>
#include <array>
#include <string_view>
#include <string>
#include "../../../Debug.h"
#include "../../Interface.h"

// Forward declaration
class SystemController;

struct NvsConfig : public ModuleConfig {};

/**
 * @class Nvs
 * @brief Manages persistent storage (NVS) for system settings.
 * Performs immediate writes with namespace-prefixed keys.
 */
class Nvs : public Interface {
public:
    explicit                    Nvs                 (SystemController& controller);

    void                        begin               (const ModuleConfig& cfg)               override;
    void                        loop                ()                                      override;
//    void                        enable              ()                                      override;
//    void                        disable             ()                                      override;
    void                        reset               ()                                      override;

//    std::string_view            status              (bool verbose=true)             const   override;
//    bool                        is_enabled          (bool verbose=true)             const   override;
//    bool                        is_disabled         (bool verbose=true)             const   override;

    void                        sync_color          (std::array<uint8_t, 3> color)          override;
    void                        sync_brightness     (uint8_t brightness)                    override;
    void                        sync_state          (uint8_t state)                         override;
    void                        sync_mode           (uint8_t mode)                          override;
    void                        sync_length         (uint16_t length)                       override;
    void                        sync_all            (std::array<uint8_t, 3> color,
                                                     uint8_t brightness,
                                                     uint8_t state,
                                                     uint8_t mode,
                                                     uint16_t length)                       override;

    // Generic NVS Write Methods
    void write_str(std::string_view ns, std::string_view key, std::string_view value);
    void write_uint8(std::string_view ns, std::string_view key, uint8_t value);
    void write_uint16(std::string_view ns, std::string_view key, uint16_t value);
    void write_bool(std::string_view ns, std::string_view key, bool value);
    void remove(std::string_view ns, std::string_view key);

    // Generic NVS Read Methods
    std::string read_str(std::string_view ns, std::string_view key, std::string_view default_value = "");
    uint8_t read_uint8(std::string_view ns, std::string_view key, uint8_t default_value = 0);
    uint16_t read_uint16(std::string_view ns, std::string_view key, uint16_t default_value = 0);
    bool read_bool(std::string_view ns, std::string_view key, bool default_value = false);

private:
    static constexpr size_t MAX_KEY_LEN = 15;  // ESP-IDF NVS max key length

    Preferences preferences;

    // Build full key with namespace prefix, enforcing key-length limit
    std::string full_key(std::string_view ns, std::string_view key) const;
};

#endif // NVS_H