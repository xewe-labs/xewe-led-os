// src/SystemController.cpp
#include "SystemController.h"

SystemController::SystemController()
  : serial_port(*this)
  , nvs(*this)
  , system(*this)
  , command_parser(*this)
  , led_strip(*this)
  , wifi(*this)
  , web(*this)
//  , homekit(*this)
//  , alexa(*this)
{
    modules[0] = &serial_port;
    modules[1] = &nvs;
    modules[2] = &command_parser;
    modules[3] = &system;
    modules[4] = &led_strip;
    modules[5] = &wifi;
    modules[6] = &web;
//    modules[7] = &homekit;
//    modules[8] = &alexa;

    interfaces[0] = &led_strip;
    interfaces[1] = &nvs;
    interfaces[2] = &web;
//    interfaces[3] = homekit;
//    interfaces[4] = alexa;
}

void SystemController::begin() {
    SerialPortConfig serial_cfg;
    serial_port.begin(serial_cfg);

    NvsConfig nvs_cfg;
    nvs.begin(nvs_cfg);

    SystemConfig system_cfg;
    system.begin(system_cfg);

    LedStripConfig led_strip_cfg;
    led_strip.begin(led_strip_cfg);

    WifiConfig wifi_cfg;
    wifi.begin(wifi_cfg);

    WebConfig web_cfg;
    web_cfg.wifi_enabled = wifi.is_enabled();
    web_cfg.device_name  = String(get_name().c_str());
    web.begin(web_cfg);

//    HomekitConfig homekit_cfg;
//    homekit.begin(homekit_cfg);

//    nvs.sync_from_memory();

    command_groups.clear();
    for (auto module : modules) {
        if (module != nullptr) {
            auto grp = module->get_commands_group();
            if (!grp.commands.empty()) {
                command_groups.push_back(grp);
            }
        }
    }

    ParserConfig parser_cfg;
    parser_cfg.groups      = command_groups.data();
    parser_cfg.group_count = command_groups.size();
    command_parser.begin(parser_cfg);
}

void SystemController::loop() {
    // --- Performance Timing Setup ---
    // Using static variables to retain values across calls without modifying the header.
    static unsigned long last_report_time_ms = 0;
    const unsigned long REPORT_INTERVAL_MS = 5000; // Report every 5 seconds.

    // Accumulators for timing data in microseconds. One extra for the command parser.
    static unsigned long timing_accumulators[MODULE_COUNT + 1] = {0};
    static unsigned long loop_counter = 0;

    unsigned long start_time_us;

    // --- Time Each Module's Loop ---
    for (size_t i = 0; i < MODULE_COUNT; ++i) {
        if (modules[i] != nullptr) {
            start_time_us = micros();
            modules[i]->loop();
            timing_accumulators[i] += micros() - start_time_us;
        }
    }

    // --- Time Command Parser Logic ---
    start_time_us = micros();
    if (serial_port.has_line()) {
        command_parser.parse(serial_port.read_line());
    }
    // Add elapsed time to the dedicated accumulator for the parser.
    timing_accumulators[MODULE_COUNT] += micros() - start_time_us;

    loop_counter++;

    // --- Reporting Logic ---
    unsigned long current_millis = millis();
    if (current_millis - last_report_time_ms >= REPORT_INTERVAL_MS) {
        Serial.println();
        Serial.println("--- Loop Performance Report ---");
        Serial.printf("Averages over %lu loops (%lu ms interval):\n", loop_counter, REPORT_INTERVAL_MS);

        // Print timing for each module
        for (size_t i = 0; i < MODULE_COUNT; ++i) {
            if (modules[i] != nullptr) {
                float avg_time_us = (float)timing_accumulators[i] / loop_counter;
                std::string module_name = modules[i]->get_commands_group().name;
                if (module_name.empty()) {
                    module_name = "Module " + std::to_string(i);
                }
                Serial.printf("  - %-15s: %.2f µs\n", module_name.c_str(), avg_time_us);
            }
        }

        // Print timing for the parser
        float avg_parser_time_us = (float)timing_accumulators[MODULE_COUNT] / loop_counter;
        Serial.printf("  - %-15s: %.2f µs\n", "Parser Logic", avg_parser_time_us);
        Serial.println("-------------------------------");

        // Reset for the next reporting interval
        last_report_time_ms = current_millis;
        loop_counter = 0;
        for (size_t i = 0; i < MODULE_COUNT + 1; ++i) {
            timing_accumulators[i] = 0;
        }
    }
}

