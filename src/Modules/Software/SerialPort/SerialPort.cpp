// src/Modules/Software/SerialPort/SerialPort.cpp
#include "SerialPort.h"

SerialPort::SerialPort(SystemController& controller)
  : Module(controller,
           /* module_name */ "serial_port",
           /* nvs_key     */ "serial_port",
           /* can_be_disabled */ false)
{}

void SerialPort::begin(const ModuleConfig& /*cfg*/) {
    Serial.begin(baud_rate);
    delay(2000);
}

void SerialPort::loop() {
    // No periodic work; everything is pull-based via read_line() when needed.
}

void SerialPort::enable() {
    // always on
}

void SerialPort::disable() {
    // never disabled
}

void SerialPort::reset() {
    flush_input();
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

void SerialPort::print(const String& message) {
    Serial.print(message);
}

void SerialPort::println(const String& message) {
    Serial.println(message);
}

bool SerialPort::has_line() const {
    return Serial.available() > 0;
}

String SerialPort::read_line() {
    String line;
    while (true) {
        if (Serial.available()) {
            char c = Serial.read();
            yield();
            print(String(c));    // echo
            if (c == '\n') break;
            if (c != '\r')  line += c;
        } else {
            yield();
        }
    }
    line.trim();
    return line;
}

String SerialPort::get_string(const String message) {
    print(message);
    flush_input();
    return read_line();
}

int SerialPort::get_int(const String message) {
    String input = get_string(message);
    while (input.length() == 0) {
        input = get_string();
    }
    return input.toInt();
}

bool SerialPort::get_confirmation(const String message) {
    println(message);
    print("(y/n): ");
    String input = get_string();
    input.trim();
    input.toLowerCase();
    return (input == "y" || input == "yes" || input == "1" || input == "true");
}

bool SerialPort::prompt_user_yn(const String message, uint16_t timeout) {
    println(message);
    uint32_t start_time = millis();
    while (millis() - start_time < timeout) {
        print("(y/n)?: ");
        String input = get_string();
        input.trim();
        input.toLowerCase();
        if (input == "y") return true;
        if (input == "n") return false;
    }
    println("Timeout!");
    return false;
}

void SerialPort::print_spacer() {
    println("");
}
