// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Hardware/Buttons/Buttons.h
#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <tuple>
#include <vector>
#include <algorithm>
#include <limits>
#include <utility>

#include "../../Core/Nvs/FlexData.h"
#include "../../Module/Module.h"


struct ButtonsConfig : public ModuleConfig {};


enum class ButtonInputMode : uint8_t {
    PULL_UP   = 0,
    PULL_DOWN = 1
};


enum class ButtonTriggerEvent : uint8_t {
    ON_PRESS   = 0,
    ON_RELEASE = 1,
    ON_CHANGE  = 2
};


struct ButtonData : FlexData<ButtonData> {
    uint32_t    id                   = 0;
    uint8_t     pin                  = 0;
    std::string command;
    uint32_t    debounce_interval    = 50;
    uint8_t     type                 = static_cast<uint8_t>(ButtonInputMode::PULL_UP);
    uint8_t     event                = static_cast<uint8_t>(ButtonTriggerEvent::ON_PRESS);

    // Runtime-only fields. Not persisted.
    uint32_t    last_debounce_time   = 0;
    int         last_steady_state    = 0;
    int         last_flicker_state   = 0;

    static constexpr auto fields() {
        return std::make_tuple(
            fld("id",                &ButtonData::id),
            fld("pin",               &ButtonData::pin),
            fld("command",           &ButtonData::command),
            fld("debounce_interval", &ButtonData::debounce_interval),
            fld("type",              &ButtonData::type),
            fld("event",             &ButtonData::event)
        );
    }
};


struct ButtonsData : FlexData<ButtonsData> {
    std::vector<ButtonData> buttons;

    static constexpr auto fields() {
        return std::make_tuple(
            fld("buttons", &ButtonsData::buttons)
        );
    }
};


class Buttons : public Module {
public:
    explicit                    Buttons                     (ModuleController& controller);

    void                        begin_routines_regular      (const ModuleConfig& cfg) override;
    void                        loop                        () override;

    void                        reset                       (const bool verbose=false,
                                                             const bool do_restart=true,
                                                             const bool keep_enabled=true) override;

    std::string                 status                      (const bool verbose=false) const override;

    void                        add                         (uint8_t pin,
                                                             std::string command,
                                                             ButtonInputMode type,
                                                             ButtonTriggerEvent event,
                                                             uint32_t debounce_interval);

    void                        remove                      (uint32_t button_id);

    void                        load_from_nvs               ();
    void                        save_to_nvs                 ();

private:
    void                        button_add_cli              (std::span<const std::string> args);
    void                        button_remove_cli           (std::span<const std::string> args);

    ButtonsData                 data;
};