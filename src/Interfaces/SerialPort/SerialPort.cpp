#include "SerialPort.h"

SerialPort::SerialPort() {}

bool SerialPort::begin (uint32_t baud_rate) {
    baud_rate_ = baud_rate_;
    Serial.begin(baud_rate);
    delay(1500);
    return true;
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
            print(String(c));
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

String SerialPort::get_string(const String message) {
    print(message);
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
    print("(y/n): ");
    String input = get_string();
    input.trim();
    input.toLowerCase();
    return (input == "y" || input == "yes" || input == "1" || input == "true");
}

bool SerialPort::prompt_user_yn(const String message, uint16_t timeout) {
    println(message);
    uint32_t start_time = millis();
    while(millis() - start_time < timeout){
        print("(y/n)?: ");
        String input = get_string();
        input.trim();
        input.toLowerCase();

        if (input == "y") {
            return true;
        }
        if (input == "n") {
            return false;
        }
    }
    print("Timeout!");
    return false;
}


void SerialPort::print_spacer(){
    print("\n");
}