// src/Modules/ModuleName/ModuleName.h
#pragma once

#include "../../Module/Module.h"
#include "../../../Debug.h"

#include "../../../StringUtils.h"
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

    void                        begin_routines_required     (const ModuleConfig& cfg)       override;
    void                        loop                        ()                              override;
    void                        reset                       (const bool verbose=false)      override;

    // other methods
    void                        print                       (std::string_view message);
    void                        println                     (std::string_view message);
    void                        printf                      (const char* format, ...);

    void                        print_spacer                (uint16_t total_width=50,
                                                             char major_character = '-',
                                                             const std::string& edge_characters = "|");
    void                        print_centered              (std::string_view message,
                                                             uint16_t total_width=50,
                                                             const std::string& edge_characters = "|");


    bool                        has_line                    () const;
    std::string                 read_line                   ();

    std::string                 get_string                  (std::string_view prompt = {});
    int                         get_int                     (std::string_view prompt = {});
    bool                        get_confirmation            (std::string_view prompt = {});
    bool                        prompt_user_yn              (std::string_view prompt = {}, uint16_t timeout = 10000);


private:
    size_t                     input_buffer_pos             = 0;
    size_t                     line_length                  = 0;
    bool                       line_ready                   = false;
    static constexpr size_t    INPUT_BUFFER_SIZE            = 256;
    char                       input_buffer                 [INPUT_BUFFER_SIZE];

    void                       flush_input                  ();
};
