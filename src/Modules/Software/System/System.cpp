#include "System.h"

#include "../../../SystemController/SystemController.h"

System::System(SystemController& controller)
      : Module(controller,
               /* module_name */ "system",
               /* nvs_key      */ "sys",
               /* requires_init_setup */ true,
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

bool System::init_setup(bool verbose, bool enable_prompt, bool reboot_after) {
    controller.serial_port.println("sample init setup for the system class");

    return true;
}

void System::begin(const ModuleConfig& cfg_base) {
    Module::begin(cfg_base);

}

void System::loop() {
}

void System::reset(bool verbose) {
}