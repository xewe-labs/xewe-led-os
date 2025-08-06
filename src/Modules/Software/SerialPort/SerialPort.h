// src/Modules/Software/SerialPort/SerialPort.h
#pragma once

#include <functional>
#include <string_view>
#include <cstdlib>    // for std::strtol
#include <cstring>

#include "../../Module.h"
#include "../../../Debug.h"

struct SerialPortConfig : public ModuleConfig {
    unsigned long baud_rate = 115200;
};

class SerialPort : public Module {
public:
    explicit                    SerialPort          (SystemController& controller);

    void                        begin               (const ModuleConfig& cfg)               override;
    void                        loop                ()                                      override;
//    void                        enable              ()                                      override;
//    void                        disable             ()                                      override;
    void                        reset               (bool verbose=true)                                      override;

//    std::string_view            status              (bool verbose=true)             const   override;
//    bool                        is_enabled          (bool verbose=true)             const   override;
//    bool                        is_disabled         (bool verbose=true)             const   override;

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

private:
    size_t               input_buffer_pos = 0;
    size_t               line_length      = 0;
    bool                 line_ready       = false;
    static constexpr size_t INPUT_BUFFER_SIZE = 256;
    char                input_buffer[INPUT_BUFFER_SIZE];

    void flush_input();
};
