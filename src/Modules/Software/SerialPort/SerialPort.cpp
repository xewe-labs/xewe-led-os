// src/Modules/SerialPort/SerialPort.cpp

#include "SerialPort.h"
#include "../../../Debug.h"


SerialPort::SerialPort(SystemController& controller)
      : Module(controller,
               /* module_name         */ "serial_port",
               /* module_description  */ "Allows to send and receive text messages over the USB wire",
               /* nvs_key             */ "ser",
               /* requires_init_setup */ false,
               /* can_be_disabled     */ false,
               /* has_cli_cmds        */ false)
{}

void SerialPort::begin_routines_required (const ModuleConfig& cfg) {
    const auto& config = static_cast<const SerialPortConfig&>(cfg);
    Serial.setTxBufferSize(1024);
    Serial.setRxBufferSize(1024);
    Serial.begin(config.baud_rate);
    delay(1000);
}

void SerialPort::loop () {
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

void SerialPort::reset (const bool verbose=false) {
    Module::reset(verbose);
    flush_input();
    input_buffer_pos = 0;
    line_length      = 0;
    line_ready       = false;
}

void SerialPort::print(std::string_view msg) {
    Serial.write(reinterpret_cast<const uint8_t*>(msg.data()), msg.size());
}

void SerialPort::println(std::string_view msg) {
    Serial.write(reinterpret_cast<const uint8_t*>(msg.data()), msg.size());
    Serial.write(reinterpret_cast<const uint8_t*>("\r\n"), 2); // CRLF is safest on serial terms
}

void SerialPort::printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    // First, compute required length
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);
    if (needed <= 0) { va_end(args); return; }

    // Allocate exact size (+1 for terminator used by vsnprintf)
    std::string buf(static_cast<size_t>(needed), '\0');
    vsnprintf(buf.data(), buf.size() + 1, fmt, args);
    va_end(args);

    Serial.write(reinterpret_cast<const uint8_t*>(buf.data()), buf.size());
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
    print("(y/n)?: ");
    while (millis() - start < timeout) {
        loop();
        if (has_line()) {
            std::string sv = read_line();
            if (!sv.empty()) {
                char c = static_cast<char>(std::tolower(sv[0]));
                if (c == 'y') return true;
                if (c == 'n') return false;
                print("(y/n)?: ");
            }
        }
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

void SerialPort::print_spacer (uint16_t total_width,
                                char major_character,
                                const std::string& edge_characters) {

    print(xewe::str::generate_split_line(total_width, major_character, edge_characters);
}

void SerialPort::print_centered (std::string_view message,
                            uint16_t total_width,
                            const std::string& edge_characters) {
    print(xewe::str::center_text(message, total_width, edge_characters);
}