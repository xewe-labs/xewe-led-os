// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Module/SyncModule.h
#pragma once

#include "Module.h"


class SyncModule : public Module {
public:
    SyncModule(ModuleController& controller,
        std::string              id,
        std::string              name,
        std::string              description,
        bool                     requires_init_setup,
        bool                     can_be_disabled,
        bool                     has_cli_commands)
        : Module(controller,
              id,
              name,
              description,
              requires_init_setup,
              can_be_disabled,
              has_cli_commands) {}

    // these functions should result in an update of the internal SyncModule state
    virtual void sync_color      (std::array<uint8_t, 3> color) = 0;
    virtual void sync_brightness (uint8_t brightness)           = 0;
    virtual void sync_state      (uint8_t state)                = 0;
    virtual void sync_mode       (uint8_t mode)                 = 0;
    virtual void sync_length     (uint16_t length)              = 0;
    virtual void sync_all        (std::array<uint8_t, 3> color,
                                  uint8_t                brightness,
                                  uint8_t                state,
                                  uint8_t                mode,
                                  uint16_t               length);

    // sync mode param. not all syncable modules support mode params
    virtual void sync_param      (std::string key,
                                  uint8_t     value);
};
