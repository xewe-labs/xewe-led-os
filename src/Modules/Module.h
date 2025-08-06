// src/Modules/Module.h
#pragma once

#include <cstddef>        // for std::size_t
#include <functional>     // for std::function
#include <span>           // for std::span
#include <string>         // for std::string
#include <string_view>    // for std::string_view
#include <utility>        // for std::move
#include <vector>         // for std::vector

class SystemController;

/// Lightweight configuration holder for modules.
class ModuleConfig {
public:
    ModuleConfig() = default;
    virtual ~ModuleConfig() noexcept = default;
    ModuleConfig(const ModuleConfig&) = default;
    ModuleConfig& operator=(const ModuleConfig&) = default;
    ModuleConfig(ModuleConfig&&) noexcept = default;
    ModuleConfig& operator=(ModuleConfig&&) noexcept = default;
};

using command_function_t = std::function<void(std::string_view args)>;

struct Command {
    std::string         name;
    std::string         description;
    std::string         sample_usage;
    std::size_t         arg_count;
    command_function_t  function;
};

struct CommandsGroup {
    std::string              name;
    std::span<const Command> commands;
};

class Module {
public:
    Module(SystemController& controller,
           std::string       module_name,
           std::string       nvs_key,
           bool              can_be_disabled,
           bool              has_cli_commands)
      : controller(controller)
      , module_name(std::move(module_name))
      , nvs_key(std::move(nvs_key))
      , enabled(true)
      , can_be_disabled(can_be_disabled)
      , has_cli_commands(has_cli_commands)
    {
        if (has_cli_commands)
            register_generic_commands();
    }

    virtual ~Module() noexcept = default;

    Module(const Module&)            = delete;
    Module& operator=(const Module&) = delete;
    Module(Module&&)                 = delete;
    Module& operator=(Module&&)      = delete;

    // required implementation
    virtual void                begin               (const ModuleConfig& cfg)       = 0;
    virtual void                loop                ()                              = 0;
    virtual void                reset               (bool verbose=true)             = 0;

    //optional implementation
    virtual bool                enable              (bool verbose=true);
    virtual bool                disable             (bool verbose=true);
    virtual std::string_view    status              (bool verbose=true)             const;
    virtual bool                is_enabled          (bool verbose=true)             const;
    virtual bool                is_disabled         (bool verbose=true)             const;

    /// Returns up-to-date group with all commands added so far.
    CommandsGroup get_commands_group();

protected:
    SystemController&      controller;
    std::string            module_name;
    std::string            nvs_key;
    bool                   enabled;
    bool                   can_be_disabled;
    bool                   has_cli_commands;

    std::vector<Command>   commands_storage;
    CommandsGroup          commands_group;

    void register_generic_commands();
};
