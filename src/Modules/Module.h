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

/// Generic Module base: now owns its own command storage.
class Module {
public:
    Module(SystemController& controller_ref,
           std::string       module_name_param,
           std::string       nvs_key_param,
           bool              can_be_disabled,
           bool              has_cli_cmds)
      : controller(controller_ref)
      , module_name(std::move(module_name_param))
      , nvs_key(std::move(nvs_key_param))
      , enabled(true)
      , can_be_disabled(can_be_disabled)
      , has_cli_cmds(has_cli_cmds)
    {
        if (has_cli_cmds)
            add_generic_commands();
    }

    virtual ~Module() noexcept = default;

    Module(const Module&)            = delete;
    Module& operator=(const Module&) = delete;
    Module(Module&&)                 = delete;
    Module& operator=(Module&&)      = delete;

    virtual void begin(const ModuleConfig& cfg) = 0;
    virtual void loop()                         = 0;
    virtual void enable()                       = 0;
    virtual void disable()                      = 0;
    virtual void reset()                        = 0;
    virtual std::string_view status() const     = 0;

    /// Returns up-to-date group with all commands added so far.
    CommandsGroup get_commands_group() {
        commands_group.name     = module_name;
        commands_group.commands = std::span<const Command>(
            commands_storage.data(),
            commands_storage.size()
        );
        return commands_group;
    }

protected:
    SystemController&      controller;
    std::string            module_name;
    std::string            nvs_key;
    bool                   enabled;
    bool                   can_be_disabled;
    bool                   has_cli_cmds;

    std::vector<Command>   commands_storage;
    CommandsGroup          commands_group;

    void add_generic_commands();
};
