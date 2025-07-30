#include "SystemController.h"
#include <cstring>

SystemController::SystemController()
  : serialPort(*this)
  , cmdParser(*this)
{
    modules[0] = &serialPort;
    modules[1] = &cmdParser;
}

void SystemController::begin() {
    // Initialize all modules
    for (auto m : modules) {
        m->begin(ModuleConfig{});
    }
}

void SystemController::loop() {
    // If the serial port has a full line, dispatch it to each module
    if (!serialPort.has_line()) return;

    String line = serialPort.read_line();
    std::string input(line.c_str());
    for (auto m : modules) {
        m->loop(input);
    }
}

void SystemController::enable() {
    enabled = true;
    for (auto m : modules) {
        m->enable();
    }
}

void SystemController::disable() {
    enabled = false;
    for (auto m : modules) {
        m->disable();
    }
}

void SystemController::reset() {
    for (auto m : modules) {
        m->reset();
    }
}

const char* SystemController::status() const {
    return enabled ? "enabled" : "disabled";
}

void SystemController::enable_module(const char* module_name) {
    if (std::strcmp(module_name, "command_parser") == 0) {
        cmdParser.enable();
    }
}

void SystemController::disable_module(const char* module_name) {
    if (std::strcmp(module_name, "command_parser") == 0) {
        cmdParser.disable();
    }
}

void SystemController::reset_module(const char* module_name) {
    if (std::strcmp(module_name, "command_parser") == 0) {
        cmdParser.reset();
    }
}

const char* SystemController::module_status(const char* module_name) const {
    if (std::strcmp(module_name, "command_parser") == 0) {
        return cmdParser.status().data();
    } else if (std::strcmp(module_name, "serial_port") == 0) {
        return serialPort.status().data();
    }
    return nullptr;
}
