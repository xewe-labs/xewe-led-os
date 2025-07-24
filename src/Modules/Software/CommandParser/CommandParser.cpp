#include "CommandParser.h"

void CommandParser::begin(const CommandGroup* groups, size_t group_count) {
    groups_      = groups;
    group_count_ = group_count;
}

void CommandParser::print_help(const String& group_name) const {
    // This function remains unchanged.
    for (size_t i = 0; i < group_count_; ++i) {
        const CommandGroup& grp = groups_[i];
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
                for(int k=0; k < padding; ++k) Serial.print(" ");
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
    // This function remains unchanged.
    Serial.println("\n===== All Available Commands =====");
    for (size_t i = 0; i < group_count_; ++i) {
        if (strlen(groups_[i].name) > 0) {
            print_help(groups_[i].name);
        }
    }
    Serial.println("==================================");
}

void CommandParser::loop(const String& input) const {
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
        rest = "";
    } else {
        group_name = line.substring(0, sp1);
        rest = line.substring(sp1 + 1);
        rest.trim();
    }

    // --- Tokenizer with Quote State Tracking ---
    struct Token {
        String value;
        bool was_quoted;
    };
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
        command_name = all_tokens[0].value;
        // The rest of the tokens are arguments
        arg_tokens.assign(all_tokens.begin() + 1, all_tokens.end());
    }
    // --- End Tokenizer ---

    for (size_t i = 0; i < group_count_; ++i) {
        const CommandGroup& grp = groups_[i];
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

                    // --- Reconstruct argument string based on your new rules ---
                    String arg_string_for_handler;
                    for (size_t k = 0; k < arg_tokens.size(); ++k) {
                        const Token& token = arg_tokens[k];
                        if (token.was_quoted) {
                            if (token.value.indexOf(' ') != -1) {
                                // Case 2: Quoted with spaces -> KEEP quotes
                                arg_string_for_handler += '"';
                                arg_string_for_handler += token.value;
                                arg_string_for_handler += '"';
                            } else {
                                // Case 1: Quoted with no spaces -> STRIP quotes
                                arg_string_for_handler += token.value;
                            }
                        } else {
                            // Not quoted originally, use value directly
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