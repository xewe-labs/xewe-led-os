#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <Arduino.h>
#include <functional> // Required for std::function
#include "../../Debug.h" // Assuming Debug.h is in a parent directory

class CommandParser {
public:
    // --- Type Definitions ---
    // Defines the function signature for a command handler.
    // It takes a const reference to a String containing the arguments.
    using command_function_t = std::function<void(const String& args)>;

    // Defines the structure for a single command.
    struct Command {
        const char* name;
        const char* description;
        size_t               arg_count;
        command_function_t   function;
    };

    // Defines the structure for a group of related commands.
    struct CommandGroup {
        const char* name;
        const Command* commands;
        size_t               command_count;
    };

    // --- Public Methods ---
    CommandParser() = default;

    /**
     * @brief Initializes the command parser with an array of command groups.
     * @param groups A pointer to the array of CommandGroup structs.
     * @param group_count The total number of groups in the array.
     */
    void begin(const CommandGroup* groups, size_t group_count);

    /**
     * @brief Parses and executes a command from an input string.
     * Expected format: "$<group> <command> [args...]"
     * @param input The complete command string to parse.
     */
    void loop(const String& input) const;

    /**
     * @brief Prints a formatted help message for a specific command group.
     * @param group_name The name of the group to print help for.
     */
    void print_help(const String& group_name) const;

    /**
     * @brief Prints a formatted help message for all registered command groups.
     */
    void print_all_commands() const;

private:
    // --- Private Members ---
    const CommandGroup* groups_       = nullptr; // Pointer to the command groups array
    size_t               group_count_ = 0;       // Number of command groups
};

#endif // COMMAND_PARSER_H
