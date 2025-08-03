// src/Modules/Software/SerialPort/SerialPort.h
#pragma once

#include <functional>
#include <string_view>
#include "../../Module.h"
#include "../../../Debug.h"

/// Configuration for SerialPort: all user‐tuneable parameters go here.
struct SerialPortConfig : public ModuleConfig {
    unsigned long baud_rate = 115200;
};

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

    // Extra API (String‐based)
    void print(std::string_view message);
    void println(std::string_view message);
    bool has_line() const;
    std::string_view read_line();
    std::string_view get_string(std::string_view prompt = {});
    int get_int(std::string_view prompt = {});
    bool get_confirmation(std::string_view prompt = {});
    bool prompt_user_yn(std::string_view prompt = {}, uint16_t timeout = 10000);
    void print_spacer();

    // Async callback API
    using line_callback_t = std::function<void(std::string_view)>;
    void set_line_callback(line_callback_t callback);

private:
    size_t               input_buffer_pos = 0;
    size_t               line_length       = 0;
    bool                 line_ready        = false;
    line_callback_t      line_callback;

    unsigned long        baud_rate         = 115200;  // overwritten by cfg in begin()

    static constexpr size_t INPUT_BUFFER_SIZE = 256;
    char                input_buffer[INPUT_BUFFER_SIZE];

    void flush_input();
};