void SystemController::reset() {
    for (size_t i = 0; i < MODULE_COUNT; ++i) {
        if (modules[i] != nullptr) {
            modules[i]->reset();
        }
    }
}

std::string SystemController::status() const {
    return std::string("ok");
}

void SystemController::module_enable(std::string_view module_name) {
    for (auto module : modules) {
        if (module != nullptr && module->get_commands_group().name == module_name) {
            module->enable();
            return;
        }
    }
    Serial.printf("Error: Module '%s' not found\n", module_name.data());
}

void SystemController::module_disable(std::string_view module_name) {
    for (auto module : modules) {
        if (module != nullptr && module->get_commands_group().name == module_name) {
            module->disable();
            return;
        }
    }
    Serial.printf("Error: Module '%s' not found\n", module_name.data());
}

void SystemController::module_reset(std::string_view module_name) {
    for (auto module : modules) {
        if (module != nullptr && module->get_commands_group().name == module_name) {
            module->reset();
            return;
        }
    }
    Serial.printf("Error: Module '%s' not found\n", module_name.data());
}

std::string SystemController::module_status(std::string_view module_name) const {
    for (auto module : modules) {
        if (module != nullptr && module->get_commands_group().name == module_name) {
            return module->status();
        }
    }
    return std::string("unknown");
}

void SystemController::module_print_help(std::string_view module_name) {
    command_parser.print_help(std::string(module_name));
}

void SystemController::sync_color(std::array<uint8_t,3> color, std::array<uint8_t,INTERFACE_COUNT> sync_flags) {
    for (int i = 0; i < INTERFACE_COUNT; i++) {
        if (interfaces[i] != nullptr && sync_flags[i]) {
            interfaces[i]->sync_color(color);
        }
    }
}

void SystemController::sync_brightness(uint8_t brightness, std::array<uint8_t,INTERFACE_COUNT> sync_flags) {
    for (int i = 0; i < INTERFACE_COUNT; i++) {
        if (interfaces[i] != nullptr && sync_flags[i]) {
            interfaces[i]->sync_brightness(brightness);
        }
    }
}

void SystemController::sync_state(uint8_t state, std::array<uint8_t,INTERFACE_COUNT> sync_flags) {
    for (int i = 0; i < INTERFACE_COUNT; i++) {
        if (interfaces[i] != nullptr && sync_flags[i]) {
            interfaces[i]->sync_state(state);
        }
    }
}

void SystemController::sync_mode(uint8_t mode, std::array<uint8_t,INTERFACE_COUNT> sync_flags) {
    for (int i = 0; i < INTERFACE_COUNT; i++) {
        if (interfaces[i] != nullptr && sync_flags[i]) {
            interfaces[i]->sync_mode(mode);
        }
    }
}

void SystemController::sync_length(uint16_t length, std::array<uint8_t,INTERFACE_COUNT> sync_flags) {
    for (int i = 0; i < INTERFACE_COUNT; i++) {
        if (interfaces[i] != nullptr && sync_flags[i]) {
            interfaces[i]->sync_length(length);
        }
    }
}

void SystemController::sync_all(std::array<uint8_t,3> color, uint8_t brightness, uint8_t state, uint8_t mode, uint16_t length, std::array<uint8_t,INTERFACE_COUNT> sync_flags) {
    for (int i = 0; i < INTERFACE_COUNT; i++) {
        if (interfaces[i] != nullptr && sync_flags[i]) {
            interfaces[i]->sync_all(color, brightness, state, mode, length);
        }
    }
}

std::string SystemController::get_name() {
    return std::string("Test Lights");
}