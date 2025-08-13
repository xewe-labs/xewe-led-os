// src/Modules/Software/SerialPort/SerialPort.cpp
#include "SerialPort.h"
#include "../../../SystemController/SystemController.h"


SerialPort::SerialPort(SystemController& controller)
      : Module(controller,
               /* module_name */ "serial_port",
               /* nvs_key      */ "ser",
               /* can_be_disabled */ false,
               /* has_cli_cmds */ false)
    {}

void SerialPort::begin(const ModuleConfig& cfg_base) {
    const auto& cfg = static_cast<const SerialPortConfig&>(cfg_base);
    Serial.begin(cfg.baud_rate);
    delay(2000);
}

void SerialPort::loop() {
    while (Serial.available()) {
        char c = Serial.read();
        yield();
        Serial.write(c);

        if (c == '\r') {
            continue;
        }
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

void SerialPort::reset(bool verbose) {
    flush_input();
    input_buffer_pos = 0;
    line_length      = 0;
    line_ready       = false;
}

void SerialPort::print(std::string_view message) {
    std::string tmp(message);
    Serial.print(tmp.c_str());
}

void SerialPort::println(std::string_view message) {
    std::string tmp(message);
    Serial.println(tmp.c_str());
}

bool SerialPort::has_line() const {
    return line_ready;
}

std::string SerialPort::read_line() {
    if (!line_ready) {
        return std::string();
    }
    std::string sv(input_buffer, line_length);
    line_ready       = false;
    line_length      = 0;
    input_buffer_pos = 0;
    return sv;
}

std::string SerialPort::get_string(std::string_view prompt) {
    if (!prompt.empty()) print(prompt);
    while (!has_line()) loop();
    return read_line();
}

int SerialPort::get_int(std::string_view prompt) {
    std::string sv = get_string(prompt);
    while (sv.empty()) sv = get_string();
    char buf[32];
    size_t len = sv.copy(buf, sizeof(buf) - 1);
    buf[len] = '\0';
    return static_cast<int>(std::strtol(buf, nullptr, 10));
}

bool SerialPort::get_confirmation(std::string_view prompt) {
    println(prompt);
    print("(y/n): ");
    std::string sv = get_string();
    std::transform(sv.begin(), sv.end(), sv.begin(), ::tolower);
    return (sv == "y" || sv == "yes" || sv == "1" || sv == "true");
}

bool SerialPort::prompt_user_yn(std::string_view prompt, uint16_t timeout) {
    println(prompt);
    uint32_t start = millis();
    while (millis() - start < timeout) {
        print("(y/n)?: ");
        if (has_line()) {
            std::string sv = read_line();
            if (!sv.empty()) {
                char c = static_cast<char>(std::tolower(sv[0]));
                if (c == 'y') return true;
                if (c == 'n') return false;
            }
        }
        loop();
    }
    println("Timeout!");
    return false;
}

void SerialPort::print_spacer() {
    println("");
}

void SerialPort::flush_input() {
    while (Serial.available()) {
        Serial.read();
        loop();
    }
}
