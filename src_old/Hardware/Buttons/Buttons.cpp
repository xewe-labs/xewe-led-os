#include "Buttons.h"
#include "../../SystemController/SystemController.h" // Include full header for command callbacks

// Constructor is simplified, no longer needs Nvs or SerialPort
Buttons::Buttons(SystemController& sys)
    : system(sys)
{}

// This method is now empty, as loading is explicitly called by SystemController
void Buttons::begin() {}

// This method is called by SystemController after it reads configs from NVS
void Buttons::load_configs(const std::vector<String>& configs) {
    for (const auto& config : configs) {
        if (!config.isEmpty()) {
            add_button_from_config(config);
        }
    }
}

void Buttons::loop() {
    // Iterate through each configured button to check its state
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

// Clears only the live, in-memory buttons
void Buttons::reset() {
    buttons.clear();
}

// Removes a single button from the live, in-memory vector
void Buttons::remove_button(uint8_t pin) {
    buttons.erase(std::remove_if(buttons.begin(), buttons.end(),
        [pin](const Button& btn) {
            return btn.pin == pin;
        }), buttons.end());
}

// Returns the status of live buttons as a string for printing by SystemController
String Buttons::get_live_status() {
    if (buttons.empty()) {
        return "No buttons are currently active in memory.";
    }
    String status_string = "--- Active Button Instances (Live) ---\n";
    for (const auto& btn : buttons) {
        status_string += "  - Pin: " + String(btn.pin) + ", CMD: \"" + btn.command + "\"\n";
    }
    status_string += "------------------------------------";
    return status_string;
}

// --- Private Methods ---

// Adds a button to the live vector. No NVS interaction.
bool Buttons::add_button_from_config(const String& config) {
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

bool Buttons::parse_config_string(const String& config, Button& button) {
    String temp_config = config;
    temp_config.trim();

    int first_space = temp_config.indexOf(' ');
    if (first_space == -1) return false;
    button.pin = temp_config.substring(0, first_space).toInt();
    temp_config = temp_config.substring(first_space + 1);
    temp_config.trim();

    if (temp_config.charAt(0) != '"') return false;
    if (temp_config.charAt(1) != '$') return false;
    int quote_end = temp_config.indexOf('"', 1);
    if (quote_end == -1) return false;
    button.command = temp_config.substring(1, quote_end);
    temp_config = temp_config.substring(quote_end + 1);
    temp_config.trim();

    String type_str = "pullup";
    String event_str = "on_press";
    String debounce_str = "50";

    if (!temp_config.isEmpty()) {
        first_space = temp_config.indexOf(' ');
        if (first_space == -1) {
            type_str = temp_config;
            temp_config = "";
        } else {
            type_str = temp_config.substring(0, first_space);
            temp_config = temp_config.substring(first_space + 1);
            temp_config.trim();
        }

        if (!temp_config.isEmpty()) {
            first_space = temp_config.indexOf(' ');
            if (first_space == -1) {
                event_str = temp_config;
                temp_config = "";
            } else {
                event_str = temp_config.substring(0, first_space);
                temp_config = temp_config.substring(first_space + 1);
                temp_config.trim();
            }

            if (!temp_config.isEmpty()) {
                debounce_str = temp_config;
            }
        }
    }

    button.type = (type_str == "pulldown") ? InputMode::BUTTON_PULLDOWN : InputMode::BUTTON_PULLUP;
    if (event_str == "release") {
        button.event = TriggerEvent::BUTTON_ON_RELEASE;
    } else if (event_str == "change") {
        button.event = TriggerEvent::BUTTON_ON_CHANGE;
    } else {
        button.event = TriggerEvent::BUTTON_ON_PRESS;
    }
    button.debounce_interval = debounce_str.toInt();
    if (button.debounce_interval == 0 && debounce_str != "0") {
        button.debounce_interval = 50;
    }

    return true;
}
