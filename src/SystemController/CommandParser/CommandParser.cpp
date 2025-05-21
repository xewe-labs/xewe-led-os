// CommandParser.cc
#include "CommandParser.h"
#include <Arduino.h>  // for Serial

void CommandParser::set_groups(const CommandGroup* groups, size_t group_count) {
    groups_      = groups;
    group_count_ = group_count;
}

void CommandParser::parse_and_execute(const String& input) const {
    String line = input;
    line.trim();

    // 1) Require leading '$'
    if (!line.startsWith("$")) {
        Serial.println("Error: commands must start with '$'");
        return;
    }

    // 2) Strip off the '$'
    line = line.substring(1);
    line.trim();

    // 3) Split off <group> and the rest (if any)
    int sp1 = line.indexOf(' ');
    String group_name;
    String rest;
    if (sp1 < 0) {
        // only group provided
        group_name = line;
        rest       = String();
    } else {
        group_name = line.substring(0, sp1);
        rest       = line.substring(sp1 + 1);
        rest.trim();
    }

    // 4) From rest, split <command> and [args...]
    String command_name;
    String arguments;
    if (rest.length() == 0) {
        // no command specified
        command_name = String();
        arguments    = String();
    } else {
        int sp2 = rest.indexOf(' ');
        if (sp2 < 0) {
            command_name = rest;
            arguments    = String();
        } else {
            command_name = rest.substring(0, sp2);
            arguments    = rest.substring(sp2 + 1);
            arguments.trim();
        }
    }

    // 5) Look up the group
    for (size_t i = 0; i < group_count_; ++i) {
        const CommandGroup& grp = groups_[i];
        if (group_name.equalsIgnoreCase(grp.name)) {
            // special: no command given → run first command in group
            if (command_name.length() == 0) {
                if (grp.command_count > 0) {
                    grp.commands[0].function(String());
                } else {
                    Serial.println("Error: no commands in group.");
                }
                return;
            }

            // 6) Otherwise, look up the named command
            for (size_t j = 0; j < grp.command_count; ++j) {
                const Command& cmd = grp.commands[j];
                if (command_name.equalsIgnoreCase(cmd.name)) {
                    // 7) Optional: require args if arg_count>0
                    if (cmd.arg_count > 0 && arguments.length() == 0) {
                        Serial.print("Error: '");
                        Serial.print(cmd.name);
                        Serial.println("' requires arguments.");
                        return;
                    }
                    // 8) Dispatch!
                    cmd.function(arguments);
                    return;
                }
            }

            Serial.println("Unknown command in group.");
            return;
        }
    }

    Serial.println("Unknown command group.");
}
