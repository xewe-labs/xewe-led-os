// src/SystemController.cpp
#include "SystemController.h"
#include "Interfaces/Hardware/Nvs/Nvs.h"

SystemController::SystemController()
  : nvs(*this)
  , serial_port(*this)
  , command_parser(*this)
  , wifi(*this)
{
    modules[0] = &nvs;
    modules[1] = &serial_port;
    modules[2] = &command_parser;
    modules[3] = &wifi;

    serial_port.set_line_callback([this](std::string_view line) {
        command_parser.parse(line);
    });
}

void SystemController::begin() {
    // Initialize NVS namespace first
    NvsConfig nvs_cfg;
    nvs.begin(nvs_cfg);

    // Then serial port
    SerialPortConfig serial_cfg;
    serial_port.begin(serial_cfg);

    // Then Wi-Fi
    WifiConfig wifi_cfg;
    wifi.begin(wifi_cfg);

    // Build CLI command groups...
    command_groups.clear();
    for (auto module : modules) {
        auto grp = module->get_commands_group();
        if (!grp.commands.empty()) {
            command_groups.push_back(grp);
        }
    }

    // Hand off to the command parser
    ParserConfig parser_cfg;
    parser_cfg.groups      = command_groups.data();
    parser_cfg.group_count = command_groups.size();
    command_parser.begin(parser_cfg);
}

// (rest of file unchanged)
