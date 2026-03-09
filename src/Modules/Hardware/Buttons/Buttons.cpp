/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Modules/Hardware/Buttons/Buttons.cpp


#include "Buttons.h"
#include "../../../SystemController/SystemController.h"


Buttons::Buttons(SystemController& controller)
      : Module(controller,
               /* module_name         */ "Buttons",
               /* module_description  */ "Allows to bind CLI cmds to physical buttons",
               /* nvs_key             */ "btn",
               /* requires_init_setup */ false,
               /* can_be_disabled     */ true,
               /* has_cli_cmds        */ true)
{
    commands_storage.push_back({
        "add",
        "Add a button mapping: <pin> \"<$cmd ...>\" [pullup|pulldown] [on_press|on_release|on_change] [debounce_ms]",
        std::string("$") + lower(module_name) + " add 9 \"$system reboot\" pullup on_press 50",
        5,
        [this](std::string_view args){ button_add_cli(args); }
    });
    commands_storage.push_back({
        "remove",
        "Remove ALL button mappings bound to a specific pin",
        std::string("$") + lower(module_name) + " remove 9",
        1,
        [this](std::string_view args){ button_remove_cli(args); }
    });
}

void Buttons::begin_routines_regular (const ModuleConfig& cfg) {
    if (is_enabled() && !loaded_from_nvs) {
        load_from_nvs();
    }
}

void Buttons::loop () {
    for (auto& button : buttons) {
        int current_state = digitalRead(button.pin);

        if (current_state != button.last_flicker_state) {
            button.last_debounce_time = millis();
        }
        button.last_flicker_state = current_state;

        if ((millis() - button.last_debounce_time) > button.debounce_interval) {
            if (current_state != button.last_steady_state) {
                button.last_steady_state = current_state;

                bool is_pressed = (button.type == InputMode::BUTTON_PULLUP) ? (current_state == LOW) : (current_state == HIGH);

                bool should_trigger = false;
                if (button.event == TriggerEvent::BUTTON_ON_CHANGE) {
                    should_trigger = true;
                } else if (button.event == TriggerEvent::BUTTON_ON_PRESS && is_pressed) {
                    should_trigger = true;
                } else if (button.event == TriggerEvent::BUTTON_ON_RELEASE && !is_pressed) {
                    should_trigger = true;
                }

                if (should_trigger) {
                    controller.command_parser.parse(button.command);
                }
            }
        }
    }
}

void Buttons::reset (const bool verbose, const bool do_restart, const bool keep_enabled) {
    nvs_clear_all();
    buttons.clear();
    Module::reset(verbose, do_restart, keep_enabled);
}

std::string Buttons::status (const bool verbose) const {
    if (is_disabled()) return "Buttons module disabled";

    std::string s = "";
    if (buttons.empty()) {
        s = "No buttons are currently active in memory.";
    } else {
        s = "--- Active Button Instances (Live) ---\n";
        for (const auto& btn : buttons) {
            s += "  - Pin: " + std::to_string(btn.pin) + ", ID: " + std::to_string(btn.b_id) + ", CMD: \"" + btn.command + "\"\n";
        }
        s += "------------------------------------";
    }
    if (verbose) controller.serial_port.print(s);
    return s;
}

void Buttons::load_configs(const std::vector<std::string>& configs) {
    if (is_disabled()) return;

    buttons.clear();
    for (const auto& cfg : configs) {
        if (!cfg.empty()) add_button_from_config(cfg);
    }
    loaded_from_nvs = true;
}

bool Buttons::add_button_from_config(const std::string& config) {
    if (is_disabled()) return false;

    Button new_button;
    if (parse_config_string(config, new_button)) {
        if (new_button.type == InputMode::BUTTON_PULLUP) {
            pinMode(new_button.pin, INPUT_PULLUP);
        } else {
            pinMode(new_button.pin, INPUT_PULLDOWN);
        }

        new_button.last_steady_state = digitalRead(new_button.pin);
        new_button.last_flicker_state = new_button.last_steady_state;
        new_button.last_debounce_time = 0;

        // Auto-assign b_id based on the highest existing b_id
        uint32_t next_b_id = 0;
        for (const auto& b : buttons) {
            if (b.b_id >= next_b_id) {
                next_b_id = b.b_id + 1;
            }
        }
        new_button.b_id = next_b_id;

        buttons.push_back(new_button);

        // Keep buttons sorted by b_id to naturally resolve execution priority
        std::sort(buttons.begin(), buttons.end(), [](const Button& a, const Button& b) {
            return a.b_id < b.b_id;
        });

        return true;
    }
    return false;
}

void Buttons::remove_button(uint8_t pin) {
    if (is_disabled()) return;

    // Removes all buttons bound to this pin
    buttons.erase(std::remove_if(buttons.begin(), buttons.end(),
        [pin](const Button& btn) { return btn.pin == pin; }), buttons.end());
}

