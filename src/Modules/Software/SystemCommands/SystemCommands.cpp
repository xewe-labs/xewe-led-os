#include "SystemCommands.h"

SystemCommands::SystemCommands(SystemController& controller)
      : Module(controller,
               /* module_name */ "system",
               /* nvs_key      */ "sys",
               /* can_be_disabled */ false,
               /* has_cli_cmds */ true)
    {
        commands_storage.push_back({
            "restart",
            "Restart the ESP",
            std::string("Sample Use: $") + lower(module_name) + " restart",
            0,
            [this](std::string_view args){ ESP.restart(); }
        });
        commands_storage.push_back({
            "reboot",
            "Restart the ESP",
            std::string("Sample Use: $") + lower(module_name) + " reboot",
            0,
            [this](std::string_view args){ ESP.restart(); }
        });
    }
void SystemCommands::begin(const ModuleConfig& cfg_base) {
}

void SystemCommands::loop() {
}

void SystemCommands::reset(bool verbose) {
}