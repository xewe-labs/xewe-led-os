// src/SystemController.cpp

#include "SystemController.h"
#include <Arduino.h>    // for Serial, String
#include <cstring>

SystemController::SystemController()
  : cmdParser(*this)      // pass self into CommandParser ctor
{
    modules[0] = &cmdParser;
}

void SystemController::begin() {
    // Build an array of CommandGroup for all modules that expose commands.
    static const CommandGroup allGroups[] = {
        cmdParser.get_commands_group()
    };

    // Initialize the parser with that array
    ParserConfig parserCfg;
    parserCfg.groups      = allGroups;
    parserCfg.group_count = sizeof(allGroups) / sizeof(allGroups[0]);
    cmdParser.begin(parserCfg);
}

void SystemController::loop() {
    // Read a line from Serial and dispatch it to each module
    if (!Serial.available()) return;

    String line = Serial.readStringUntil('\n');
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
        // Convert std::string_view to C-string
        return cmdParser.status().data();
    }
    return nullptr;
}