bool Buttons::parse_config_string(const std::string& config, Button& button) {
    if (is_disabled()) return false;

    std::string s = config;
    trim(s);

    auto sp = s.find(' ');
    if (sp == std::string::npos) return false;
    try {
        button.pin = static_cast<uint8_t>(std::stoi(s.substr(0, sp)));
    } catch (...) { return false; }
    s = s.substr(sp + 1);
    trim(s);

    if (s.empty() || s[0] != '"') return false;
    auto endq = s.find('"', 1);
    if (endq == std::string::npos) return false;
    button.command = s.substr(1, endq - 1);
    s = s.substr(endq + 1);
    trim(s);

    std::string type_str = "pullup";
    std::string event_str = "on_press";
    std::string debounce_str = "50";

    if (!s.empty()) {
        sp = s.find(' ');
        if (sp == std::string::npos) { type_str = s; s.clear(); }
        else { type_str = s.substr(0, sp); s = s.substr(sp + 1); trim(s); }

        if (!s.empty()) {
            sp = s.find(' ');
            if (sp == std::string::npos) { event_str = s; s.clear(); }
            else { event_str = s.substr(0, sp); s = s.substr(sp + 1); trim(s); }

            if (!s.empty()) debounce_str = s;
        }
    }

    button.type = (type_str == "pulldown") ? BUTTON_PULLDOWN : BUTTON_PULLUP;

    if (event_str == "release" || event_str == "on_release") {
        button.event = BUTTON_ON_RELEASE;
    } else if (event_str == "change" || event_str == "on_change") {
        button.event = BUTTON_ON_CHANGE;
    } else {
        button.event = BUTTON_ON_PRESS;
    }

    try {
        button.debounce_interval = static_cast<uint32_t>(std::stoul(debounce_str));
    } catch (...) {
        button.debounce_interval = 50;
    }

    return true;
}

/* --- NVS helpers (encapsulated) --- */

void Buttons::load_from_nvs() {
    if (is_disabled()) return;

    int btn_count = controller.nvs.read_uint8(nvs_key, "btn_count", 0);
    std::vector<std::string> cfgs;
    cfgs.reserve(btn_count);
    for (int i = 0; i < btn_count; i++) {
        std::string key = "btn_cfg_" + std::to_string(i);
        std::string s = controller.nvs.read_str(nvs_key, key);
        if (!s.empty()) cfgs.emplace_back(std::move(s));
    }
    load_configs(cfgs);
}

bool Buttons::nvs_has_exact_config(const std::string& config_str) const {
    if (is_disabled()) return false;

    int btn_count = controller.nvs.read_uint8(nvs_key, "btn_count", 0);
    for (int i = 0; i < btn_count; i++) {
        std::string key = "btn_cfg_" + std::to_string(i);
        std::string existing = controller.nvs.read_str(nvs_key, key);
        if (existing == config_str) return true;
    }
    return false;
}

bool Buttons::nvs_remove_by_pin(const std::string& pin_str) {
    if (is_disabled()) return false;

    int btn_count = controller.nvs.read_uint8(nvs_key, "btn_count", 0);
    std::string prefix = pin_str + " ";
    std::vector<std::string> kept_configs;
    bool found = false;

    // Filter out configs matching the pin
    for (int i = 0; i < btn_count; i++) {
        std::string key = "btn_cfg_" + std::to_string(i);
        std::string cfg = controller.nvs.read_str(nvs_key, key);

        if (cfg.rfind(prefix, 0) == 0) {
            found = true;
        } else {
            kept_configs.push_back(cfg);
        }
    }

    if (!found) return false;

    // Rewrite NVS densely
    nvs_clear_all();
    for (const auto& cfg : kept_configs) {
        nvs_append_config(cfg);
    }

    return true;
}

void Buttons::nvs_append_config(const std::string& cfg) {
    if (is_disabled()) return;

    int btn_count = controller.nvs.read_uint8(nvs_key, "btn_count", 0);
    std::string key = "btn_cfg_" + std::to_string(btn_count);
    controller.nvs.write_str(nvs_key, key, cfg);
    controller.nvs.write_uint8(nvs_key, "btn_count", btn_count + 1);
}

void Buttons::nvs_clear_all() {
    if (is_disabled()) return;

    int btn_count = controller.nvs.read_uint8(nvs_key, "btn_count", 0);
    for (int i = 0; i < btn_count; i++) {
        std::string key = "btn_cfg_" + std::to_string(i);
        controller.nvs.remove(nvs_key, key);
    }
    controller.nvs.write_uint8(nvs_key, "btn_count", 0);
}

std::string Buttons::pin_prefix(const std::string& cfg) {
    if (is_disabled()) return "";

    auto sp = cfg.find(' ');
    if (sp == std::string::npos) return {};
    return cfg.substr(0, sp);
}

/* --- CLI handlers (called by ctor-registered lambdas) --- */
void Buttons::button_add_cli(std::string_view args_sv) {
    if (is_disabled()) return;

    if (!is_enabled()) {
        controller.serial_port.print("Buttons Module is disabled. Use '$buttons enable'");
        return;
    }

    std::string args(args_sv);
    std::string pin_str = pin_prefix(args);
    if (pin_str.empty()) {
        controller.serial_port.print("Error: Invalid add syntax.");
        return;
    }

    // Prevent adding the exact same mapping twice
    if (nvs_has_exact_config(args)) {
        std::string msg = "Error: This exact button configuration already exists.";
        controller.serial_port.print(msg);
        return;
    }

    if (add_button_from_config(args)) {
        nvs_append_config(args);
        std::string msg = "Successfully added button action: " + args;
        controller.serial_port.print(msg);
    } else {
        controller.serial_port.print("Error: Invalid button configuration string.");
    }
}

void Buttons::button_remove_cli(std::string_view args_sv) {
    if (is_disabled()) return;

    if (!is_enabled()) {
        controller.serial_port.print("Buttons Module is disabled. Use '$buttons enable'");
        return;
    }

    std::string pin_str(args_sv);
    trim(pin_str);

    if (pin_str.empty()) {
        controller.serial_port.print("Error: Invalid pin number provided.");
        return;
    }

    if (!nvs_remove_by_pin(pin_str)) {
        std::string msg = "Error: No active buttons found on pin " + pin_str;
        controller.serial_port.print(msg);
        return;
    }

    uint8_t pin_to_remove = static_cast<uint8_t>(std::atoi(pin_str.c_str()));
    remove_button(pin_to_remove);
    std::string msg = "Successfully removed ALL buttons mapped to pin " + pin_str;
    controller.serial_port.print(msg);
}