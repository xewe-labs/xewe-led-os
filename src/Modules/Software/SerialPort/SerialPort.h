#pragma once

#include "../../Module.h"
#include <Arduino.h>

class SerialPort : public Module {
public:
    explicit SerialPort(SystemController& controller);

    // Module interface
    void begin(const ModuleConfig& cfg) override;
    void loop(const std::string& args) override;
    void enable() override;
    void disable() override;
    void reset() override;
    std::string_view status() const override;

    // Extra API
    void print(const String& message);
    void println(const String& message);
    bool has_line() const;
    String read_line();
    String get_string(const String message = "");
    int get_int(const String message = "");
    bool get_confirmation(const String message = "");
    bool prompt_user_yn(const String message, uint16_t timeout = 10000);
    void print_spacer();

private:
    unsigned long baud_rate_ = 115200;
    void flush_input();
};
