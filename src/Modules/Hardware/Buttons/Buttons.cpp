// src/Modules/Buttons/Buttons.cpp

#include "Buttons.h"
#include "../../../SystemController/SystemController.h"

#include <Arduino.h>
#include <algorithm>

Buttons::Buttons(SystemController& controller)
      : Module(controller,
               /* module_name         */ "Buttons",
               /* module_description  */ "Allows to bind CLI cmds on the physical buttons",
               /* nvs_key             */ "bts",
               /* requires_init_setup */ true,
               /* can_be_disabled     */ true,
               /* has_cli_cmds        */ true),
        system(controller)
{}

void Buttons::begin_routines_required (const ModuleConfig& cfg) {
}

void Buttons::begin_routines_init (const ModuleConfig& cfg) {
}

void Buttons::begin_routines_regular (const ModuleConfig& cfg) {
}

void Buttons::begin_routines_common (const ModuleConfig& cfg) {
}

void Buttons::load_configs(const std::vector<std::string>& configs) {
    buttons.clear();
    for (const auto& cfg : configs) {
        if (!cfg.empty()) add_button_from_config(cfg);
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
                    system.execute_command(button.command);
                }
            }
        }
    }
}

void Buttons::reset (const bool verbose) {
    buttons.clear();
    Module::reset(verbose);
}

bool Buttons::enable (const bool verbose) {
    return Module::enable(verbose);
}

bool Buttons::disable (const bool verbose) {
    return Module::disable(verbose);
}

std::string Buttons::status (const bool verbose) const {
    if (buttons.empty()) return "No buttons are currently active in memory.";
    std::string s = "--- Active Button Instances (Live) ---\n";
    for (const auto& btn : buttons) {
        s += "  - Pin: " + std::to_string(btn.pin) + ", CMD: \"" + btn.command + "\"\n";
    }
    s += "------------------------------------";
    return s;
}

bool Buttons::add_button_from_config(const std::string& config) {
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

        buttons.push_back(new_button);
        return true;
    }
    return false;
}

void Buttons::remove_button(uint8_t pin) {
    buttons.erase(std::remove_if(buttons.begin(), buttons.end(),
        [pin](const Button& btn) { return btn.pin == pin; }), buttons.end());
}

std::string Buttons::get_live_status() const {
    return status(false);
}

static inline void trim(std::string& s) {
    const char* ws = " \t\r\n";
    s.erase(0, s.find_first_not_of(ws));
    s.erase(s.find_last_not_of(ws) + 1);
}

bool Buttons::parse_config_string(const std::string& config, Button& button) {
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

    button.type =
        (type_str == "pulldown") ? BUTTON_PULLDOWN : BUTTON_PULLUP;

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
