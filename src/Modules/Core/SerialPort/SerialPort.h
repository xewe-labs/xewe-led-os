// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Core/SerialPort/SerialPort.h

#pragma once

#include <limits>
#include <optional>
#include <cstring>

#include "../../Module/Module.h"


struct SerialPortConfig : public ModuleConfig {
    unsigned long baud_rate = 115200;
};


class SerialPort : public Module {
public:
    explicit                    SerialPort                  (ModuleController&      controller);

    void                        begin_routines_required     (const ModuleConfig&    cfg)                    override;
    void                        loop                        ()                                              override;
    void                        reset                       (const bool             verbose=false,
                                                             const bool             do_restart=true,
                                                             const bool             keep_enabled=true)      override;
    // printers
    void                        print                       (std::string_view       message                 = {},
                                                             std::string_view       end                     = xewe::str::kCRLF,
                                                             std::string_view       edge_character          = {},
                                                             const char             text_align              = 'l',
                                                             const char             wrap_mode               = 'w',
                                                             const uint16_t         message_width           = 0,
                                                             const uint16_t         margin_l                = 0,
                                                             const uint16_t         margin_r                = 0
                                                            );
    void                        printf_fmt                  (std::string_view       end,
                                                             std::string_view       edge_character,
                                                             const char             text_align,
                                                             const char             wrap_mode,
                                                             const uint16_t         message_width,
                                                             const uint16_t         margin_l,
                                                             const uint16_t         margin_r,
                                                             const char*            fmt,
                                                             ...
                                                            );
    void                        printf                      (const char*            fmt,
                                                             ...
                                                            );
    void                        print_separator             (const uint16_t         total_width             = 50,
                                                             std::string_view       fill                    = "-",
                                                             std::string_view       edge_character          = "+"
                                                            );
    void                        print_spacer                (const uint16_t         total_width             = 50,
                                                             std::string_view       edge_character          = {}
                                                            );
    void                        print_header                (std::string_view       message,
                                                             const uint16_t         total_width             = 50,
                                                             std::string_view       edge_character          = "|",
                                                             std::string_view       cross_edge_character    = "+",
                                                             std::string_view       sep_fill                = "-"
                                                            );
    void                        print_table                 (const std::vector<std::vector<std::string_view>>&
                                                                                    table,
                                                             std::string_view       header_content          = {},
                                                             const uint16_t         max_col_width           = 30,
                                                             std::string_view       edge_character          = "|",
                                                             std::string_view       cross_edge_character    = "+",
                                                             std::string_view       sep_fill                = "-"
                                                            );

    std::string                 render_table                (const std::vector<std::vector<std::string_view>>&
                                                                                    table,
                                                             std::string_view       header_content          = {},
                                                             const uint16_t         max_col_width           = 30,
                                                             std::string_view       edge_character          = "|",
                                                             std::string_view       cross_edge_character    = "+",
                                                             std::string_view       sep_fill                = "-"
                                                            ) const;
    // getters
    std::string                 get_string                  (std::string_view       prompt                  = {},
                                                             const uint16_t         min_length              = 0,
                                                             const uint16_t         max_length              = 0,
                                                             const uint16_t         retry_count             = 0,
                                                             const uint32_t         timeout_ms              = 0,
                                                             std::string_view       default_value           = {},
                                                             std::optional<std::reference_wrapper<bool>>
                                                                                    success_sink            = std::nullopt
                                                            );
    int                         get_int                     (std::string_view       prompt                  = {},
                                                             const int              min_value               = std::numeric_limits<int>::min(),
                                                             const int              max_value               = std::numeric_limits<int>::max(),
                                                             const uint16_t         retry_count             = 0,
                                                             const uint32_t         timeout_ms              = 0,
                                                             const int              default_value           = 0,
                                                             std::optional<std::reference_wrapper<bool>>
                                                                                    success_sink            = std::nullopt
                                                            );
    uint8_t                     get_uint8                   (std::string_view       prompt                  = {},
                                                             const uint8_t          min_value               = std::numeric_limits<uint8_t>::min(),
                                                             const uint8_t          max_value               = std::numeric_limits<uint8_t>::max(),
                                                             const uint16_t         retry_count             = 0,
                                                             const uint32_t         timeout_ms              = 0,
                                                             const uint8_t          default_value           = 0,
                                                             std::optional<std::reference_wrapper<bool>>
                                                                                    success_sink            = std::nullopt
                                                            );
    uint16_t                    get_uint16                  (std::string_view       prompt                  = {},
                                                             const uint16_t         min_value               = std::numeric_limits<uint16_t>::min(),
                                                             const uint16_t         max_value               = std::numeric_limits<uint16_t>::max(),
                                                             const uint16_t         retry_count             = 0,
                                                             const uint32_t         timeout_ms              = 0,
                                                             const uint16_t         default_value           = 0,
                                                             std::optional<std::reference_wrapper<bool>>
                                                                                    success_sink            = std::nullopt
                                                            );
    uint32_t                    get_uint32                  (std::string_view       prompt                  = {},
                                                             const uint32_t         min_value               = std::numeric_limits<uint32_t>::min(),
                                                             const uint32_t         max_value               = std::numeric_limits<uint32_t>::max(),
                                                             const uint16_t         retry_count             = 0,
                                                             const uint32_t         timeout_ms              = 0,
                                                             const uint32_t         default_value           = 0,
                                                             std::optional<std::reference_wrapper<bool>>
                                                                                    success_sink            = std::nullopt
                                                            );
    float                       get_float                   (std::string_view       prompt                  = {},
                                                             const float            min_value               = -std::numeric_limits<float>::infinity(),
                                                             const float            max_value               = std::numeric_limits<float>::infinity(),
                                                             const uint16_t         retry_count             = 0,
                                                             const uint32_t         timeout_ms              = 0,
                                                             const float            default_value           = 0.0f,
                                                             std::optional<std::reference_wrapper<bool>>
                                                                                    success_sink            = std::nullopt
                                                            );
    bool                        get_yn                      (std::string_view       prompt                  = {},
                                                             const uint16_t         retry_count             = 0,
                                                             const uint32_t         timeout_ms              = 0,
                                                             const bool             default_value           = false,
                                                             std::optional<std::reference_wrapper<bool>>
                                                                                    success_sink            = std::nullopt
                                                            );
    uint8_t                     get_menu_choice             (std::string_view       prompt                  = {},
                                                             const std::vector<std::string> options         = {},
                                                             const uint8_t          min_value               = std::numeric_limits<uint8_t>::min(),
                                                             const uint8_t          max_value               = std::numeric_limits<uint8_t>::max(),
                                                             const uint16_t         retry_count             = 0,
                                                             const uint32_t         timeout_ms              = 0,
                                                             const uint8_t          default_value           = 0,
                                                             std::optional<std::reference_wrapper<bool>>
                                                                                    success_sink            = std::nullopt
                                                            );
    bool                        has_line                    ()                                              const;
    std::string                 read_line                   ();

private:
    void                        flush_input                 ();
    void                        print_raw                   (std::string_view message);
    void                        println_raw                 (std::string_view message);
    void                        printf_raw                  (const char* fmt,
                                                             ...
                                                            );
    bool                        read_line_with_timeout      (std::string& out,
                                                             const uint32_t timeout_ms
                                                            );
    void                        write_line_crlf             (std::string_view s);

