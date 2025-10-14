// src/Modules/Software/System/System.cpp

#include "System.h"
#include "../../../SystemController/SystemController.h"


System::System(SystemController& controller)
      : Module(controller,
               /* module_name         */ "",
               /* module_description  */ "",
               /* nvs_key             */ "",
               /* requires_init_setup */ false,
               /* can_be_disabled     */ false,
               /* has_cli_cmds        */ false)
{}


void System::begin_routines_required (const ModuleConfig& cfg) {
    controller.serial_port.print("\n\n\n\n\n");
    controller.serial_port.print_spacer();
    controller.serial_port.print_centered("XeWe LED OS");
    controller.serial_port.print_spacer();
    controller.serial_port.print_centered("Version 2.0");
    controller.serial_port.print_centered("https://github.com/maxdokukin/XeWe-LED-OS");
    controller.serial_port.print_spacer();
    controller.serial_port.print_centered("ESP32 OS to control ");
    controller.serial_port.print_centered("addressable LED lights");
    controller.serial_port.print_spacer();
    controller.serial_port.print_centered("Communication supported:");
    controller.serial_port.print_centered("");
    controller.serial_port.print_centered("Alexa");
    controller.serial_port.print_centered("HomeKit");
    controller.serial_port.print_centered("Web Browser");
    controller.serial_port.print_centered("Serial Port CLI");
    controller.serial_port.print_centered("Physical Buttons");
    controller.serial_port.print_spacer();
    controller.serial_port.print("\n\n\n\n\n");
}

void System::begin_routines_init (const ModuleConfig& cfg) {
    controller.serial_port.print_spacer();
    controller.serial_port.print_centered("Set Up Flow");
    controller.serial_port.print_centered("");
    controller.serial_port.print_centered("- Device Name                          ");
    controller.serial_port.print_centered("- LED Strip                            ");
    controller.serial_port.print_centered("- // Buttons                           ");
    controller.serial_port.print_centered("- WiFi                                 ");
    controller.serial_port.print_centered("- Web Interface           REQUIRES WiFi");
    controller.serial_port.print_centered("- HomeKit                 REQUIRES WiFi");
    controller.serial_port.print_centered("- Alexa                   REQUIRES WiFi");
    controller.serial_port.print_spacer();

    controller.serial_port.print_spacer();
    controller.serial_port.print_centered("Name Your Device");
    controller.serial_port.print_spacer();
    controller.serial_port.print_centered("Set the name your device will proudly hold until");
    controller.serial_port.print_centered("the last electron leaves it");
    controller.serial_port.print_centered("Sample names: \"Desk Lights\" or \"Ceiling Lights\"");

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
}

std::string System::get_device_name () { return controller.nvs.read_str(nvs_key, "dname"); };
