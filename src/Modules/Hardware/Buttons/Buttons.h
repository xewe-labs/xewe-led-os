/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Modules/Hardware/Buttons/Buttons.h
#pragma once

#include "../../Module/Module.h"

struct ButtonsConfig : public ModuleConfig {};


class Buttons : public Module {
public:
    explicit                    Buttons                     (SystemController& controller);

    void                        begin_routines_regular      (const ModuleConfig& cfg)       override;

    void                        loop                        ()                              override;

    void                        reset                       (const bool verbose=false,
                                                             const bool do_restart=true,
                                                             const bool keep_enabled=true)    override;

    string                      status                      (const bool verbose=false)      const override;

    void                        load_configs                (const std::vector<std::string>& configs);
    bool                        add_button_from_config      (const std::string& config);
    void                        remove_button               (uint8_t pin);

private:
    enum                        InputMode                   { BUTTON_PULLUP, BUTTON_PULLDOWN };
    enum                        TriggerEvent                { BUTTON_ON_PRESS, BUTTON_ON_RELEASE, BUTTON_ON_CHANGE };

    struct Button {
        uint32_t b_id;
        uint8_t pin;
        std::string command;
        uint32_t debounce_interval;
        InputMode type;
        TriggerEvent event;
        uint32_t last_debounce_time;
        int last_steady_state;
        int last_flicker_state;
    };

    bool                        parse_config_string         (const std::string& config, Button& button);

    void                        load_from_nvs               ();
    bool                        nvs_has_exact_config        (const std::string& config_str) const; // <-- Replaced nvs_has_pin
    bool                        nvs_remove_by_pin           (const std::string& pin_str);
    void                        nvs_append_config           (const std::string& cfg);
    void                        nvs_clear_all               ();
    std::string                 pin_prefix                  (const std::string& cfg);

    void                        button_add_cli              (std::string_view args);
    void                        button_remove_cli           (std::string_view args);

    std::vector<Button>         buttons;
    bool                        loaded_from_nvs             {false};
};
