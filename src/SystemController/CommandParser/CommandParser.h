// CommandParser.h
#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <Arduino.h>
#include "../../Debug.h"

class CommandParser {
public:
    using command_function_t = std::function<void(const String& args)>;

    struct Command {
        const char*          name;
        const char*          description;
        size_t               arg_count;
        command_function_t   function;
    };

    struct CommandGroup {
        const char*          name;
        const Command*       commands;
        size_t               command_count;
    };

    CommandParser() = default;

    // Provide your groups array and its size
    void begin(const CommandGroup* groups, size_t group_count);

    // Parse lines of the form "$<group> <command> [args...]"
    void loop(const String& input) const;

private:
    const CommandGroup* groups_       = nullptr;
    size_t               group_count_ = 0;
};

#endif // COMMAND_PARSER_H
