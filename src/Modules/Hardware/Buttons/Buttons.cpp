//// src/Modules/Buttons/Buttons.cpp
//
//#include "Buttons.h"
//#include "../../../SystemController/SystemController.h"
//
//#include <Arduino.h>
//#include <algorithm>
//#include <string_view>
//
//Buttons::Buttons(SystemController& controller)
//      : Module(controller,
//               /* module_name         */ "Buttons",
//               /* module_description  */ "Allows to bind CLI cmds on the physical buttons",
//               /* nvs_key             */ "bts",
//               /* requires_init_setup */ true,
//               /* can_be_disabled     */ true,
//               /* has_cli_cmds        */ true),
//        system(controller)
//{
//    commands_storage.push_back({
//        "status",
//        "Show configured buttons and live state",
//        std::string("Sample Use: $") + lower(module_name) + " status",
//        0,
//        [this](std::string_view) {
//            if (!is_enabled()) {
//                system.serial_port.println("Buttons Module is disabled. Use '$buttons enable'");
//                return;
//            }
//            system.serial_port.println("\n--- Saved Button Configurations (NVS) ---");
//            int btn_count = system.nvs.read_uint8("btn_count", 0);
//            if (btn_count == 0) {
//                system.serial_port.println("No buttons configured in storage.");
//            } else {
//                for (int i = 0; i < btn_count; i++) {
//                    String key = "btn_cfg_" + String(i);
//                    system.serial_port.println("  - " + system.nvs.read_str(key.c_str()));
//                }
//            }
//            system.serial_port.println("-----------------------------------------");
//            system.serial_port.println(String(get_live_status().c_str()));
//        }
//    });
//    commands_storage.push_back({
//        "reset",
//        "Clear all saved button configurations",
//        std::string("Sample Use: $") + lower(module_name) + " reset",
//        0,
//        [this](std::string_view) {
//            if (!is_enabled()) {
//                system.serial_port.println("Buttons Module is disabled. Use '$buttons enable'");
//                return;
//            }
//            if (system.serial_port.prompt_user_yn("Are you sure you want to delete ALL button configurations?")) {
//                nvs_clear_all();
//                buttons.clear();
//                system.serial_port.println("All button configurations have been reset.");
//                system.serial_port.get_string("Press enter to restart for changes to take full effect.");
//                reset(true);
//            }
//        }
//    });
//    commands_storage.push_back({
//        "add",
//        "Add a button mapping: <pin> \"<$cmd ...>\" [pullup|pulldown] [on_press|on_release|on_change] [debounce_ms]",
//        std::string("Sample Use: $") + lower(module_name) + " add 9 \"\\$led turn_off\" pullup on_press 50",
//        5,
//        [this](std::string_view args_sv) {
//            if (!is_enabled()) {
//                system.serial_port.println("Buttons Module is disabled. Use '$buttons enable'");
//                return;
//            }
//            std::string args(args_sv);
//            std::string pin_str = pin_prefix(args);
//            if (pin_str.empty()) {
//                system.serial_port.println("Error: Invalid add syntax.");
//                return;
//            }
//            if (nvs_has_pin(pin_str)) {
//                system.serial_port.println(String("Error: A button is already configured on pin ").concat(pin_str.c_str()));
//                return;
//            }
//            if (add_button_from_config(args)) {
//                nvs_append_config(args);
//                system.serial_port.println(String("Successfully added button action: ").concat(args.c_str()));
//            } else {
//                system.serial_port.println("Error: Invalid button configuration string.");
//            }
//        }
//    });
//    commands_storage.push_back({
//        "remove",
//        "Remove a button mapping by pin",
//        std::string("Sample Use: $") + lower(module_name) + " remove 9",
//        1,
//        [this](std::string_view args_sv) {
//            if (!is_enabled()) {
//                system.serial_port.println("Buttons Module is disabled. Use '$buttons enable'");
//                return;
//            }
//            std::string pin_str(std::string(args_sv));
//            if (pin_str.empty()) {
//                system.serial_port.println("Error: Invalid pin number provided.");
//                return;
//            }
//            if (!nvs_remove_by_pin(pin_str)) {
//                system.serial_port.println(String("Error: No button found on pin ").concat(pin_str.c_str()));
//                return;
//            }
//            uint8_t pin_to_remove = static_cast<uint8_t>(atoi(pin_str.c_str()));
//            remove_button(pin_to_remove);
//            system.serial_port.println(String("Successfully removed button on pin ").concat(pin_str.c_str()));
//        }
//    });
//    commands_storage.push_back({
//        "enable",
//        "Enable the Button Module",
//        std::string("Sample Use: $") + lower(module_name) + " enable",
//        0,
//        [this](std::string_view) {
//            enable(true);
//        }
//    });
//    commands_storage.push_back({
//        "disable",
//        "Disable the Button Module",
//        std::string("Sample Use: $") + lower(module_name) + " disable",
//        0,
//        [this](std::string_view) {
//            disable(true);
//        }
//    });
//}
//
//void Buttons::begin_routines_required (const ModuleConfig& cfg) {
//}
//
//void Buttons::begin_routines_init (const ModuleConfig& cfg) {
//}
//
//void Buttons::begin_routines_regular (const ModuleConfig& cfg) {
//    if (is_enabled() && !loaded_from_nvs) {
//        load_from_nvs();
//    }
//}
//
//void Buttons::begin_routines_common (const ModuleConfig& cfg) {
//}
//
//void Buttons::load_configs(const std::vector<std::string>& configs) {
//    buttons.clear();
//    for (const auto& cfg : configs) {
//        if (!cfg.empty()) add_button_from_config(cfg);
//    }
//    loaded_from_nvs = true;
//}
//
//void Buttons::loop () {
//    for (auto& button : buttons) {
//        int current_state = digitalRead(button.pin);
//
//        if (current_state != button.last_flicker_state) {
//            button.last_debounce_time = millis();
//        }
//        button.last_flicker_state = current_state;
//
//        if ((millis() - button.last_debounce_time) > button.debounce_interval) {
//            if (current_state != button.last_steady_state) {
//                button.last_steady_state = current_state;
//
//                bool is_pressed = (button.type == InputMode::BUTTON_PULLUP) ? (current_state == LOW) : (current_state == HIGH);
//
//                bool should_trigger = false;
//                if (button.event == TriggerEvent::BUTTON_ON_CHANGE) {
//                    should_trigger = true;
//                } else if (button.event == TriggerEvent::BUTTON_ON_PRESS && is_pressed) {
//                    should_trigger = true;
//                } else if (button.event == TriggerEvent::BUTTON_ON_RELEASE && !is_pressed) {
//                    should_trigger = true;
//                }
//
//                if (should_trigger) {
//                    system.execute_command(button.command);
//                }
//            }
//        }
//    }
//}
//
//void Buttons::reset (const bool verbose) {
//    buttons.clear();
//    nvs_clear_all();
//    Module::reset(verbose);
//}
////
////bool Buttons::enable (const bool verbose) {
////    bool ok = Module::enable(verbose);
////    return ok;
////}
////
////bool Buttons::disable (const bool verbose) {
//////    buttons.clear();
////    loaded_from_nvs = false;
////    return Module::disable(verbose);
////}
//
//std::string Buttons::status (const bool verbose) const {
//    if (buttons.empty()) return "No buttons are currently active in memory.";
//    std::string s = "--- Active Button Instances (Live) ---\n";
//    for (const auto& btn : buttons) {
//        s += "  - Pin: " + std::to_string(btn.pin) + ", CMD: \"" + btn.command + "\"\n";
//    }
//    s += "------------------------------------";
//    return s;
//}
//
//bool Buttons::add_button_from_config(const std::string& config) {
//    Button new_button;
//    if (parse_config_string(config, new_button)) {
//        if (new_button.type == InputMode::BUTTON_PULLUP) {
//            pinMode(new_button.pin, INPUT_PULLUP);
//        } else {
//            pinMode(new_button.pin, INPUT_PULLDOWN);
//        }
//
//        new_button.last_steady_state = digitalRead(new_button.pin);
//        new_button.last_flicker_state = new_button.last_steady_state;
//        new_button.last_debounce_time = 0;
//
//        buttons.push_back(new_button);
//        return true;
//    }
//    return false;
//}
//
//void Buttons::remove_button(uint8_t pin) {
//    buttons.erase(std::remove_if(buttons.begin(), buttons.end(),
//        [pin](const Button& btn) { return btn.pin == pin; }), buttons.end());
//}
//
//std::string Buttons::get_live_status() const {
//    return status(false);
//}
//
//static inline void trim(std::string& s) {
//    const char* ws = " \t\r\n";
//    s.erase(0, s.find_first_not_of(ws));
//    s.erase(s.find_last_not_of(ws) + 1);
//}
//
//bool Buttons::parse_config_string(const std::string& config, Button& button) {
//    std::string s = config;
//    trim(s);
//
//    auto sp = s.find(' ');
//    if (sp == std::string::npos) return false;
//    try {
//        button.pin = static_cast<uint8_t>(std::stoi(s.substr(0, sp)));
//    } catch (...) { return false; }
//    s = s.substr(sp + 1);
//    trim(s);
//
//    if (s.empty() || s[0] != '"') return false;
//    auto endq = s.find('"', 1);
//    if (endq == std::string::npos) return false;
//    button.command = s.substr(1, endq - 1);
//    s = s.substr(endq + 1);
//    trim(s);
//
//    std::string type_str = "pullup";
//    std::string event_str = "on_press";
//    std::string debounce_str = "50";
//
//    if (!s.empty()) {
//        sp = s.find(' ');
//        if (sp == std::string::npos) { type_str = s; s.clear(); }
//        else { type_str = s.substr(0, sp); s = s.substr(sp + 1); trim(s); }
//
//        if (!s.empty()) {
//            sp = s.find(' ');
//            if (sp == std::string::npos) { event_str = s; s.clear(); }
//            else { event_str = s.substr(0, sp); s = s.substr(sp + 1); trim(s); }
//
//            if (!s.empty()) debounce_str = s;
//        }
//    }
//
//    button.type =
//        (type_str == "pulldown") ? BUTTON_PULLDOWN : BUTTON_PULLUP;
//
//    if (event_str == "release" || event_str == "on_release") {
//        button.event = BUTTON_ON_RELEASE;
//    } else if (event_str == "change" || event_str == "on_change") {
//        button.event = BUTTON_ON_CHANGE;
//    } else {
//        button.event = BUTTON_ON_PRESS;
//    }
//
//    try {
//        button.debounce_interval = static_cast<uint32_t>(std::stoul(debounce_str));
//    } catch (...) {
//        button.debounce_interval = 50;
//    }
//
//    return true;
//}
//
///* --- NVS helpers (encapsulated) --- */
//
//void Buttons::load_from_nvs() {
//    int btn_count = system.nvs.read_uint8(nvs_key, "btn_count", 0);
//    std::vector<std::string> cfgs;
//    cfgs.reserve(btn_count);
//    for (int i = 0; i < btn_count; i++) {
//        String key = "btn_cfg_" + String(i);
//        String s = system.nvs.read_str(nvs_key, key.c_str());
//        if (s.length()) cfgs.emplace_back(s.c_str());
//    }
//    load_configs(cfgs);
//}
//
//bool Buttons::nvs_has_pin(const std::string& pin_str) const {
//    int btn_count = system.nvs.read_uint8(nvs_key, "btn_count", 0);
//    for (int i = 0; i < btn_count; i++) {
//        String key = "btn_cfg_" + String(i);
//        String existing = system.nvs.read_str(key.c_str());
//        if (existing.startsWith(String(pin_str.c_str()) + " ")) return true;
//    }
//    return false;
//}
//
//bool Buttons::nvs_remove_by_pin(const std::string& pin_str) {
//    int btn_count = system.nvs.read_uint8(nvs_key, "btn_count", 0);
//    int found_index = -1;
//    for (int i = 0; i < btn_count; i++) {
//        String key = "btn_cfg_" + String(i);
//        String cfg = system.nvs.read_str(nvs_key, key.c_str());
//        if (cfg.startsWith(String(pin_str.c_str()) + " ")) { found_index = i; break; }
//    }
//    if (found_index == -1) return false;
//
//    for (int i = found_index; i < btn_count - 1; i++) {
//        String next_key = "btn_cfg_" + String(i + 1);
//        String cur_key  = "btn_cfg_" + String(i);
//        String next_cfg = system.nvs.read_str(nvs_key, next_key.c_str());
//        system.nvs.write_str(cur_key.c_str(), next_cfg);
//    }
//    String last_key = "btn_cfg_" + String(btn_count - 1);
//    system.nvs.remove(nvs_key, last_key.c_str());
//    system.nvs.write_uint8(nvs_key, "btn_count", btn_count - 1);
//    return true;
//}
//
//void Buttons::nvs_append_config(const std::string& cfg) {
//    int btn_count = system.nvs.read_uint8("btn_count", 0);
//    String key = "btn_cfg_" + String(btn_count);
//    system.nvs.write_str(nvs_key, key.c_str(), String(cfg.c_str()));
//    system.nvs.write_uint8(nvs_key, "btn_count", btn_count + 1);
//}
//
//void Buttons::nvs_clear_all() {
//    int btn_count = system.nvs.read_uint8(nvs_key, "btn_count", 0);
//    for (int i = 0; i < btn_count; i++) {
//        system.nvs.remove(nvs_key, ("btn_cfg_" + String(i)).c_str());
//    }
//    system.nvs.write_uint8(nvs_key, "btn_count", 0);
//}
//
//std::string Buttons::pin_prefix(const std::string& cfg) {
//    auto sp = cfg.find(' ');
//    if (sp == std::string::npos) return {};
//    return cfg.substr(0, sp);
//}
