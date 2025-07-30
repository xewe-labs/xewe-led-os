#ifndef MODULE_H
#define MODULE_H

#include <cstddef>      // for std::size_t
#include <functional>   // for std::function
#include <string>       // for std::string
#include <utility>      // for std::move

class SystemController;

class ModuleConfig {
public:
    ModuleConfig() {}

    virtual ~ModuleConfig() noexcept = default;

    ModuleConfig(const ModuleConfig&)            = default;
    ModuleConfig& operator=(const ModuleConfig&) = default;
    ModuleConfig(ModuleConfig&&) noexcept         = default;
    ModuleConfig& operator=(ModuleConfig&&) noexcept = default;
};

using command_function_t = std::function<void(const char* args)>;

struct Command {
    std::string         name;
    std::string         description;
    std::string         sample_usage;
    std::size_t         arg_count;
    command_function_t  function;
};

struct CommandGroup {
    std::string    name;
    const Command* commands;
    std::size_t    command_count;
};

/// Generic Module base: parameterized only by your runtime‐context type.
template<typename ContextT>
class Module {
public:
    Module(SystemController&  controller_ref,
           std::string        module_name_param,
           std::string        nvs_key_param,
           bool               can_be_disabled)
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

    // **begin takes ModuleConfig, loop takes only ContextT**
    virtual void begin(const ModuleConfig& cfg) = 0;
    virtual void loop(ContextT ctx)             = 0;

    virtual void enable()                = 0;
    virtual void disable()               = 0;
    virtual void reset()                 = 0;
    virtual const char* status() const   = 0;

protected:
    SystemController&   controller;
    std::string         module_name;
    std::string         nvs_key;
    bool                enabled;
    bool                can_be_disabled;

    const Command*      commands       = nullptr;
    std::size_t         cmd_count      = 0;
    CommandGroup        commands_group{};
};

// Specialization for no‐context modules: begin()/loop() take no args.
template<>
class Module<void> {
public:
    Module(SystemController&  controller_ref,
           std::string        module_name_param,
           std::string        nvs_key_param,
           bool               can_be_disabled)
      : controller(controller_ref)
      , module_name(std::move(module_name_param))
      , nvs_key(std::move(nvs_key_param))
      , enabled(true)
      , can_be_disabled(can_be_disabled)
    {}

    virtual ~Module() noexcept = default;

    Module(const Module&)            = delete;
    Module& operator=(const Module&) = delete;
    Module(Module&&)                 = delete;
    Module& operator=(Module&&)      = delete;

    virtual void begin()              = 0;
    virtual void loop()               = 0;
    virtual void enable()             = 0;
    virtual void disable()            = 0;
    virtual void reset()              = 0;
    virtual const char* status() const= 0;

protected:
    SystemController&   controller;
    std::string         module_name;
    std::string         nvs_key;
    bool                enabled;
    bool                can_be_disabled;

    const Command*      commands       = nullptr;
    std::size_t         cmd_count      = 0;
    CommandGroup        commands_group{};
};

#endif // MODULE_H
