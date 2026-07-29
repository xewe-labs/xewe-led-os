// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Module/Module.h

#pragma once

#include <Arduino.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../../Debug.h"
#include "../../../Config.h"
#include "../../Utils/XeWeString.h"
#include "../../Utils/XeWeValidator.h"


class ModuleController;

class ModuleConfig {
public:
    ModuleConfig                                            ()                              = default;
    virtual ~ModuleConfig                                   () noexcept                     = default;
    ModuleConfig                                            (const ModuleConfig&)           = default;
    ModuleConfig& operator=                                 (const ModuleConfig&)           = default;
    ModuleConfig                                            (ModuleConfig&&) noexcept       = default;
    ModuleConfig& operator=                                 (ModuleConfig&&) noexcept       = default;
};

using command_function_t = std::function<void(std::span<const std::string> args)>;

struct Command {
    std::string                 name;
    std::string                 description;
    std::string                 sample_usage;
    size_t                      arg_count;
    command_function_t          function;
};

class Module {
public:
    Module(ModuleController&    controller,
           std::string          id,
           std::string          name,
           std::string          description,
           bool                 requires_init_setup,
           bool                 can_be_disabled,
           bool                 has_cli_commands)
      : controller              (controller)
      , id                      (std::move(id))
      , name                    (std::move(name))
      , description             (std::move(description))
      , requires_init_setup     (requires_init_setup)
      , can_be_disabled         (can_be_disabled)
      , has_cli_commands        (has_cli_commands)
      , enabled                 (true)
    {
        if (has_cli_commands)   register_generic_commands();
    }

    virtual ~Module                                         () noexcept                     = default;

    Module                                                  (const Module&)                 = delete;
    Module& operator=                                       (const Module&)                 = delete;
    Module                                                  (Module&&)                      = delete;
    Module& operator=                                       (Module&&)                      = delete;

    // begin logic
    void                        begin                       (const ModuleConfig& cfg);
    virtual void                begin_routines_required     (const ModuleConfig& cfg);
    virtual void                begin_routines_init         (const ModuleConfig& cfg);
    virtual void                begin_routines_regular      (const ModuleConfig& cfg);
    virtual void                begin_routines_common       (const ModuleConfig& cfg);

    void                        add_requirement             (Module& other);

    // loop and flow logic
    virtual void                loop                        ();

    virtual void                enable                      (const bool verbose=false,
                                                             const bool do_restart=true);
    virtual void                disable                     (const bool verbose=false,
                                                             const bool do_restart=true);
    virtual void                reset                       (const bool verbose=false,
                                                             const bool do_restart=true,
                                                             const bool keep_enabled=true);

    // info
    virtual std::string         status                      (const bool verbose=false)      const;
    bool                        is_enabled                  (const bool verbose=false)      const;
    bool                        is_disabled                 (const bool verbose=false)      const;
    bool                        init_setup_complete         (const bool verbose=false)      const;
    bool                        has_cli_cmds                () const { return has_cli_commands; }

    // getters
    std::string_view            get_id                      () const { return id; }
    std::string_view            get_name                    () const { return name; }
    std::span<const Command>    get_commands                () const;

protected:
    ModuleController&           controller;
    std::string                 id;
    std::string                 name;
    std::string                 description;

    bool                        can_be_disabled;
    bool                        requires_init_setup;
    bool                        has_cli_commands;
    bool                        enabled;

    std::vector<Command>        commands_storage;

    bool                        requirements_enabled        (const bool verbose=false)      const;
    void                        register_generic_commands   ();

    void                        run_with_dots               (const std::function<void()>& work,
                                                             uint32_t duration_ms=1000,
                                                             uint32_t dot_interval_ms=200);

private:
    std::vector<Module*>        required_modules;
    std::vector<Module*>        dependent_modules;
};
