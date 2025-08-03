// src/Modules/Software/SerialPort/SerialPort.h
#pragma once

#include <functional>
#include <string_view>
#include "../../Module.h"
#include "../../../Debug.h"

class SerialPort : public Module {
public:
    explicit SerialPort(SystemController& controller);

    // Module interface
    void begin(const ModuleConfig& cfg) override;
    void loop() override;
    void enable() override;
    void disable() override;
    void reset() override;
    std::string_view status() const override;

    // Extra API (C‐string based)
    void print(const char* message);
    void println(const char* message);
    bool has_line() const;
    std::string_view read_line();
    std::string_view get_string(const char* prompt = "");
    int get_int(const char* prompt = "");
    bool get_confirmation(const char* prompt = "");
    bool prompt_user_yn(const char* prompt = "", uint16_t timeout = 10000);
    void print_spacer();

    // Async callback API
    using line_callback_t = std::function<void(std::string_view)>;
    /// Register a callback that will be invoked whenever a complete line is received.
    void set_line_callback(line_callback_t callback);

private:
    unsigned long baud_rate = 115200;
    static constexpr size_t INPUT_BUFFER_SIZE = 256;
    char                input_buffer[INPUT_BUFFER_SIZE];
    size_t              input_buffer_pos = 0;
    size_t              line_length       = 0;
    bool                line_ready        = false;
    line_callback_t     line_callback;

    void flush_input();
};
