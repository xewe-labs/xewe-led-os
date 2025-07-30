// CommandParser.cpp

#include "CommandParser.h"
#include <Arduino.h>
#include <cctype>
#include <cstring>
#include <string>

CommandParser::CommandParser(SystemController& controller)
  : Module(controller,
           /* module name */ "command_parser",
           /* NVS key     */ "cmd_parser",
           /* can disable */ true)
{}

void CommandParser::begin(const ModuleConfig& cfg) {
    // Cast the generic ModuleConfig to our ParserConfig
    const auto& config = static_cast<const ParserConfig&>(cfg);
    groups_      = config.groups;
    group_count_ = config.group_count;
}

void CommandParser::loop(const std::string& args) {
    if (!enabled) return;
    if (args.empty()) return;

    // Convert std::string to Arduino String for parsing convenience
    String line(args.c_str());
    parse(line);
}

void CommandParser::enable() {
    if (can_be_disabled) enabled = true;
}

void CommandParser::disable() {
    if (can_be_disabled) enabled = false;
}

void CommandParser::reset() {
    // No persistent state to clear
}

std::string_view CommandParser::status() const {
    return enabled ? "enabled" : "disabled";
}

void CommandParser::print_help(const std::string& group_name) const {
    // Use Arduino String for case‐insensitive comparison
    String target(group_name.c_str());
    for (size_t i = 0; i < group_count_; ++i) {
        const auto& grp = groups_[i];
        if (target.equalsIgnoreCase(grp.name.c_str())) {
            Serial.println("----------------------------------------");
            Serial.print(grp.name.c_str());
            Serial.println(" commands:");
            for (size_t j = 0; j < grp.commands.size(); ++j) {
                const auto& cmd = grp.commands[j];
                Serial.print("  $");
                Serial.print(grp.name.c_str());
                Serial.print(" ");
                Serial.print(cmd.name.c_str());
                int padding = 20 - int(grp.name.size() + cmd.name.size());
                for (int k = 0; k < padding; ++k) Serial.print(" ");
                Serial.print("- ");
                Serial.print(cmd.description.c_str());
                Serial.print(" (args: ");
                Serial.print(cmd.arg_count);
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
    for (size_t i = 0; i < group_count_; ++i) {
        const auto& grp = groups_[i];
        if (!grp.name.empty()) {
            print_help(grp.name);
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

    // Strip leading '$' and split off group name
    line = line.substring(1);
    line.trim();
    int sp1 = line.indexOf(' ');
    String groupName = (sp1 < 0 ? line : line.substring(0, sp1));

    // Extract rest of line and trim it
    String rest;
    if (sp1 < 0) {
        rest = String();
    } else {
        rest = line.substring(sp1 + 1);
    }
    rest.trim();

    struct Token { String value; bool wasQuoted; };
    std::vector<Token> tokens;

    // Tokenize, respecting quoted strings
    int pos = 0;
    while (pos < rest.length()) {
        while (pos < rest.length() && isspace(rest.charAt(pos))) pos++;
        if (pos >= rest.length()) break;

        bool quoted = false;
        String val;
        if (rest.charAt(pos) == '"') {
            quoted = true;
            int endQ = rest.indexOf('"', pos + 1);
            if (endQ < 0) {
                Serial.println("Error: Unterminated quote in command.");
                return;
            }
            val = rest.substring(pos + 1, endQ);
            pos = endQ + 1;
        } else {
            int nxt = rest.indexOf(' ', pos);
            if (nxt < 0) {
                val = rest.substring(pos);
                pos = rest.length();
            } else {
                val = rest.substring(pos, nxt);
                pos = nxt;
            }
        }
        tokens.push_back({ val, quoted });
    }

    // First token is the command name, rest are args
    String cmdName;
    std::vector<Token> args;
    if (!tokens.empty()) {
        cmdName = tokens[0].value;
        args.assign(tokens.begin() + 1, tokens.end());
    }

    // Find the matching group
    for (size_t i = 0; i < group_count_; ++i) {
        const auto& group = groups_[i];
        if (groupName.equalsIgnoreCase(group.name.c_str())) {
            // No subcommand => run the first command in group
            if (cmdName.isEmpty()) {
                if (!group.commands.empty()) {
                    group.commands[0].function("");
                } else {
                    Serial.println("Error: no commands in group.");
                }
                return;
            }
            // Lookup the command
            for (size_t j = 0; j < group.commands.size(); ++j) {
                const auto& cmd = group.commands[j];
                if (cmdName.equalsIgnoreCase(cmd.name.c_str())) {
                    if (cmd.arg_count != args.size()) {
                        Serial.printf(
                            "Error: '%s' expects %u args, but got %u\n",
                            cmd.name.c_str(),
                            unsigned(cmd.arg_count),
                            unsigned(args.size())
                        );
                        return;
                    }
                    // Rebuild args into a single string
                    String rebuilt;
                    for (size_t k = 0; k < args.size(); ++k) {
                        const auto& t = args[k];
                        if (t.wasQuoted && t.value.indexOf(' ') >= 0) {
                            rebuilt += '"' + t.value + '"';
                        } else {
                            rebuilt += t.value;
                        }
                        if (k + 1 < args.size()) rebuilt += ' ';
                    }
                    // Convert to std::string for the std::function
                    std::string argStr = rebuilt.c_str();
                    cmd.function(argStr);
                    return;
                }
            }
            Serial.printf("Error: Unknown command '%s' in group '%s'\n",
                          cmdName.c_str(), groupName.c_str());
            return;
        }
    }

    Serial.printf("Error: Unknown command group '%s'\n",
                  groupName.c_str());
}
