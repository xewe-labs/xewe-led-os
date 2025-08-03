// src/Modules/Software/SerialPort/SerialPort.cpp
#include "SerialPort.h"
#include "../../../SystemController.h"
#include <charconv>
#include <cstring>
#include <algorithm>

SerialPort::SerialPort(SystemController& controller)
  : Module(controller,
           /* module_name */ "serial_port",
           /* nvs_key     */ "serial_port",
           /* can_be_disabled */ false),
    input_buffer_pos(0),
    line_length(0),
    line_ready(false),
    line_callback(nullptr)
{}

void SerialPort::begin(const ModuleConfig& /*cfg*/) {
    Serial.begin(baud_rate);
    delay(2000);
}

void SerialPort::loop() {
    while (Serial.available()) {
        char c = Serial.read();
        yield();
        // Echo back
        Serial.write(c);

        if (c == '\r') {
            continue;  // skip carriage return
        }

        if (c == '\n' || input_buffer_pos >= INPUT_BUFFER_SIZE - 1) {
            // terminate buffer and mark ready
            input_buffer[input_buffer_pos] = '\0';
            line_length = input_buffer_pos;
            input_buffer_pos = 0;
            line_ready = true;

            // invoke callback
            if (line_callback) {
                line_callback(std::string_view(input_buffer, line_length));
            }
        } else {
            // accumulate
            input_buffer[input_buffer_pos++] = c;
        }
    }
}

void SerialPort::enable()   {}
void SerialPort::disable()  {}

void SerialPort::reset() {
    flush_input();
    input_buffer_pos = 0;
    line_length       = 0;
    line_ready        = false;
}

std::string_view SerialPort::status() const {
    return "ready";
}

void SerialPort::flush_input() {
    while (Serial.available()) {
        Serial.read();
        yield();
    }
}

void SerialPort::print(const char* message) {
    Serial.print(message);
}

void SerialPort::println(const char* message) {
    Serial.println(message);
}

bool SerialPort::has_line() const {
    return line_ready;
}

std::string_view SerialPort::read_line() {
    if (!line_ready) {
        return {};
    }
    std::string_view sv(input_buffer, line_length);
    line_ready = false;
    line_length = 0;
    input_buffer_pos = 0;
    return sv;
}

std::string_view SerialPort::get_string(const char* prompt) {
    if (prompt && *prompt) {
        print(prompt);
    }
    while (!has_line()) {
        yield();
    }
    return read_line();
}

int SerialPort::get_int(const char* prompt) {
    auto sv = get_string(prompt);
    while (sv.empty()) {
        sv = get_string();
    }
    int value = 0;
    std::from_chars(sv.data(), sv.data() + sv.size(), value);
    return value;
}

bool SerialPort::get_confirmation(const char* prompt) {
    println(prompt);
    print("(y/n): ");
    auto sv = get_string();
    std::string tmp(sv);
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
    return (tmp == "y" || tmp == "yes" || tmp == "1" || tmp == "true");
}

bool SerialPort::prompt_user_yn(const char* prompt, uint16_t timeout) {
    println(prompt);
    uint32_t start_time = millis();
    while (millis() - start_time < timeout) {
        print("(y/n)?: ");
        if (has_line()) {
            auto sv = read_line();
            if (!sv.empty()) {
                char c = static_cast<char>(std::tolower(sv[0]));
                if (c == 'y') return true;
                if (c == 'n') return false;
            }
        }
        yield();
    }
    println("Timeout!");
    return false;
}

void SerialPort::print_spacer() {
    println("");
}

void SerialPort::set_line_callback(line_callback_t callback) {
    line_callback = std::move(callback);
}
