/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Modules/SerialPort/SerialPort.cpp
#include "SerialPort.h"
#include "../../../SystemController/SystemController.h"
#include <cstring>  // strlen

SerialPort::SerialPort(SystemController& controller)
    : Module(controller,
             /* module_name         */ "Serial_Port",
             /* module_description  */ "Allows to send and receive text messages over the USB wire",
             /* nvs_key             */ "ser",
             /* requires_init_setup */ false,
             /* can_be_disabled     */ false,
             /* has_cli_cmds        */ false) {
}

void SerialPort::begin_routines_required(const ModuleConfig& cfg) {
    const auto& config = static_cast<const SerialPortConfig&>(cfg);
    Serial.setTxBufferSize(2048);
    Serial.setRxBufferSize(1024);
    Serial.begin(config.baud_rate);
    delay(1000);
}

void SerialPort::loop() {
    while (Serial.available()) {
        char c = static_cast<char>(Serial.read());
        yield();

        Serial.write(static_cast<uint8_t>(c));

        if (c == '\r') continue;
        if (c == '\n' || input_buffer_pos >= INPUT_BUFFER_SIZE - 1) {
            input_buffer[input_buffer_pos] = '\0';
            line_length = input_buffer_pos;
            input_buffer_pos = 0;
            line_ready = true;
        } else {
            input_buffer[input_buffer_pos++] = c;
        }
    }
}

void SerialPort::reset (const bool verbose, const bool do_restart, const bool keep_enabled) {
    flush_input();
    input_buffer_pos = 0;
    line_length      = 0;
    line_ready       = false;
    Module::reset(verbose, do_restart, keep_enabled);
}

// printers
void SerialPort::print(std::string_view message,
                       std::string_view end,
                       std::string_view edge_character,
                       const char text_align,
                       const char wrap_mode,
                       const uint16_t message_width,
                       const uint16_t margin_l,
                       const uint16_t margin_r) {
    auto lines_sv = split_lines_sv(message, '\n');
    const bool use_wrap = (message_width > 0);

    for (size_t i = 0; i < lines_sv.size(); ++i) {
        std::string base_line(lines_sv[i]);
        rtrim_cr(base_line);

        std::vector<std::string> chunks = use_wrap
            ? ((wrap_mode == 'c' || wrap_mode == 'C')
                  ? wrap_fixed(base_line, message_width)
                  : wrap_words(base_line, message_width))
            : std::vector<std::string>{base_line};

        for (size_t j = 0; j < chunks.size(); ++j) {
            const bool is_last = (i + 1 == lines_sv.size()) && (j + 1 == chunks.size());
            std::string out = compose_box_line(chunks[j], edge_character,
                                               message_width, margin_l, margin_r, text_align);
            Serial.write(reinterpret_cast<const uint8_t*>(out.data()), out.size());
            if (is_last) {
                if (!end.empty())
                    Serial.write(reinterpret_cast<const uint8_t*>(end.data()), end.size());
            } else {
                Serial.write(reinterpret_cast<const uint8_t*>(kCRLF), 2);
            }
        }
    }
}

