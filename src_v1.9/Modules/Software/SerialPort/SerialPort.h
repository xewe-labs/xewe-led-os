// src/Modules/Software/SerialPort/SerialPort.h
#pragma once

#include "../../Module/Module.h"
#include "../../../Debug.h"
#include <functional>
#include <string>
#include <string_view>
#include <cstdlib>
#include <cstring>

struct SerialPortConfig : public ModuleConfig {
    unsigned long               baud_rate                   = 9600;
};


class SerialPort : public Module {
public:
    explicit                    SerialPort                  (SystemController& controller);

    // required implementation
    bool                        begin                       (const ModuleConfig& cfg)       override;
    void                        loop                        ()                              override;
    void                        reset                       (bool verbose=false)            override;

    // other methods
    void                        print                       (std::string_view message);
    void                        println                     (std::string_view message);
    void                        printf                      (const char* format, ...);

    bool                        has_line                    () const;
    std::string                 read_line                   ();

    std::string                 get_string                  (std::string_view prompt = {});
    int                         get_int                     (std::string_view prompt = {});
    bool                        get_confirmation            (std::string_view prompt = {});
    bool                        prompt_user_yn              (std::string_view prompt = {}, uint16_t timeout = 30000);
    void                        print_spacer                ();

private:
    size_t                     input_buffer_pos             = 0;
    size_t                     line_length                  = 0;
    bool                       line_ready                   = false;
    static constexpr size_t    INPUT_BUFFER_SIZE            = 256;
    char                       input_buffer                 [INPUT_BUFFER_SIZE];

    void                       flush_input                  ();
};