    template <typename Ret, typename CheckFn>
    Ret                         get_core                    (std::string_view prompt,
                                                             uint16_t retry_count,
                                                             uint32_t timeout_ms,
                                                             Ret default_value,
                                                             std::optional<std::reference_wrapper<bool>> success_sink,
                                                             std::string_view iter_prompt,
                                                             bool iter_prompt_crlf,
                                                             CheckFn&& checker
                                                            );
    template <typename T>
    T                           get_integral                (std::string_view prompt,
                                                             const T min_value,
                                                             const T max_value,
                                                             const uint16_t retry_count,
                                                             const uint32_t timeout_ms,
                                                             const T default_value,
                                                             std::optional<std::reference_wrapper<bool>> success_sink
                                                            );

    std::size_t                 input_buffer_pos            = 0;
    std::size_t                 line_length                 = 0;
    bool                        line_ready                  = false;
    static constexpr std::size_t INPUT_BUFFER_SIZE          = 255;
    char                        input_buffer                [INPUT_BUFFER_SIZE] = {};
};


template <typename Ret, typename CheckFn>
inline Ret SerialPort::get_core (std::string_view prompt,
                                 uint16_t retry_count,
                                 uint32_t timeout_ms,
                                 Ret default_value,
                                 std::optional<std::reference_wrapper<bool>> success_sink,
                                 std::string_view iter_prompt,
                                 bool iter_prompt_crlf,
                                 CheckFn&& checker)
{
    auto set_success = [&](bool ok){
        if (success_sink.has_value()) success_sink->get() = ok;
    };

    flush_input();

    if (!prompt.empty()) this->println_raw(prompt);

    const bool infinite = (retry_count == 0);
    uint16_t attempts_left = retry_count;

    for (;;) {
        if (!iter_prompt.empty()) {
            if (iter_prompt_crlf) this->println_raw(iter_prompt);
            else                  this->print_raw(iter_prompt);
        }

        std::string line;
        bool got = this->read_line_with_timeout(line, timeout_ms);
        if (!got) {
            this->println_raw("! Timeout.");
            if (!infinite) {
                if (attempts_left == 0) { set_success(false); return default_value; }
                --attempts_left;
                if (attempts_left == 0) { set_success(false); return default_value; }
            }
            continue;
        }

        const char* err = nullptr;
        Ret value = default_value;
        bool ok = checker(line, value, err);
        if (!ok) {
            if (err) this->println_raw(err);
            if (!infinite) {
                if (attempts_left == 0) { set_success(false); return default_value; }
                --attempts_left;
                if (attempts_left == 0) { set_success(false); return default_value; }
            }
            continue;
        }

        set_success(true);
        return value;
    }
}
