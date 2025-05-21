#include "SerialPort.h"

SerialPort::SerialPort(unsigned long baud_rate)
    : baud_rate_(baud_rate) {
    Serial.begin(baud_rate_);
    // Give USB-CDC a moment (milliseconds)
    delay(1500);
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
            if (c == '\n') {
                break;
            }
            if (c != '\r') {
                line += c;
            }
        } else {
            yield();
        }
    }
    line.trim();
    return line;
}

String SerialPort::get_string() {
    flush_input();
    return read_line();
}

int SerialPort::get_int() {
    String input = get_string();
    while (input.length() == 0) {
        input = get_string();
    }
    return input.toInt();
}

bool SerialPort::get_confirmation() {
    println("Press (y) to confirm");
    String input = get_string();
    input.trim();
    input.toLowerCase();
    return (input == "y" || input == "yes" || input == "1" || input == "true");
}

void SerialPort::print_spacer(){
    print("\n");
}