// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// <filepath from project root>
#pragma once

#include "../Module/Module.h" // adjust this path if needed


struct ModuleNameConfig : public ModuleConfig {};


class ModuleName : public Module {
public:
    explicit                    ModuleName                  (ModuleController& controller);

    // optional functions, can be overridden; def is Module.cpp
    void                        begin_routines_required     (const ModuleConfig& cfg)       override;
    void                        begin_routines_init         (const ModuleConfig& cfg)       override;
    void                        begin_routines_regular      (const ModuleConfig& cfg)       override;
    void                        begin_routines_common       (const ModuleConfig& cfg)       override;

    void                        loop                        ()                              override;

    void                        enable                      (const bool verbose=false,
                                                             const bool do_restart=true)    override;
    void                        disable                     (const bool verbose=false,
                                                             const bool do_restart=true)    override;
    void                        reset                       (const bool verbose=false,
                                                             const bool do_restart=true,
                                                             const bool keep_enabled=true)    override;

    string                      status                      (const bool verbose=false)      const override;

    // custom functions template
    void                        custom_function             ();

private:

};
