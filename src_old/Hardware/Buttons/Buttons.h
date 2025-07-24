#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>
#include <vector>
#include <functional>

// Forward declaration of SystemController to avoid circular include issues
class SystemController;

class Buttons {
public:
    /**
     * @brief Constructor for the Buttons class.
     * @param system A reference to the main SystemController for command execution callbacks.
     */
    Buttons(SystemController& system);

    /**
     * @brief Loads a vector of configuration strings into live, active buttons.
     * @param configs A vector of strings, each containing a button's configuration.
     */
    void load_configs(const std::vector<String>& configs);

    /**
     * @brief The main loop function for the controller.
     * Must be called repeatedly to detect button presses.
     */
    void loop();

    void begin();

    /**
     * @brief Clears all configured buttons from memory. Does not affect NVS.
     */
    void reset();

    /**
     * @brief Tries to parse and add a new button to the live configuration.
     * This does NOT save to NVS.
     * @param config The configuration string (e.g., "9 \"$led toggle\" pullup on_press 50").
     * @return True if the button was parsed and added successfully, false otherwise.
     */
    bool add_button_from_config(const String& config);

    /**
     * @brief Removes a button from the live configuration based on its GPIO pin.
     * This does NOT save to NVS.
     * @param pin The GPIO pin of the button to remove.
     */
    void remove_button(uint8_t pin);

    /**
     * @brief Returns the status of all currently active (in-memory) buttons as a formatted string.
     * @return A String containing the list of live buttons for printing.
     */
    String get_live_status();

private:
    // Enum to define the hardware wiring of the button.
    enum InputMode {
        BUTTON_PULLUP,   // Button connects pin to GND. Uses internal pull-up. Pin is HIGH when idle.
        BUTTON_PULLDOWN  // Button connects pin to VCC. Uses internal pull-down. Pin is LOW when idle.
    };

    // Enum to define what physical action triggers the command.
    enum TriggerEvent {
        BUTTON_ON_PRESS,   // Triggers when the button is first pressed.
        BUTTON_ON_RELEASE, // Triggers when the button is released.
        BUTTON_ON_CHANGE   // Triggers on any state change.
    };
    // Struct to hold the complete configuration and state for a single physical button.
    struct Button {
        uint8_t pin;
        String command;
        uint32_t debounce_interval;
        InputMode type;
        TriggerEvent event;

        // State variables used for debouncing logic
        uint32_t last_debounce_time;
        int last_steady_state;
        int last_flicker_state;
    };

    /**
     * @brief Internal helper to parse a configuration string and populate a Button struct.
     * @param config The string to parse.
     * @param button The struct to populate.
     * @return True on success, false on failure.
     */
    bool parse_config_string(const String& config, Button& button);

    // Reference to the main system controller for command execution callbacks.
    SystemController& system;

    // A std::vector to store all configured and active button objects.
    std::vector<Button> buttons;
};

#endif // BUTTONS_H
