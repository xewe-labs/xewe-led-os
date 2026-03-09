/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Modules/SerialPort/SerialPort.h
#pragma once

#include "../../Module/Module.h"

#include <optional>


struct SerialPortConfig : public ModuleConfig {
    unsigned long baud_rate = 115200;
};


class SerialPort : public Module {
public:
    explicit                    SerialPort                  (SystemController& controller);

    void                        begin_routines_required     (const ModuleConfig&    cfg)                    override;
    void                        loop                        ()                                              override;
    void                        reset                       (const bool verbose=false,
                                                             const bool do_restart=true,
                                                             const bool keep_enabled=true)                  override;
    // printers
    void                        print                       (string_view            message                 = {},
                                                             string_view            end                     = kCRLF,
                                                             string_view            edge_character          = {},
                                                             const char             text_align              = 'l',
                                                             const char             wrap_mode               = 'w',
                                                             const uint16_t         message_width           = 0,
                                                             const uint16_t         margin_l                = 0,
                                                             const uint16_t         margin_r                = 0
                                                            );
    void                        printf_fmt                  (string_view            edge_character,
                                                             string_view            end,
                                                             const char             text_align,
                                                             const char             wrap_mode,
                                                             const uint16_t         message_width,
                                                             const uint16_t         margin_l,
                                                             const uint16_t         margin_r,
                                                             const char*            fmt,
                                                             ...
                                                            );
    void                        printf                      (const char* fmt,
                                                             ...
                                                            );
    void                        print_separator             (const uint16_t         total_width             = 50,
                                                             string_view            fill                    = "-",
                                                             string_view            edge_character          = "+"
                                                            );
    void                        print_spacer                (const uint16_t         total_width             = 50,
                                                             string_view            edge_character          = {}
                                                            );
    void                        print_header                (string_view            message,
                                                             const uint16_t         total_width             = 50,
                                                             string_view            edge_character          = "|",
                                                             string_view            cross_edge_character    = "+",
                                                             string_view            sep_fill                = "-"
                                                            );
    void                        print_table                 (const vector<vector<string_view>>& table,
                                                             string_view            header_content          = {},
                                                             const uint16_t         max_col_width           = 30,
                                                             string_view            edge_character          = "|",
                                                             string_view            cross_edge_character    = "+",
                                                             string_view            sep_fill                = "-"
                                                            );

    // getters
    string                      get_string                  (string_view            prompt                  = {},
                                                             const uint16_t         min_length              = 0,
                                                             const uint16_t         max_length              = 0,
                                                             const uint16_t         retry_count             = 0,
                                                             const uint32_t         timeout_ms              = 0,
                                                             string_view            default_value           = {},
                                                             optional<reference_wrapper<bool>> success_sink = nullopt
                                                            );
    int                         get_int                     (string_view            prompt                  = {},
                                                             const int              min_value               = numeric_limits<int>::min(),
                                                             const int              max_value               = numeric_limits<int>::max(),
                                                             const uint16_t         retry_count             = 0,
                                                             const uint32_t         timeout_ms              = 0,
                                                             const int              default_value           = 0,
                                                             optional<reference_wrapper<bool>> success_sink = nullopt
                                                            );
    uint8_t                     get_uint8                   (string_view            prompt                  = {},
                                                             const uint8_t          min_value               = numeric_limits<uint8_t>::min(),
                                                             const uint8_t          max_value               = numeric_limits<uint8_t>::max(),
                                                             const uint16_t         retry_count             = 0,
                                                             const uint32_t         timeout_ms              = 0,
                                                             const uint8_t          default_value           = 0,
                                                             optional<reference_wrapper<bool>> success_sink = nullopt
                                                            );
    uint16_t                    get_uint16                  (string_view            prompt                  = {},
                                                             const uint16_t         min_value               = numeric_limits<uint16_t>::min(),
                                                             const uint16_t         max_value               = numeric_limits<uint16_t>::max(),
                                                             const uint16_t         retry_count             = 0,
                                                             const uint32_t         timeout_ms              = 0,
                                                             const uint16_t         default_value           = 0,
                                                             optional<reference_wrapper<bool>> success_sink = nullopt
                                                            );
    uint32_t                    get_uint32                  (string_view            prompt                  = {},
                                                             const uint32_t         min_value               = numeric_limits<uint32_t>::min(),
                                                             const uint32_t         max_value               = numeric_limits<uint32_t>::max(),
                                                             const uint16_t         retry_count             = 0,
                                                             const uint32_t         timeout_ms              = 0,
                                                             const uint32_t         default_value           = 0,
                                                             optional<reference_wrapper<bool>> success_sink = nullopt
                                                            );
    float                       get_float                   (string_view            prompt                  = {},
                                                             const float            min_value               = -numeric_limits<float>::infinity(),
                                                             const float            max_value               = numeric_limits<float>::infinity(),
                                                             const uint16_t         retry_count             = 0,
                                                             const uint32_t         timeout_ms              = 0,
                                                             const float            default_value           = 0.0f,
                                                             optional<reference_wrapper<bool>> success_sink = nullopt
                                                            );
    bool                        get_yn                      (string_view            prompt                  = {},
                                                             const uint16_t         retry_count             = 0,
                                                             const uint32_t         timeout_ms              = 0,
                                                             const bool             default_value           = false,
                                                             optional<reference_wrapper<bool>> success_sink = nullopt
                                                            );
    uint8_t                     get_menu_choice             (string_view            prompt                  = {},
                                                             const vector<string>   options                 = {},
                                                             const uint8_t          min_value               = numeric_limits<uint8_t>::min(),
                                                             const uint8_t          max_value               = numeric_limits<uint8_t>::max(),
                                                             const uint16_t         retry_count             = 0,
                                                             const uint32_t         timeout_ms              = 0,
                                                             const uint8_t          default_value           = 0,
                                                             optional<reference_wrapper<bool>> success_sink = nullopt
                                                            );
    bool                        has_line                    ()                                              const;
    string                      read_line                   ();

private:
    void                        flush_input                 ();
    void                        print_raw                   (string_view message);
    void                        println_raw                 (string_view message);
    void                        printf_raw                  (const char* fmt,
                                                             ...
                                                             );
    bool                        read_line_with_timeout      (string& out,
                                                             const uint32_t timeout_ms
                                                            );
    void                        write_line_crlf             (string_view s);

    template <typename Ret, typename CheckFn>
    Ret                         get_core                    (string_view prompt,
                                                             uint16_t retry_count,
                                                             uint32_t timeout_ms,
                                                             Ret default_value,
                                                             optional<reference_wrapper<bool>> success_sink,
                                                             string_view iter_prompt,
                                                             bool iter_prompt_crlf,
                                                             CheckFn&& checker
                                                            );
    template <typename T>
    T                           get_integral                (string_view prompt,
                                                             const T min_value,
                                                             const T max_value,
                                                             const uint16_t retry_count,
                                                             const uint32_t timeout_ms,
                                                             const T default_value,
                                                             optional<reference_wrapper<bool>> success_sink
                                                            );

    size_t                      input_buffer_pos            = 0;
    size_t                      line_length                 = 0;
    bool                        line_ready                  = false;
    static constexpr size_t     INPUT_BUFFER_SIZE           = 255;
    char                        input_buffer                [INPUT_BUFFER_SIZE];
};


template <typename Ret, typename CheckFn>
inline Ret SerialPort::get_core (string_view prompt,
                                 uint16_t retry_count,
                                 uint32_t timeout_ms,
                                 Ret default_value,
                                 optional<reference_wrapper<bool>> success_sink,
                                 string_view iter_prompt,
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

        string line;
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
