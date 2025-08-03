// src/Modules/Software/CommandParser/CommandParser.cpp
#include "CommandParser.h"
#include "../../../SystemController.h"


CommandParser::CommandParser(SystemController& controller)
  : Module(controller,
           /* module name */ "command_parser",
           /* NVS key     */ "cmd_parser",
           /* can disable */ true)
{}

void CommandParser::begin(const ModuleConfig& cfg) {
    // Cast the generic ModuleConfig to our ParserConfig
    const auto& config = static_cast<const ParserConfig&>(cfg);
    groups      = config.groups;
    group_count = config.group_count;
}

void CommandParser::loop() {
    if (!enabled) return;
    // No default input source; implement input handling as needed.
}

void CommandParser::enable() {
    if (can_be_disabled) enabled = true;
}

void CommandParser::disable() {
    if (can_be_disabled) enabled = false;
}

void CommandParser::reset() {
    // No state to reset
}

std::string_view CommandParser::status() const {
    return enabled ? "enabled" : "disabled";
}

void CommandParser::print_help(const std::string& group_name) const {
    String target(group_name.c_str());
    for (size_t group_index = 0; group_index < group_count; ++group_index) {
        const auto& group = groups[group_index];
        if (target.equalsIgnoreCase(group.name.c_str())) {
            Serial.println("----------------------------------------");
            Serial.print(group.name.c_str());
            Serial.println(" commands:");
            for (size_t cmd_index = 0; cmd_index < group.commands.size(); ++cmd_index) {
                const auto& command = group.commands[cmd_index];
                Serial.print("  $");
                Serial.print(group.name.c_str());
                Serial.print(" ");
                Serial.print(command.name.c_str());
                int padding = 20 - int(group.name.size() + command.name.size());
                for (int pad_index = 0; pad_index < padding; ++pad_index) {
                    Serial.print(" ");
                }
                Serial.print("- ");
                Serial.print(command.description.c_str());
                Serial.print(" (args: ");
                Serial.print(command.arg_count);
                Serial.println(")");
            }
            Serial.println("----------------------------------------");
            return;
        }
    }
    Serial.print("Error: Command group '");
    Serial.print(group_name.c_str());
    Serial.println("' not found.");
}

void CommandParser::print_all_commands() const {
    Serial.println("\n===== All Available Commands =====");
    for (size_t group_index = 0; group_index < group_count; ++group_index) {
        const auto& group = groups[group_index];
        if (!group.name.empty()) {
            print_help(group.name);
        }
    }
    Serial.println("==================================");
}

void CommandParser::parse(const String& input_line) const {
    String local_line = input_line;
    local_line.trim();

    if (!local_line.startsWith("$")) {
        Serial.println("Error: commands must start with '$'");
        return;
    }

    // Strip leading '$' and split off group name
    local_line.trim();
    local_line = local_line.substring(1);
    local_line.trim();

    int first_space_index = local_line.indexOf(' ');
    String group_name = (first_space_index < 0
                         ? local_line
                         : local_line.substring(0, first_space_index));

    // Extract rest of line and trim it
    String rest_line;
    if (first_space_index < 0) {
        rest_line = String();
    } else {
        rest_line = local_line.substring(first_space_index + 1);
    }
    rest_line.trim();

    struct Token { String value; bool was_quoted; };
    std::vector<Token> tokens;

    int position = 0;
    while (position < rest_line.length()) {
        while (position < rest_line.length() && isspace(rest_line.charAt(position))) {
            ++position;
        }
        if (position >= rest_line.length()) break;

        bool is_quoted = false;
        String token_value;
        if (rest_line.charAt(position) == '"') {
            is_quoted = true;
            int end_quote_index = rest_line.indexOf('"', position + 1);
            if (end_quote_index < 0) {
                Serial.println("Error: Unterminated quote in command.");
                return;
            }
            token_value = rest_line.substring(position + 1, end_quote_index);
            position = end_quote_index + 1;
        } else {
            int next_space_index = rest_line.indexOf(' ', position);
            if (next_space_index < 0) {
                token_value = rest_line.substring(position);
                position = rest_line.length();
            } else {
                token_value = rest_line.substring(position, next_space_index);
                position = next_space_index;
            }
        }
        tokens.push_back({ token_value, is_quoted });
    }

    String cmd_name;
    std::vector<Token> arguments;
    if (!tokens.empty()) {
        cmd_name = tokens[0].value;
        arguments.assign(tokens.begin() + 1, tokens.end());
    }

    // Find the matching group
    for (size_t group_index = 0; group_index < group_count; ++group_index) {
        const auto& group = groups[group_index];
        if (group_name.equalsIgnoreCase(group.name.c_str())) {
            // No subcommand => run the first command in group
            if (cmd_name.isEmpty()) {
                if (!group.commands.empty()) {
                    group.commands[0].function("");
                } else {
                    Serial.println("Error: no commands in group.");
                }
                return;
            }
            // Lookup the command
            for (size_t cmd_index = 0; cmd_index < group.commands.size(); ++cmd_index) {
                const auto& command = group.commands[cmd_index];
                if (cmd_name.equalsIgnoreCase(command.name.c_str())) {
                    if (command.arg_count != arguments.size()) {
                        Serial.printf(
                            "Error: '%s' expects %u args, but got %u\n",
                            command.name.c_str(),
                            unsigned(command.arg_count),
                            unsigned(arguments.size())
                        );
                        return;
                    }
                    // Rebuild args into a single string
                    String rebuilt_args;
                    for (size_t arg_index = 0; arg_index < arguments.size(); ++arg_index) {
                        const auto& token = arguments[arg_index];
                        if (token.was_quoted && token.value.indexOf(' ') >= 0) {
                            rebuilt_args += '"' + token.value + '"';
                        } else {
                            rebuilt_args += token.value;
                        }
                        if (arg_index + 1 < arguments.size()) {
                            rebuilt_args += ' ';
                        }
                    }
                    // Convert to std::string for the std::function
                    std::string arg_str = rebuilt_args.c_str();
                    command.function(arg_str);
                    return;
                }
            }
            Serial.printf("Error: Unknown command '%s' in group '%s'\n",
                          cmd_name.c_str(), group_name.c_str());
            return;
        }
    }

    Serial.printf("Error: Unknown command group '%s'\n",
                  group_name.c_str());
}
