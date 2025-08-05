// src/Interfaces/Hardware/Nvs/Nvs.h
#ifndef NVS_H
#define NVS_H

#include <Preferences.h>
#include <array>
#include <string_view>
#include "../../../Debug.h"
#include "../../Interface.h"

// Forward declaration
class SystemController;

/**
 * @class Nvs
 * @brief Manages persistent storage (NVS) for system settings.
 * Uses a non-blocking timer to batch writes and reduce flash wear.
 */
class Nvs : public Interface {
public:
    explicit Nvs(SystemController& controller_ref);

    // Module interface
    void begin(const ModuleConfig& cfg) override;
    void loop() override;
    void enable() override {}
    void disable() override {}
    void reset() override;

    // Status
    std::string_view status() const override;

    // Sync methods
    void sync_color(std::array<uint8_t, 3> color) override;
    void sync_brightness(uint8_t brightness) override;
    void sync_state(uint8_t state) override;
    void sync_mode(uint8_t mode) override;
    void sync_length(uint16_t length) override;
    void sync_all(std::array<uint8_t, 3> color,
                  uint8_t brightness,
                  uint8_t state,
                  uint8_t mode,
                  uint16_t length) override;

    // Generic NVS Read/Write Methods
    void write_str(std::string_view key, std::string_view value);
    const char* read_str(std::string_view key, std::string_view default_value = "");
    void write_uint8(std::string_view key, uint8_t value);
    uint8_t read_uint8(std::string_view key, uint8_t default_value = 0);
    void write_uint16(std::string_view key, uint16_t value);
    uint16_t read_uint16(std::string_view key, uint16_t default_value = 0);
    void write_bool(std::string_view key, bool value);
    bool read_bool(std::string_view key, bool default_value = false);
    void remove(std::string_view key);
    void commit();

    bool is_initialized() const { return initialized; }

private:
    void schedule_commit();

    Preferences    preferences;
    bool           initialized  = false;
    bool           dirty        = false;
    unsigned long  commit_time  = 0;

    static constexpr unsigned long COMMIT_DELAY_MS = 1000;
};

#endif // NVS_H
