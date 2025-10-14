// src/Interfaces/Nvs/Nvs.h
#pragma once

#include "../../Interface/Interface.h"
#include "../../../Config.h"

#include "../../../Debug.h"

#include <Preferences.h>
#include <array>
#include <string_view>
#include <string>


struct NvsConfig : public ModuleConfig {};


class Nvs : public Interface {
public:
    explicit                    Nvs                         (SystemController& controller);

    // required implementation
    void                        sync_color                  (std::array<uint8_t,3> color)   override;
    void                        sync_brightness             (uint8_t brightness)            override;
    void                        sync_state                  (uint8_t state)                 override;
    void                        sync_mode                   (uint8_t mode)                  override;
    void                        sync_length                 (uint16_t length)               override;

    // optional implementation
    void                        reset                       (const bool verbose=false)      override;

    // other methods
    void                        sync_from_memory            (std::array<uint8_t,5> sync_flags);
    void                        write_str                   (std::string_view ns,
                                                             std::string_view key,
                                                             std::string_view value);
    void                        write_uint8                 (std::string_view ns,
                                                             std::string_view key,
                                                             uint8_t value);
    void                        write_uint16                (std::string_view ns,
                                                             std::string_view key,
                                                             uint16_t value);
    void                        write_bool                  (std::string_view ns,
                                                             std::string_view key,
                                                             bool value);
    void                        remove                      (std::string_view ns,
                                                             std::string_view key);

    // Generic NVS Read Methods
    std::string                 read_str                    (std::string_view ns,
                                                             std::string_view key,
                                                             std::string_view default_value = "");
    uint8_t                     read_uint8                  (std::string_view ns,
                                                             std::string_view key,
                                                             uint8_t default_value = 0);
    uint16_t                    read_uint16                 (std::string_view ns,
                                                             std::string_view key,
                                                             uint16_t default_value = 0);
    bool                        read_bool                   (std::string_view ns,
                                                             std::string_view key,
                                                               bool default_value = false);

private:
    static constexpr size_t     MAX_KEY_LEN                 = 15;
    Preferences                 preferences;
    std::string                 full_key                    (std::string_view ns,
                                                             std::string_view key) const;
};
