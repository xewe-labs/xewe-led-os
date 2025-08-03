#pragma once

#include <cstddef>        // for std::size_t
#include <functional>     // for std::function
#include <span>           // for std::span
#include <string>         // for std::string
#include <string_view>    // for std::string_view
#include <utility>        // for std::move

class SystemController;

/// Lightweight configuration holder for modules.
class ModuleConfig {
public:
    ModuleConfig() = default;
    virtual ~ModuleConfig() noexcept = default;

    ModuleConfig(const ModuleConfig&)            = default;
    ModuleConfig& operator=(const ModuleConfig&) = default;
    ModuleConfig(ModuleConfig&&) noexcept         = default;
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

struct CommandGroup {
    std::string             name;
    std::span<const Command> commands;
};

/// Generic Module base: parameterized by your runtime‐context type.
class Module {
public:
    Module(SystemController& controller_ref,
           std::string       module_name_param,
           std::string       nvs_key_param,
           bool              can_be_disabled)
      : controller(controller_ref)
      , module_name(std::move(module_name_param))
      , nvs_key(std::move(nvs_key_param))
      , enabled(true)
      , can_be_disabled(can_be_disabled)
    {}

    virtual ~Module() noexcept = default;

    // no copy/move
    Module(const Module&)            = delete;
    Module& operator=(const Module&) = delete;
    Module(Module&&)                 = delete;
    Module& operator=(Module&&)      = delete;

    /// Initialize the module with its config.
    virtual void begin(const ModuleConfig& cfg) = 0;

    /// Called repeatedly with the context object.
    virtual void loop(const std::string& args) = 0;

    virtual void enable() = 0;
    virtual void disable() = 0;
    virtual void reset() = 0;

    /// Return a view into a status string owned by the module.
    virtual std::string_view status() const = 0;

    CommandGroup get_commands_group() { return commands_group}
protected:
    SystemController&      controller;
    std::string            module_name;
    std::string            nvs_key;
    bool                   enabled;
    bool                   can_be_disabled;

    std::span<const Command> commands{};
    CommandGroup             commands_group{};
};
