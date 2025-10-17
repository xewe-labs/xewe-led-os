/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/



// src/Modules/Wifi/Wifi.cpp

#include "Wifi.h"
#include "../../../SystemController/SystemController.h"


Wifi::Wifi(SystemController& controller)
      : Module(controller,
               /* module_name         */ "Wifi",
               /* module_description  */ "Allows to connect to a local WiFi network\nfor extended control\nNOTE: Some WiFi networks (ex: cafes/hotspots)\nhave AP client isolation in that case you\ncan't use Web/HomeKit/Alexa features",
               /* nvs_key             */ "wf",
               /* requires_init_setup */ true,
               /* can_be_disabled     */ true,
               /* has_cli_cmds        */ true) {

    commands_storage.push_back({
        "connect",
        "Connect or reconnect to WiFi",
        std::string("Sample Use: $") + lower(module_name) + " connect",
        0,
        [this](std::string_view){ connect(true); }
    });
    commands_storage.push_back({
        "disconnect",
        "Disconnect from WiFi",
        std::string("Sample Use: $") + lower(module_name) + " disconnect",
        0,
        [this](std::string_view){ disconnect(true); }
    });
    commands_storage.push_back({
        "scan",
        "List available WiFi networks",
        std::string("Sample Use: $") + lower(module_name) + " scan",
        0,
        [this](std::string_view){ scan(true); }
    });
}

void Wifi::begin_routines_required (const ModuleConfig& cfg) {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(controller.system.get_device_name().c_str());
    disconnect(false);
    delay(100);
}

void Wifi::begin_routines_init (const ModuleConfig& cfg) {
    if (!connect(true)) // connect, and if not connected, then disable
        disable();
}

void Wifi::begin_routines_regular (const ModuleConfig& cfg) {
    connect(false);
}

//void Wifi::begin_routines_common (const ModuleConfig& cfg) {
//    // do your custom routines here
//}

void Wifi::loop () {
    if (is_disabled()) return;

    // enforce Wifi connection if the module is active
    while (WiFi.status() != WL_CONNECTED) {
        bool user_disabled = controller.serial_port.prompt_user_yn("Wifi connection lost\nReconnecting in 5 seconds\nDisable WiFi module?", 5000);
        if (user_disabled) {
            disable(true);
        }
        connect(true);
    }
}

void Wifi::reset (const bool verbose) {
    controller.nvs.remove(nvs_key, "ssid");
    controller.nvs.remove(nvs_key, "psw");
    disconnect(false);
    Module::reset(verbose);
}

void Wifi::enable (const bool verbose) {
    connect(true);
    Module::enable(verbose);;
}

void Wifi::disable (const bool verbose) {
    disconnect(false);
    Module::disable(verbose);;
}

std::string Wifi::status(bool verbose) const {
    DBG_PRINTF(Wifi, "status(verbose=%d)\n", verbose);
    Module::status(verbose);

    std::string status_string {};
    if (is_disconnected(true)) {
        status_string = "disconnected";
    } else if (is_connected()) {
        status_string = "Connected to " + get_ssid()
                      + "\nLocal ip: " + get_local_ip()
                      + "\nMac: " + get_mac_address();
        if (verbose) {
            controller.serial_port.println(status_string);
        }
    }
    return status_string;
}


bool Wifi::connect(bool prompt_for_credentials) {
    DBG_PRINTF(Wifi, "connect(prompt_for_credentials=%d)\n", prompt_for_credentials);
    if (is_disabled(true)) return false;
    if (is_connected(true)) return true;

    // First debug is already present

    std::string ssid, pwd;
    if (read_stored_credentials(ssid, pwd)) {
        DBG_PRINTLN(Wifi, "connect(): stored credentials found");

        controller.serial_port.println("Stored WiFi credentials found");
        if (join(ssid, pwd, 10000, 3)) {
            DBG_PRINTLN(Wifi, "connect(): join() succeeded with stored credentials");
            return true;
        } else {
            DBG_PRINTLN(Wifi, "connect(): join() failed with stored credentials");
            controller.serial_port.println("Stored WiFi credentials not valid.");
            if (!prompt_for_credentials) {
                controller.serial_port.println("Use '$wifi reset' to reset credentials");
            }
        }
    } else {
        DBG_PRINTLN(Wifi, "connect(): no stored credentials");
        controller.serial_port.println("Stored WiFi credentials not found");
        if (!prompt_for_credentials) {
            controller.serial_port.println("Type '$wifi connect' to select a new network");
        }
    }


    if (prompt_for_credentials) {
        while (is_disconnected()) {
            DBG_PRINTLN(Wifi, "connect(): prompting for credentials");
            uint8_t prompt_status = prompt_credentials(ssid, pwd);
            DBG_PRINTF(Wifi, "connect(): prompt_credentials returned %d\n", prompt_status);
            if (prompt_status == 1) {
                DBG_PRINTLN(Wifi, "connect(): user terminated setup");
                controller.serial_port.println("Terminated WiFi setup");
                return false;
            } else if (prompt_status == 2) {
                DBG_PRINTLN(Wifi, "connect(): invalid choice, retrying");
                continue;
            } else if (prompt_status == 3) {
                DBG_PRINTLN(Wifi, "connect(): invalid choice, retrying");
                controller.serial_port.println("Invalid choice");
                continue;
            } else {
                DBG_PRINTLN(Wifi, "connect(): attempting join() with user credentials");
                if (join(ssid, pwd, 10000, 1)) {
                    DBG_PRINTLN(Wifi, "connect(): join() succeeded with user credentials");
                    controller.nvs.write_str(nvs_key, "ssid", ssid);
                    controller.nvs.write_str(nvs_key, "psw", pwd);
                    return true;
                }
            }
        }
    }
    return false;
}

