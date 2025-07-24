#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include "../../Module.h"             // updated Module template
#include <Arduino.h>
#include <functional>
#include <vector>
#include "../../../Debug.h"            // original Debug include path

/**
 * @brief Configuration passed into both begin() and loop().
 *
 *  - .groups/.group_count must be set before calling begin()
 *  - .input           must be set before calling loop()
 */
struct ParserConfig {
    const class CommandParser::CommandGroup* groups;
    size_t                                   group_count;
    const String*                            input;
};

class CommandParser : public Module<ParserConfig> {
public:
    using command_function_t = std::function<void(const String& args)>;

    struct Command {
        const char*         name;
        const char*         description;
        size_t              arg_count;
        command_function_t  function;
    };

    struct CommandGroup {
        const char*    name;
        const Command* commands;
        size_t         command_count;
    };

    /**
     * @brief Construct the parser module.
     * @param controller_ref  Reference to the shared SystemController
     */
    explicit CommandParser(SystemController& controller_ref);

    // ─── Module<ParserConfig> interface ────────────────────────────────────────
    void begin(ParserConfig* config) override;
    void loop(ParserConfig* config) override;
    void enable() override;
    void disable() override;
    void reset() override;
    const char* status() override;
    // ────────────────────────────────────────────────────────────────────────────

    // Unchanged helpers from your original API:
    void print_help(const String& group_name) const;
    void print_all_commands() const;

private:
    /// Core parsing routine (formerly loop(input))
    void parse(const String& input) const;

    const CommandGroup* groups_       = nullptr;
    size_t               group_count_ = 0;
};

#endif // COMMAND_PARSER_H