void SerialPort::printf_fmt(std::string_view end,
                        std::string_view edge_character,
                        const char text_align,
                        const char wrap_mode,
                        const uint16_t message_width,
                        const uint16_t margin_l,
                        const uint16_t margin_r,
                        const char* fmt, ...) {
    if (!fmt) {
        print("", end, edge_character, text_align, wrap_mode, message_width, margin_l, margin_r);
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    va_list ap2;
    va_copy(ap2, ap);
    const int needed = vsnprintf(nullptr, 0, fmt, ap);
    va_end(ap);

    std::string msg;
    if (needed > 0) {
        std::vector<char> buf(static_cast<size_t>(needed) + 1u);
        vsnprintf(buf.data(), buf.size(), fmt, ap2);
        msg.assign(buf.data(), static_cast<size_t>(needed));
    }
    va_end(ap2);

    print(msg, end, edge_character, text_align, wrap_mode, message_width, margin_l, margin_r);
}

void SerialPort::printf(const char* fmt, ...) {
    if (!fmt) { print(); return; }

    va_list ap;
    va_start(ap, fmt);
    va_list ap2;
    va_copy(ap2, ap);
    const int needed = vsnprintf(nullptr, 0, fmt, ap);
    va_end(ap);

    std::string msg;
    if (needed > 0) {
        std::vector<char> buf(static_cast<size_t>(needed) + 1u);
        vsnprintf(buf.data(), buf.size(), fmt, ap2);
        msg.assign(buf.data(), static_cast<size_t>(needed));
    }
    va_end(ap2);

    print(msg);
}


void SerialPort::print_separator(const uint16_t total_width,
                                 std::string_view fill,
                                 std::string_view edge_character) {
    std::string line;
    if (total_width == 0) {
        line.clear();
    } else if (edge_character.empty()) {
        // Full-width fill pattern
        line = repeat_pattern(fill, total_width);
    } else {
        const size_t e = edge_character.size();
        if (total_width <= e) {
            line.assign(edge_character.substr(0, total_width));
        } else if (total_width <= 2 * e) {
            // Not enough room for interior
            line.assign(edge_character.substr(0, total_width));
        } else {
            const uint16_t inner = static_cast<uint16_t>(total_width - 2 * e);
            line.reserve(total_width);
            line.append(edge_character);
            line += repeat_pattern(fill, inner);
            line.append(edge_character);
        }
    }
    write_line_crlf(line);
}

void SerialPort::print_spacer(const uint16_t total_width,
                              std::string_view edge_character) {
    std::string line;
    if (total_width == 0) {
        line.clear();
    } else if (edge_character.empty()) {
        line.assign(static_cast<size_t>(total_width), ' ');
    } else {
        const size_t e = edge_character.size();
        if (total_width <= e) {
            line.assign(edge_character.substr(0, total_width));
        } else if (total_width <= 2 * e) {
            line.assign(edge_character.substr(0, total_width));
        } else {
            const uint16_t inner = static_cast<uint16_t>(total_width - 2 * e);
            line.reserve(total_width);
            line.append(edge_character);
            line.append(inner, ' ');
            line.append(edge_character);
        }
    }
    write_line_crlf(line);
}

void SerialPort::print_header(std::string_view message,
                              const uint16_t total_width,
                              std::string_view edge_character,
                              std::string_view cross_edge_character,
                              std::string_view sep_fill) {
    print_separator(total_width, sep_fill, cross_edge_character);

    auto parts = split_by_token(message, "\\sep");
    const uint16_t edge_w = static_cast<uint16_t>(edge_character.size() * 2) + 2;
    const uint16_t content_width =
        (!edge_character.empty() && total_width > edge_w)
            ? static_cast<uint16_t>(total_width - edge_w)
            : total_width;

    for (auto& p : parts) {
        print(p, kCRLF, edge_character, 'c', 'w', content_width, 1, 1);
        print_separator(total_width, sep_fill, cross_edge_character);
    }
}

void SerialPort::print_table(const vector<vector<string_view>>& table,
                             string_view header_content,
                             const uint16_t max_col_width,
                             string_view edge_character,
                             string_view cross_edge_character,
                             string_view sep_fill) {
    if (table.empty()) return;

    // 1. Calculate Column Widths
    // We must ensure the column is wide enough for the longest *line* in a multi-line cell,
    // not just the total length of the string.
    size_t num_cols = 0;
    for (const auto& row : table) num_cols = max(num_cols, row.size());

    vector<uint16_t> col_widths(num_cols, 0);

    for (const auto& row : table) {
        for (size_t c = 0; c < row.size(); ++c) {
            string_view cell = row[c];
            size_t max_line_len = 0;

            // Find longest segment between '\n'
            size_t start = 0;
            while (start <= cell.length()) {
                size_t end = cell.find('\n', start);
                if (end == string_view::npos) end = cell.length();

                size_t segment_len = end - start;
                if (segment_len > max_line_len) max_line_len = segment_len;

                if (end == cell.length()) break;
                start = end + 1;
            }

            // Width = Longest Line + 1 space left + 1 space right
            size_t req_width = max_line_len + 2;
            if (req_width > max_col_width) req_width = max_col_width;

            if (req_width > col_widths[c]) {
                col_widths[c] = static_cast<uint16_t>(req_width);
            }
        }
    }

    // 2. Calculate Total Table Width
    size_t total_table_width = edge_character.size();
    for (auto w : col_widths) {
        total_table_width += w + edge_character.size();
    }

    // Helper: Print a complex divider (e.g., +-----+-----+)
    auto print_complex_divider = [&]() {
        string line;
        line.reserve(total_table_width);
        line.append(cross_edge_character);
        for (size_t c = 0; c < num_cols; ++c) {
            for (size_t k = 0; k < col_widths[c]; ++k) {
                if (!sep_fill.empty())
                    line += sep_fill[k % sep_fill.size()];
                else
                    line += '-';
            }
            line.append(cross_edge_character);
        }
        write_line_crlf(line);
    };

    // Helper: Wrap text respecting explicit '\n'
    auto get_wrapped_lines = [&](string_view text, uint16_t width) -> vector<string> {
        vector<string> result;
        if (width <= 2) width = 3; // minimal safety
        uint16_t content_width = width - 2;

        size_t start = 0;
        // If empty, return one empty line so height calc works
        if (text.empty()) return { "" };

        while (start <= text.length()) {
            size_t end = text.find('\n', start);
            if (end == string_view::npos) end = text.length();

            // 1. Extract the explicit line segment
            string_view segment = text.substr(start, end - start);

            // 2. Wrap this segment specifically
            if (segment.empty()) {
                // Explicit empty line (e.g. \n\n)
                result.push_back("");
            } else {
                // Use existing wrapper for this segment
                vector<string> seg_lines = wrap_words(string(segment), content_width);
                if (seg_lines.empty()) result.push_back("");
                else result.insert(result.end(), seg_lines.begin(), seg_lines.end());
            }

            if (end == text.length()) break;
            start = end + 1;
        }
        return result;
    };

    // 3. Print Header (if exists)
    if (!header_content.empty()) {
        print_separator(static_cast<uint16_t>(total_table_width), sep_fill, cross_edge_character);
        uint16_t header_content_width = static_cast<uint16_t>(total_table_width - (edge_character.size() * 2));
        print(header_content, kCRLF, edge_character, 'c', 'w', header_content_width, 0, 0);
    }

    // 4. Print Table Body
    print_complex_divider();

    for (const auto& row : table) {
        // Pre-calculate wrapped blocks
        vector<vector<string>> row_blocks;
        size_t max_row_height = 0;

        for (size_t c = 0; c < num_cols; ++c) {
            string_view entry = (c < row.size()) ? row[c] : "";
            vector<string> wrapped = get_wrapped_lines(entry, col_widths[c]);

            // Ensure at least one line exists
            if (wrapped.empty()) wrapped.push_back("");

            max_row_height = max(max_row_height, wrapped.size());
            row_blocks.push_back(move(wrapped));
        }

        // Print physical lines for this row
        for (size_t h = 0; h < max_row_height; ++h) {
            string line_out;
            line_out.reserve(total_table_width);
            line_out.append(edge_character);

            for (size_t c = 0; c < num_cols; ++c) {
                string segment = (h < row_blocks[c].size()) ? row_blocks[c][h] : "";

                // Left Margin
                line_out += ' ';
                line_out += segment;

                // Right Padding
                size_t current_len = segment.length();
                size_t target_len  = static_cast<size_t>(col_widths[c]) - 2;

                if (target_len > current_len) {
                    line_out.append(target_len - current_len, ' ');
                }

                line_out += ' '; // Right Margin
                line_out += edge_character;
            }
            write_line_crlf(line_out);
        }

        // Separator between rows
        print_complex_divider();
    }
}

// getters
string SerialPort::get_string(string_view prompt,
                                   const uint16_t min_length,
                                   const uint16_t max_length,
                                   const uint16_t retry_count,
                                   const uint32_t timeout_ms,
                                   string_view default_value,
                                   optional<reference_wrapper<bool>> success_sink) {
    const size_t min_len = static_cast<size_t>(min_length);
    const size_t max_len = (max_length == 0) ? (INPUT_BUFFER_SIZE - 1)
                                             : static_cast<size_t>(max_length);

    auto checker = [&](const string& line, string& out, const char*& err)->bool {
        if (line.size() < min_len || line.size() > max_len) {
            printf_raw("! Length must be in [%u..%u] chars.\r\n",
                       static_cast<unsigned>(min_len),
                       static_cast<unsigned>(max_len));
            err = nullptr;
            return false;
        }
        out = line;
        return true;
    };

    return get_core<string>(prompt, retry_count, timeout_ms, string(default_value),
                               success_sink, "> ", /*crlf*/false, checker);
}

int SerialPort::get_int(string_view prompt,
                        const int min_value,
                        const int max_value,
                        const uint16_t retry_count,
                        const uint32_t timeout_ms,
                        const int default_value,
                        optional<reference_wrapper<bool>> success_sink) {
    return get_integral<int>(prompt, min_value, max_value, retry_count, timeout_ms, default_value, success_sink);
}

uint8_t SerialPort::get_uint8(string_view prompt,
                              const uint8_t min_value,
                              const uint8_t max_value,
                              const uint16_t retry_count,
                              const uint32_t timeout_ms,
                              const uint8_t default_value,
                              optional<reference_wrapper<bool>> success_sink) {
    return get_integral<uint8_t>(prompt, min_value, max_value, retry_count, timeout_ms, default_value, success_sink);
}

uint16_t SerialPort::get_uint16(string_view prompt,
                                const uint16_t min_value,
                                const uint16_t max_value,
                                const uint16_t retry_count,
                                const uint32_t timeout_ms,
                                const uint16_t default_value,
                                optional<reference_wrapper<bool>> success_sink) {
    return get_integral<uint16_t>(prompt, min_value, max_value, retry_count, timeout_ms, default_value, success_sink);
}

uint32_t SerialPort::get_uint32(string_view prompt,
                                const uint32_t min_value,
                                const uint32_t max_value,
                                const uint16_t retry_count,
                                const uint32_t timeout_ms,
                                const uint32_t default_value,
                                optional<reference_wrapper<bool>> success_sink) {
    return get_integral<uint32_t>(prompt, min_value, max_value, retry_count, timeout_ms, default_value, success_sink);
}

float SerialPort::get_float(string_view prompt,
                            const float min_value,
                            const float max_value,
                            const uint16_t retry_count,
                            const uint32_t timeout_ms,
                            const float default_value,
                            optional<reference_wrapper<bool>> success_sink) {
    float minv = min_value, maxv = max_value;
    if (minv > maxv) std::swap(minv, maxv);

    auto checker = [&](const string& line, float& out, const char*& err)->bool {
        const char* s = line.c_str();
        char* end = nullptr;
        double dv = strtod(s, &end);
        while (end && *end == ' ') ++end;
        if (s == end || (end && *end != '\0')) {
            err = "! Invalid number. Please enter a decimal value.";
            return false;
        }
        if (dv != dv) { err = "! Invalid number."; return false; } // NaN
        float v = static_cast<float>(dv);
        if (v < minv || v > maxv) {
            printf_raw("! Out of range [%g..%g].\r\n",
                       static_cast<double>(minv),
                       static_cast<double>(maxv));
            err = nullptr;
            return false;
        }
        out = v;
        return true;
    };

    return get_core<float>(prompt, retry_count, timeout_ms, default_value,
                              success_sink, "> ", /*crlf*/false, checker);
}

bool SerialPort::get_yn(string_view prompt,
                        const uint16_t retry_count,
                        const uint32_t timeout_ms,
                        const bool default_value,
                        optional<reference_wrapper<bool>> success_sink) {
    auto checker = [&](const string& line, bool& out, const char*& err)->bool {
        string low = to_lower(line);
        if (low == "y" || low == "yes" || low == "1" || low == "true")  { out = true;  return true; }
        if (low == "n" || low == "no"  || low == "0" || low == "false") { out = false; return true; }
        err = "! Please answer 'y' or 'n'.";
        return false;
    };

    return get_core<bool>(prompt, retry_count, timeout_ms, default_value,
                             success_sink, "(y/n) > ", /*crlf*/false, checker);
}

uint8_t SerialPort::get_menu_choice(string_view prompt,
                                    const vector<string> options,
                                    const uint8_t min_value,
                                    const uint8_t max_value,
                                    const uint16_t retry_count,
                                    const uint32_t timeout_ms,
                                    const uint8_t default_value,
                                    optional<reference_wrapper<bool>> success_sink) {
    // 1. Display the prompt if provided
    if (!prompt.empty()) {
        println_raw(prompt);
    }

    // 2. Determine effective boundaries
    uint8_t actual_min = min_value;
    uint8_t actual_max = max_value;

    // Auto-adjust bounds if defaults were left untouched but options were provided
    if (!options.empty()) {
        if (actual_min == numeric_limits<uint8_t>::min()) {
            actual_min = 1; // Default to 1-based indexing for menus
        }
        if (actual_max == numeric_limits<uint8_t>::max()) {
            // Prevent overflow if actual_min + options.size() exceeds uint8_t max
            uint16_t calc_max = static_cast<uint16_t>(actual_min) + static_cast<uint16_t>(options.size()) - 1;
            actual_max = (calc_max > 255) ? 255 : static_cast<uint8_t>(calc_max);
        }
    }

    // 3. Display the menu options (if any)
    for (size_t i = 0; i < options.size(); ++i) {
        printf_raw("  %u) %s\r\n", static_cast<unsigned>(actual_min + i), options[i].c_str());
    }

    // 4. Delegate to get_uint8
    // If we only have a prompt and no options, don't double-prompt "Choice >".
    // Just use empty string so the user sees " > " right under their custom prompt.
    string_view input_prompt = (options.empty() && !prompt.empty()) ? "" : "Choice";

    return get_uint8(input_prompt, actual_min, actual_max, retry_count, timeout_ms, default_value, success_sink);
}

bool SerialPort::has_line() const { return line_ready; }

string SerialPort::read_line() {
    if (!line_ready) return {};
    string out(input_buffer, line_length);
    line_ready       = false;
    line_length      = 0;
    input_buffer_pos = 0;
    return out;
}

// private
void SerialPort::flush_input() {
    while (Serial.available()) {
        (void)Serial.read();
        yield();
    }
    input_buffer_pos = 0;
    line_length      = 0;
    line_ready       = false;
}

void SerialPort::print_raw(string_view message) {
    Serial.write(reinterpret_cast<const uint8_t*>(message.data()), message.size());
}

void SerialPort::println_raw(string_view message) {
    Serial.write(reinterpret_cast<const uint8_t*>(message.data()), message.size());
    Serial.write(reinterpret_cast<const uint8_t*>(kCRLF), 2);
}

void SerialPort::printf_raw(const char* fmt, ...) {
    if (!fmt) return;

    bool has_spec = false;
    for (const char* p = fmt; *p; ++p) {
        if (*p == '%') {
            if (*(p + 1) == '%') { ++p; continue; }
            has_spec = true;
            break;
        }
    }
    if (!has_spec) {
        size_t n = strlen(fmt);
        if (n) Serial.write(reinterpret_cast<const uint8_t*>(fmt), n);
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    va_list ap2;
    va_copy(ap2, ap);
    int needed = vsnprintf(nullptr, 0, fmt, ap);
    va_end(ap);

    if (needed <= 0) { va_end(ap2); return; }

    std::vector<char> buf(static_cast<size_t>(needed) + 1u);
    vsnprintf(buf.data(), buf.size(), fmt, ap2);
    va_end(ap2);

    Serial.write(reinterpret_cast<const uint8_t*>(buf.data()), static_cast<size_t>(needed));
}

bool SerialPort::read_line_with_timeout(string& out,
                                        const uint32_t timeout_ms) {
    uint32_t start = millis();
    for (;;) {
        loop();
        if (has_line()) {
            out = read_line();
            return true;
        }
        if (timeout_ms != 0 && (millis() - start >= timeout_ms)) {
            return false;
        }
        yield();
    }
}

void SerialPort::write_line_crlf(string_view s) {
    Serial.write(reinterpret_cast<const uint8_t*>(s.data()), s.size());
    Serial.write(reinterpret_cast<const uint8_t*>(kCRLF), 2);
}

template <typename T>
T SerialPort::get_integral(string_view prompt,
                           const T min_value,
                           const T max_value,
                           const uint16_t retry_count,
                           const uint32_t timeout_ms,
                           const T default_value,
                           optional<reference_wrapper<bool>> success_sink) {
    T minv = min_value, maxv = max_value;
    if (minv > maxv) std::swap(minv, maxv);

    auto checker = [&](const string& line, T& out, const char*& err)->bool {
        T v{};
        if (!parse_int<T>(line, v)) {
            err = "! Invalid number. Please enter a base-10 integer.";
            return false;
        }
        if (v < minv || v > maxv) {
            printf_raw("! Out of range [%lld..%lld].\r\n",
                       static_cast<long long>(minv),
                       static_cast<long long>(maxv));
            err = nullptr;
            return false;
        }
        out = v;
        return true;
    };

    return get_core<T>(prompt, retry_count, timeout_ms, default_value, success_sink, "> ", false, checker);
}