bool Wifi::disconnect(bool verbose) {
    // First debug: function name
    DBG_PRINTLN(Wifi, "disconnect()");
    if (is_disabled(verbose)) return true;
    if (is_disconnected(verbose)) return true;

    DBG_PRINTLN(Wifi, "disconnect(): start");
    WiFi.disconnect();
    unsigned long start = millis();
    constexpr unsigned long timeout = 5000;
    while (WiFi.status() == WL_CONNECTED && millis() - start < timeout) {
        delay(100);
    }
    bool done = (WiFi.status() != WL_CONNECTED);
    DBG_PRINTF(Wifi, "disconnect(): %s\n", done ? "success" : "timeout/failure");

    if (verbose) controller.serial_port.println("WiFi disconnected");
    return done;
}

bool Wifi::join(std::string_view ssid, std::string_view password, uint16_t timeout_ms, uint8_t retry_count) {
    // First debug: function name and parameters
    DBG_PRINTF(Wifi,
        "join(ssid='%.*s', password='%.*s')\n",
        int(ssid.size()), ssid.data(),
        int(password.size()), password.data()
    );
    if (is_disabled(true)) return false;

    for(uint8_t retry_counter = 0; retry_counter < retry_count; retry_counter++){
        controller.serial_port.print("Joining ");
        controller.serial_port.print(ssid.data());
        DBG_PRINTF(Wifi, "join(): ssid='%.*s'\n", int(ssid.size()), ssid.data());
        WiFi.begin(ssid.data(), password.data());
        unsigned long start = millis();

        while (millis() - start < timeout_ms) {
            controller.serial_port.print(".");
            if (WiFi.status() == WL_CONNECTED) {
                DBG_PRINTLN(Wifi, "join(): connected");
                // this is not printed for some reason
                std::string status_string = std::string("\nJoined ") + ssid.data()
                              + "\nLocal ip: " + get_local_ip()
                              + "\nMac: " + get_mac_address();
                controller.serial_port.println(status_string);
                return true;
            }
            delay(200);
        }
        WiFi.disconnect(true);
        controller.serial_port.print("\nUnable to join ");
        controller.serial_port.println(ssid.data());
        controller.serial_port.println("Check the password\ntry moving closer to router\nand restarting the router\nRetrying");
        DBG_PRINTLN(Wifi, "join(): timeout, disconnected");
    }
    if (retry_count > 1) {
        bool reset_credentials = controller.serial_port.prompt_user_yn("Would you like to reset credentials?", 10000);
        if (reset_credentials) reset();
    }

    return false;
}

std::vector<std::string> Wifi::scan(bool verbose) {
    // First debug: function name and parameter
    DBG_PRINTF(Wifi, "scan(verbose=%d)\n", verbose);
    if (is_disabled(true)) return {};

    DBG_PRINTLN(Wifi, "scan(): starting scan");
    controller.serial_port.println("Scanning WiFi networks...");
    int num_networks = WiFi.scanNetworks(true, true);
    while (num_networks == WIFI_SCAN_RUNNING) {
        delay(10);
        num_networks = WiFi.scanComplete();
    }
    DBG_PRINTF(Wifi, "scan(): scan complete, %d networks found\n", num_networks);

    std::vector<std::string> unique_ssid_list;
    if (num_networks > 0) {
        std::set<std::string> seen_ssids;
        unique_ssid_list.reserve(num_networks);
        for (int i = 0; i < num_networks; ++i) {
            String cur = WiFi.SSID(i);
            if (cur.isEmpty()) continue;
            std::string ssid(cur.c_str());
            if (seen_ssids.insert(ssid).second) {
                unique_ssid_list.push_back(ssid);
                DBG_PRINTF(Wifi, "scan(): adding [%s]\n", ssid.c_str());
            }
        }
    }

    if (verbose) {
        for (size_t j = 0; j < unique_ssid_list.size(); ++j) {
            char line[64];
            snprintf(line, sizeof(line), "%zu. %s", j, unique_ssid_list[j].c_str());
            controller.serial_port.println(line);
        }
    }

    WiFi.scanDelete();
    DBG_PRINTLN(Wifi, "scan(): done");
    return unique_ssid_list;
}

