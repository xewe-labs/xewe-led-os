// CommandParser.cpp
#include "CommandParser.h"

CommandParser::CommandParser(SystemController& controller_ref)
  : Module(controller_ref,
           /* module_name       */ "CommandParser",
           /* initially_enabled */ true,
           /* can_be_disabled   */ true,
           /* nvs_key_param     */ "cmd_parser")
{ }

void CommandParser::begin(ParserConfig* config) {
    groups      = config->groups;
    group_count = config->group_count;
}

void CommandParser::loop(ParserConfig* config) {
    if (!enabled) return;
    if (!config || !config->input) return;
    parse(*config->input);
}

void CommandParser::enable() {
    if (can_be_disabled) enabled = true;
}

void CommandParser::disable() {
    if (can_be_disabled) enabled = false;
}

void CommandParser::reset() {
    // No extra state to reset
}

const char* CommandParser::status() {
    return enabled ? "enabled" : "disabled";
}

void CommandParser::print_help(const String& group_name) const {
    for (size_t i = 0; i < group_count; ++i) {
        const CommandGroup& grp = groups[i];
        if (group_name.equalsIgnoreCase(grp.name)) {
            Serial.println("----------------------------------------");
            Serial.print(grp.name);
            Serial.println(" commands:");
            for (size_t j = 0; j < grp.command_count; ++j) {
                const Command& cmd = grp.commands[j];
                Serial.print("  $");
                Serial.print(grp.name);
                Serial.print(" ");
                Serial.print(cmd.name);
                int padding = 20 - (strlen(grp.name) + strlen(cmd.name));
                for (int k = 0; k < padding; ++k) Serial.print(" ");
                Serial.print("- ");
                Serial.print(cmd.description);
                Serial.print(" (args: ");
                Serial.print(cmd.arg_count);
                Serial.println(")");
            }
            Serial.println("----------------------------------------");
            return;
        }
    }
    Serial.print("Error: Command group '");
    Serial.print(group_name);
    Serial.println("' not found.");
}

void CommandParser::print_all_commands() const {
    Serial.println("\n===== All Available Commands =====");
    for (size_t i = 0; i < group_count; ++i) {
        if (strlen(groups[i].name) > 0) {
            print_help(groups[i].name);
        }
    }
    Serial.println("==================================");
}

void CommandParser::parse(const String& input) const {
    String line = input;
    line.trim();

    if (!line.startsWith("$")) {
        Serial.println("Error: commands must start with '$'");
        return;
    }

    line = line.substring(1);
    line.trim();

    int sp1 = line.indexOf(' ');
    String group_name;
    String rest;
    if (sp1 < 0) {
        group_name = line;
        rest       = "";
    } else {
        group_name = line.substring(0, sp1);
        rest       = line.substring(sp1 + 1);
        rest.trim();
    }

    struct Token { String value; bool was_quoted; };
    std::vector<Token> all_tokens;

    if (rest.length() > 0) {
        int pos = 0;
        while (pos < rest.length()) {
            while (pos < rest.length() && isspace(rest.charAt(pos))) pos++;
            if (pos >= rest.length()) break;

            String token_value;
            bool is_quoted = false;

            if (rest.charAt(pos) == '"') {
                is_quoted = true;
                int end_quote = rest.indexOf('"', pos + 1);
                if (end_quote == -1) {
                    Serial.println("Error: Unterminated quote in command.");
                    return;
                }
                token_value = rest.substring(pos + 1, end_quote);
                pos = end_quote + 1;
            } else {
                int next_space = rest.indexOf(' ', pos);
                if (next_space == -1) {
                    token_value = rest.substring(pos);
                    pos = rest.length();
                } else {
                    token_value = rest.substring(pos, next_space);
                    pos = next_space;
                }
            }
            all_tokens.push_back({token_value, is_quoted});
        }
    }

    String command_name;
    std::vector<Token> arg_tokens;
    if (!all_tokens.empty()) {
        command_name  = all_tokens[0].value;
        arg_tokens.assign(all_tokens.begin() + 1, all_tokens.end());
    }

    for (size_t i = 0; i < group_count; ++i) {
        const CommandGroup& grp = groups[i];
        if (group_name.equalsIgnoreCase(grp.name)) {
            if (command_name.length() == 0) {
                if (grp.command_count > 0) {
                    grp.commands[0].function("");
                } else {
                    Serial.println("Error: no commands in group.");
                }
                return;
            }

            for (size_t j = 0; j < grp.command_count; ++j) {
                const Command& cmd = grp.commands[j];
                if (command_name.equalsIgnoreCase(cmd.name)) {
                    if (cmd.arg_count != arg_tokens.size()) {
                        Serial.print("Error: '");
                        Serial.print(cmd.name);
                        Serial.print("' expects ");
                        Serial.print(cmd.arg_count);
                        Serial.print(" arguments, but got ");
                        Serial.print(arg_tokens.size());
                        Serial.println(".");
                        return;
                    }

                    String arg_string_for_handler;
                    for (size_t k = 0; k < arg_tokens.size(); ++k) {
                        const Token& token = arg_tokens[k];
                        if (token.was_quoted) {
                            if (token.value.indexOf(' ') != -1) {
                                arg_string_for_handler += '"';
                                arg_string_for_handler += token.value;
                                arg_string_for_handler += '"';
                            } else {
                                arg_string_for_handler += token.value;
                            }
                        } else {
                            arg_string_for_handler += token.value;
                        }
                        if (k < arg_tokens.size() - 1) {
                            arg_string_for_handler += ' ';
                        }
                    }

                    cmd.function(arg_string_for_handler);
                    return;
                }
            }
            Serial.print("Error: Unknown command '");
            Serial.print(command_name);
            Serial.print("' in group '");
            Serial.print(group_name);
            Serial.println("'.");
            return;
        }
    }
    Serial.print("Error: Unknown command group '");
    Serial.print(group_name);
    Serial.println("'.");
}
