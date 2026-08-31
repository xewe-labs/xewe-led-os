// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// <filepath from project root>
#pragma once

#include "../Module/SyncModule.h" // adjust this path if needed


struct ModuleNameConfig : public ModuleConfig {};


class ModuleName : public SyncModule {
public:
    explicit                    ModuleName                  (ModuleController& controller);

    // optional functions, can be overridden; def is Module.cpp
    void                        begin_routines_required     (const ModuleConfig& cfg)       override;
    void                        begin_routines_init         (const ModuleConfig& cfg)       override;
    void                        begin_routines_regular      (const ModuleConfig& cfg)       override;
    void                        begin_routines_common       (const ModuleConfig& cfg)       override;

    void                        loop                        ()                               override;

    void                        enable                      (const bool verbose=false,
                                                             const bool do_restart=true)     override;
    void                        disable                     (const bool verbose=false,
                                                             const bool do_restart=true)     override;
    void                        reset                       (const bool verbose=false,
                                                             const bool do_restart=true,
                                                             const bool keep_enabled=true)   override;

    string                      status                      (const bool verbose=false)       const override;

    // required SyncModule functions
    void                        sync_color                  (std::array<uint8_t, 3> color)  override;
    void                        sync_brightness             (uint8_t brightness)             override;
    void                        sync_state                  (bool state)                     override;
    void                        sync_mode                   (uint8_t mode)                   override;
    void                        sync_length                 (uint16_t length)                override;

    // optional SyncModule function
    void                        sync_param                  (std::string key,
                                                             uint8_t     value)               override;

    // custom functions template
    void                        custom_function             ();

private:

};