std::string Wifi::get_local_ip() const {
    // First debug: function name
    DBG_PRINTLN(Wifi, "get_local_ip()");
    if (is_disabled(true)) return {};
    if (is_disconnected(true)) return {};

    auto ip = WiFi.localIP();
    char buf[16];
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
             ip[0], ip[1], ip[2], ip[3]);
    DBG_PRINTF(Wifi, "get_local_ip(): %s\n", buf);
    return std::string(buf);
}

std::string Wifi::get_ssid() const {
    DBG_PRINTLN(Wifi, "get_ssid()");
    if (is_disabled(true)) return {};
    if (is_disconnected(true)) return {};
    return controller.nvs.read_str(nvs_key, "ssid");
}


std::string Wifi::get_mac_address() const {
    // First debug: function name
    DBG_PRINTLN(Wifi, "get_mac_address()");
    if (is_disabled(true)) return {};
    if (is_disconnected(true)) return {};

    uint8_t mac[6];
    WiFi.macAddress(mac);
    char buf[18];
    snprintf(buf, sizeof(buf),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2],
             mac[3], mac[4], mac[5]);
    DBG_PRINTF(Wifi, "get_mac_address(): %s\n", buf);
    return std::string(buf);
}

bool Wifi::read_stored_credentials(std::string& ssid, std::string& password) {
    // First debug: function name
    DBG_PRINTLN(Wifi, "read_stored_credentials()");
    if (is_disabled(true)) return false;
    DBG_PRINTLN(Wifi, "read_stored_credentials(): reading NVS");
    ssid = controller.nvs.read_str(nvs_key, "ssid");
    password = controller.nvs.read_str(nvs_key, "psw");
    DBG_PRINTF(Wifi, "read_stored_credentials(): %s\n", ssid.length() > 0 ? "found" : "none");
    return ssid.length() > 0;
}

uint8_t Wifi::prompt_credentials(std::string& ssid, std::string& password) {
    // First debug: function name
    DBG_PRINTLN(Wifi, "prompt_credentials()");
    if (is_disabled(true)) return 2;

    std::vector<std::string> networks = scan(true);
    int choice = controller.serial_port.get_int(
        "\nSelect network by number; or enter\n-1 to exit\n-2 to rescan\n-3 to enter custom SSID\nSelection: "
    );
    DBG_PRINTF(Wifi, "prompt_credentials(): user choice = %d\n", choice);
    if (choice == -1) {
        DBG_PRINTLN(Wifi, "prompt_credentials(): user exit");
        return 1;
    } else if (choice == -2) {
        DBG_PRINTLN(Wifi, "prompt_credentials(): user rescan");
        return 2;
    } else if (choice == -3) {
        DBG_PRINTLN(Wifi, "prompt_credentials(): user custom ssid");
        ssid = controller.serial_port.get_string("Enter custom SSID: ");
    } else if (choice >= 0 && choice < static_cast<int>(networks.size())) {
        ssid = networks[choice];
        DBG_PRINTF(Wifi, "prompt_credentials(): selected ssid = %s\n", ssid.c_str());
    } else {
        DBG_PRINTLN(Wifi, "prompt_credentials(): invalid choice");
        return 3;
    }

    password = controller.serial_port.get_string(
        "Selected: '" + ssid + "'\nPassword: "
    );
    DBG_PRINTLN(Wifi, "prompt_credentials(): password entered");
    DBG_PRINTF(Wifi,
        "prompt_credentials(): final ssid='%s', password='%s'\n",
        ssid.c_str(), password.c_str()
    );

    return 0;
}

bool Wifi::is_connected(bool verbose) const {
    // First debug: function name and parameter
    DBG_PRINTF(Wifi, "is_connected(verbose=%d)\n", verbose);
    bool conn = (WiFi.status() == WL_CONNECTED);
    if (verbose && conn) {
        DBG_PRINTLN(Wifi, "is_connected(): true");
        controller.serial_port.print("Connected to ");
        controller.serial_port.println(get_ssid().c_str());
    }
    DBG_PRINTF(Wifi, "is_connected(): %s\n", conn ? "true" : "false");
    return conn;
}

bool Wifi::is_disconnected(bool verbose) const {
    // First debug: function name and parameter
    DBG_PRINTF(Wifi, "is_disconnected(verbose=%d)\n", verbose);
    bool conn = (WiFi.status() == WL_CONNECTED);
    if (verbose && !conn) {
        DBG_PRINTLN(Wifi, "is_disconnected(): true");
        controller.serial_port.println("Not connected to WiFi; use $wifi connect");
    }
    DBG_PRINTF(Wifi, "is_disconnected(): %s\n", !conn ? "true" : "false");
    return !conn;
}
