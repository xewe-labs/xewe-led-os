// src/SystemController.cpp
#include "SystemController.h"
#include <cstring>

SystemController::SystemController()
  : serialPort(*this)
  , cmdParser(*this)
  , wifi(*this)
{
    modules[0] = &serialPort;
    modules[1] = &cmdParser;
    modules[2] = &wifi;
}

void SystemController::begin() {
    // 1) Serial port (no special config)
    ModuleConfig defaultCfg;
    serialPort.begin(defaultCfg);

    // 2) WiFi module
    WifiConfig wifiCfg;
    // You can override hostname here if desired:
    // wifiCfg.hostname = "MyCustomHost";
    wifi.begin(wifiCfg);

    // 3) Command parser gets the Wifi command‐group
    ParserConfig parserCfg;
    parserCfg.groups      = &wifi.get_command_group();
    parserCfg.group_count = 1;
    cmdParser.begin(parserCfg);
}

void SystemController::loop() {
    // Only act when a full line is available

    for (auto m : modules) {
        m->loop();
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
    } else if (std::strcmp(module_name, "wifi") == 0) {
        wifi.enable();
    }
}

void SystemController::disable_module(const char* module_name) {
    if (std::strcmp(module_name, "command_parser") == 0) {
        cmdParser.disable();
    } else if (std::strcmp(module_name, "wifi") == 0) {
        wifi.disable();
    }
}

void SystemController::reset_module(const char* module_name) {
    if (std::strcmp(module_name, "command_parser") == 0) {
        cmdParser.reset();
    } else if (std::strcmp(module_name, "wifi") == 0) {
        wifi.reset();
    }
}

const char* SystemController::module_status(const char* module_name) const {
    if (std::strcmp(module_name, "command_parser") == 0) {
        return cmdParser.status().data();
    } else if (std::strcmp(module_name, "serial_port") == 0) {
        return serialPort.status().data();
    } else if (std::strcmp(module_name, "wifi") == 0) {
        return wifi.status().data();
    }
    return nullptr;
}
