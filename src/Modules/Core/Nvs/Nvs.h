// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Core/Nvs/Nvs.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <Arduino.h>
#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>

#include "../../Module/Module.h"


struct NvsConfig : public ModuleConfig {};


class Nvs : public Module {
public:
    explicit                    Nvs                         (ModuleController& controller);

    void                        reset                       (const bool verbose=false,
                                                             const bool do_restart=true,
                                                             const bool keep_enabled=true) override;

    // atomic types
    template <typename T>
    bool                        write                       (std::string_view ns,
                                                             std::string_view key,
                                                             const T& value);

    template <typename T>
    T                           read                        (std::string_view ns,
                                                             std::string_view key,
                                                             T default_value = T());
    // blobs
    bool                        write_blob                  (std::string_view ns,
                                                             std::string_view key,
                                                             const std::vector<uint8_t>& data);

    std::vector<uint8_t>        read_blob                   (std::string_view ns,
                                                             std::string_view key);

    // FlexData
    template <typename T>
    bool                        write_flex                  (std::string_view ns,
                                                             std::string_view key,
                                                             const T& obj);

    template <typename T>
    bool                        read_flex                   (std::string_view ns,
                                                             std::string_view key,
                                                             T& out);

    // removal
    void                        remove                      (std::string_view ns,
                                                             std::string_view key);
    void                        reset_ns                    (std::string_view ns);

private:
    template <typename>
    struct always_false : std::false_type {};

    static constexpr std::size_t MAX_KEY_LEN = 15;

    struct ScopedHandle {
        nvs_handle_t handle = 0;
        ~ScopedHandle() { close(); }
        operator nvs_handle_t() const { return handle; }
        void close() {
            if (handle != 0) {
                nvs_close(handle);
                handle = 0;
            }
        }
    };

    bool                        m_nvs_ready                 = false;

    bool                        ensure_ready                ();
    esp_err_t                   open_handle                 (std::string_view ns,
                                                             nvs_open_mode_t mode,
                                                             ScopedHandle& scoped);
    bool                        commit_and_close            (ScopedHandle& scoped,
                                                             esp_err_t op_err);

    std::string                 sanitize_name               (std::string_view name) const;
};

#include "Nvs.tpp"