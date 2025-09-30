#include "System.h"

#include "../../../SystemController/SystemController.h"

System::System(SystemController& controller) :
    Module(
        controller,
        /* module_name */ "system",
        /* nvs_key      */ "sys",
        /* requires_init_setup */ true,
        /* can_be_disabled */ false,
        /* has_cli_cmds */ true
    ) {
    DBG_PRINTLN(System, "System: Constructor called.");
    DBG_PRINTLN(System, "System: Registering 'restart' and 'reboot' commands.");

    commands_storage.push_back({
        "restart",
        "Restart the ESP",
        std::string("Sample Use: $") + lower(module_name) + " restart",
        0,
        [this](std::string_view args) {
            DBG_PRINTLN(System, "System: 'restart' command issued. Rebooting now.");
            ESP.restart();
        }
    });

    commands_storage.push_back({
        "reboot",
        "Restart the ESP",
        std::string("Sample Use: $") + lower(module_name) + " reboot",
        0,
        [this](std::string_view args) {
            DBG_PRINTLN(System, "System: 'reboot' command issued. Rebooting now.");
            ESP.restart();
        }
    });
}

bool System::init_setup(bool verbose, bool enable_prompt, bool reboot_after) {
    DBG_PRINTF(
        System,
        "System->init_setup(verbose=%s, prompt=%s, reboot=%s): Called.\n",
        verbose ? "true" : "false",
        enable_prompt ? "true" : "false",
        reboot_after ? "true" : "false"
    );

    controller.serial_port.print(
        "\n+------------------------------------------------+\n"
        "|       Alright lets set things up for you       |\n"
        "+------------------------------------------------+\n"
    );

    controller.serial_port.print(
        "+------------------------------------------------+\n"
        "|                   Set up flow                  |\n"
        "|                                                |\n"
        "|    - Device Name                               |\n"
        "|    - LED Strip                                 |\n"
        "|    - Buttons                                   |\n"
        "|    - WiFi                                      |\n"
        "|    - Web Interface           REQUIRES WiFi     |\n"
        "|    - Alexa                   REQUIRES WiFi     |\n"
        "|    - HomeKit                 REQUIRES WiFi     |\n"
        "+------------------------------------------------+\n"
    );

    controller.serial_port.print(
        "\n+------------------------------------------------+\n"
        "|                 Name Your Device               |\n"
        "+------------------------------------------------+\n"
    );
    controller.serial_port.println(
        "Set the name your device will proudly hold until\n"
        "the last electron leaves it\n"
        "Sample names: \"Desk Lights\" or \"Ceiling Lights\"\n"
    );

    std::string device_name;
    bool confirmed = false;
    while (!confirmed) {
        device_name = controller.serial_port.get_string("Enter device name: ");
        confirmed = controller.serial_port.prompt_user_yn("Confirm name: " + device_name);
    }
    controller.nvs.write_str(nvs_key, "dname", device_name);
    controller.serial_port.get_string(
        "\nDevice name setup success!\n"
        "Press enter to continue"
    );

    DBG_PRINTLN(System, "System->init_setup(): Complete.");
    return true;
}

void System::begin(const ModuleConfig& cfg_base) {
    DBG_PRINTLN(System, "System->begin(): Called.");
    controller.serial_port.print(
        "\n\n\n+------------------------------------------------+\n"
        "|                   XeWe LED OS                  |\n"
        "+------------------------------------------------+\n"
        "|                   Version 2.0                  |\n"
        "|    https://github.com/maxdokukin/XeWe-LED-OS   |\n"
        "+------------------------------------------------+\n"
        "|              ESP32 OS to control               |\n"
        "|             addressable LED lights             |\n"
        "+------------------------------------------------+\n"
        "|            Communication supported:            |\n"
        "|                                                |\n"
        "|                      Alexa                     |\n"
        "|                     HomeKit                    |\n"
        "|                   Web Browser                  |\n"
        "|                 Serial Port CLI                |\n"
        "|                Physical Buttons                |\n"
        "+------------------------------------------------+\n"
    );
    Module::begin(cfg_base);
}

void System::loop() {
    // No debug statement here to avoid spamming the serial console
}

void System::reset(bool verbose) {
    DBG_PRINTF(System, "System->reset(verbose=%s): Called (no action taken).\n", verbose ? "true" : "false");
}

std::string            System::get_device_name             () { return controller.nvs.read_str(nvs_key, "dname"); };

//void SystemController::system_status() {
//    serial_port.print(String("\n+------------------------------------------------+\n") +
//                      "|              System Configuration              |\n" +
//                      "|                                                |\n" +
//                      "| Buttons        : " + (buttons_module_active ? "enabled " : "disabled") + "                      |\n" +
//                      "| WiFi           : " + (wifi_module_active ? "enabled " : "disabled") + "                      |\n" +
//                      "| Web Interface  : " + (webinterface_module_active ? "enabled " : "disabled") + "                      |\n" +
//                      "| Alexa          : " + (alexa_module_active ? "enabled " : "disabled") + "                      |\n" +
//                      "| HomeKit        : " + (homekit_module_active ? "enabled " : "disabled") + "                      |\n" +
//                      "+------------------------------------------------+\n");
//}
//
//void SystemController::system_reset(){
//    DBG_PRINTLN(SystemController, "system_reset()");
//    serial_port.println("\n+------------------------------------------------+\n"
//                        "|                  Resetting...                  |\n"
//                        "+------------------------------------------------+\n");
//    nvs.reset();
//    serial_port.println("NOTE: You need to manually remove device from\nAlexa and Apple Home Apps!");
//    serial_port.get_string("System reset success!\n\nPress enter to restart");
//    system_restart(1000);
//}
//
//void SystemController::system_restart(uint16_t delay_before){
//    DBG_PRINTLN(SystemController, "system_restart()");
//    serial_port.println("\n+------------------------------------------------+\n"
//                        "|                 Restarting...                  |\n"
//                        "+------------------------------------------------+\n");
//    nvs.commit();
//    delay(delay_before);
//    ESP.restart();
//}

