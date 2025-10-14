// src/Modules/Module/Module.h
#pragma once

#include <cstddef>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <set>

#include "../../Config.h"
#include "../../Debug.h"

#include "../../StringUtils.h"

class SystemController;

class ModuleConfig {
public:
    ModuleConfig                                            ()                              = default;
    virtual ~ModuleConfig                                   () noexcept                     = default;
    ModuleConfig                                            (const ModuleConfig&)           = default;
    ModuleConfig& operator=                                 (const ModuleConfig&)           = default;
    ModuleConfig                                            (ModuleConfig&&) noexcept       = default;
    ModuleConfig& operator=                                 (ModuleConfig&&) noexcept       = default;
};

using command_function_t = std::function<void(std::string args)>;

struct Command {
    std::string                 name;
    std::string                 description;
    std::string                 sample_usage;
    std::size_t                 arg_count;
    command_function_t          function;
};

struct CommandsGroup {
    std::string                 name;
    std::string                 group;
    std::span<const Command>    commands;
};

class Module {
public:
    Module(SystemController&    controller,
           std::string          module_name,
           std::string          module_description,
           std::string          nvs_key,
           bool                 requires_init_setup,
           bool                 can_be_disabled,
           bool                 has_cli_commands)
      : controller              (controller)
      , module_name             (std::move(module_name))
      , module_description      (std::move(module_description))
      , nvs_key                 (std::move(nvs_key))
      , requires_init_setup     (requires_init_setup)
      , can_be_disabled         (can_be_disabled)
      , has_cli_commands        (has_cli_commands)
      , enabled                 (true)
    {
        if (has_cli_commands)
            register_generic_commands();
    }

    virtual ~Module                                         () noexcept                     = default;

    Module                                                  (const Module&)                 = delete;
    Module& operator=                                       (const Module&)                 = delete;
    Module                                                  (Module&&)                      = delete;
    Module& operator=                                       (Module&&)                      = delete;

    virtual void                begin                       (const ModuleConfig& cfg);
    virtual void                begin_routines_required     (const ModuleConfig& cfg);
    virtual void                begin_routines_init         (const ModuleConfig& cfg);
    virtual void                begin_routines_regular      (const ModuleConfig& cfg);
    virtual void                begin_routines_common       (const ModuleConfig& cfg);

    virtual void                loop                        ();

    virtual void                reset                       (const bool verbose=false);

    virtual bool                enable                      (const bool verbose=false);
    virtual bool                disable                     (const bool verbose=false);

    virtual std::string         status                      (const bool verbose=false)      const;
    virtual bool                is_enabled                  (const bool verbose=false)      const;
    virtual bool                is_disabled                 (const bool verbose=false)      const;
    virtual bool                init_setup_complete         (const bool verbose=false)      const;


    CommandsGroup               get_commands_group          ();
    std::string_view            get_module_name             ()                              const { return module_name; };

protected:
    SystemController&           controller;
    std::string                 module_name;
    std::string                 module_description;
    std::string                 nvs_key;

    bool                        can_be_disabled;
    bool                        requires_init_setup;
    bool                        has_cli_commands;

    bool                        enabled;

    std::vector<Command>        commands_storage;
    CommandsGroup               commands_group;
    void                        register_generic_commands   ();

    void                        run_with_dots               (const std::function<void()>& work,
                                                             uint32_t duration_ms = 1000,
                                                             uint32_t dot_interval_ms = 200);
};
