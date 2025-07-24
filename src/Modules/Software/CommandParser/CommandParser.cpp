#include "CommandParser.h"

CommandParser::CommandParser(SystemController& controller_ref)
  : Module(controller_ref,
           /* module_name       */ "command_parser",
           /* initially_enabled */ true,
           /* can_be_disabled   */ true,
           /* nvs_key_param     */ "cmd_parser")
{ }

void CommandParser::begin(ParserConfig* config) {
    groups_      = config->groups;
    group_count_ = config->group_count;
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
    // No persistent state beyond groups_/group_count_
}

const char* CommandParser::status() {
    return enabled ? "enabled" : "disabled";
}

void CommandParser::print_help(const String& group_name) const {
    for (size_t i = 0; i < group_count_; ++i) {
        const auto& grp = groups_[i];
        if (group_name.equalsIgnoreCase(grp.name)) {
            Serial.println("----------------------------------------");
            Serial.print(grp.name);
            Serial.println(" commands:");
            for (size_t j = 0; j < grp.command_count; ++j) {
                const auto& cmd = grp.commands[j];
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
    for (size_t i = 0; i < group_count_; ++i) {
        const auto& grp = groups_[i];
        if (strlen(grp.name) > 0) {
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

    line = line.substring(1);
    line.trim();

    int sp1     = line.indexOf(' ');
    String grp  = (sp1 < 0 ? line : line.substring(0, sp1));
    String rest = (sp1 < 0 ? String() : line.substring(sp1 + 1)).trim();

    struct Token { String value; bool was_quoted; };
    std::vector<Token> tokens;

    int pos = 0;
    while (pos < rest.length()) {
        while (pos < rest.length() && isspace(rest.charAt(pos))) pos++;
        if (pos >= rest.length()) break;

        bool quoted = false;
        String val;
        if (rest.charAt(pos) == '"') {
            quoted = true;
            int endq = rest.indexOf('"', pos + 1);
            if (endq < 0) {
                Serial.println("Error: Unterminated quote in command.");
                return;
            }
            val = rest.substring(pos + 1, endq);
            pos = endq + 1;
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
        tokens.push_back({val, quoted});
    }

    String cmd;
    std::vector<Token> args;
    if (!tokens.empty()) {
        cmd  = tokens[0].value;
        args.assign(tokens.begin() + 1, tokens.end());
    }

    for (size_t i = 0; i < group_count_; ++i) {
        const auto& group = groups_[i];
        if (grp.equalsIgnoreCase(group.name)) {
            if (cmd.isEmpty()) {
                if (group.command_count > 0) {
                    group.commands[0].function("");
                } else {
                    Serial.println("Error: no commands in group.");
                }
                return;
            }
            for (size_t j = 0; j < group.command_count; ++j) {
                const auto& c = group.commands[j];
                if (cmd.equalsIgnoreCase(c.name)) {
                    if (c.arg_count != args.size()) {
                        Serial.printf(
                          "Error: '%s' expects %u args, but got %u\n",
                          c.name,
                          (unsigned)c.arg_count,
                          (unsigned)args.size()
                        );
                        return;
                    }
                    String rebuilt;
                    for (size_t k = 0; k < args.size(); ++k) {
                        const auto& t = args[k];
                        if (t.was_quoted && t.value.indexOf(' ') >= 0) {
                            rebuilt += '"' + t.value + '"';
                        } else {
                            rebuilt += t.value;
                        }
                        if (k + 1 < args.size()) rebuilt += ' ';
                    }
                    c.function(rebuilt);
                    return;
                }
            }
            Serial.printf("Error: Unknown command '%s' in group '%s'\n",
                          cmd.c_str(), grp.c_str());
            return;
        }
    }
    Serial.printf("Error: Unknown command group '%s'\n", grp.c_str());
}
