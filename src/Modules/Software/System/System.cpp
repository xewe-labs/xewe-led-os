#include "System.h"

System::System(SystemController& controller)
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
void System::begin(const ModuleConfig& cfg_base) {
}

void System::loop() {
}

void System::reset(bool verbose) {
}