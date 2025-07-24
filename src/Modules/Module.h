// src/Modules/Module.h
#ifndef MODULE_H
#define MODULE_H

#include <cstdint>
#include <type_traits>
class SystemController;
#include "Software/CommandParser/CommandParser.h"

template<typename ContextT = void>
class Module {
public:
    using Command      = CommandParser::Command;
    using CommandGroup = CommandParser::CommandGroup;

    /**
     * @tparam N
     *   Deduce the number of commands in your static Command array.
     *   We `static_assert(N>0)` to enforce at least 1 command at compile time.
     */
    template<size_t N>
    Module(SystemController& controller_ref,
           const char*       module_name_param,
           const Command     (&cmds)[N],
           bool              initially_enabled,
           bool              can_be_disabled,
           const char*       nvs_key_param)
      : controller(controller_ref)
      , module_name(module_name_param)
      , enabled(initially_enabled)
      , can_be_disabled(can_be_disabled)
      , nvs_key(nvs_key_param)
      , commands(cmds)
      , cmd_count(N)
      , commands_group{ module_name_param, commands, cmd_count }
    {
        static_assert(N > 0, "Every Module must define at least one Command");
    }

    virtual ~Module() noexcept = default;

    virtual void begin(ContextT* context = nullptr) = 0;
    virtual void loop(ContextT* context = nullptr)  = 0;
    virtual void enable()                          = 0;
    virtual void disable()                         = 0;
    virtual void reset()                           = 0;
    virtual const char* status()                   = 0;

protected:
    SystemController&   controller;
    const char*         module_name;
    const char*         nvs_key;
    bool                enabled;
    bool                can_be_disabled;

    // owned by base now:
    const Command*      commands;
    size_t              cmd_count;
    CommandGroup        commands_group;
};

#endif // MODULE_H
