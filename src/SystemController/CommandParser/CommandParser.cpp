// CommandParser.cpp

#include "CommandParser.h"

void CommandParser::begin(const CommandGroup* groups, size_t group_count) {
    groups_      = groups;
    group_count_ = group_count;
}

void CommandParser::parse_and_execute(const String& input) const {
    DBG_PRINTLN(CommandParser, "parse_and_execute: input = " + input);

    String line = input;
    line.trim();
    DBG_PRINTLN(CommandParser, "After trim: line = " + line);

    // 1) Require leading '$'
    if (!line.startsWith("$")) {
        Serial.println("Error: commands must start with '$'");
        DBG_PRINTLN(CommandParser, "Input missing leading '$', aborting parse");
        return;
    }

    // 2) Strip off the '$'
    line = line.substring(1);
    line.trim();
    DBG_PRINTLN(CommandParser, "After stripping '$': line = " + line);

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
    DBG_PRINTLN(CommandParser, "Parsed group_name = " + group_name + ", rest = " + rest);

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
    DBG_PRINTLN(CommandParser, "Parsed command_name = " + command_name + ", arguments = " + arguments);

    // 5) Look up the group
    DBG_PRINTF(CommandParser, "Looking up group among %d groups\n", group_count_);
    for (size_t i = 0; i < group_count_; ++i) {
        const CommandGroup& grp = groups_[i];
        DBG_PRINTLN(CommandParser, "Checking group: " + String(grp.name));
        if (group_name.equalsIgnoreCase(grp.name)) {
            DBG_PRINTLN(CommandParser, "Matched group: " + String(grp.name));

            // special: no command given → run first command in group
            if (command_name.length() == 0) {
                if (grp.command_count > 0) {
                    DBG_PRINTLN(CommandParser, "No command provided; executing first command in group");
                    grp.commands[0].function(String());
                } else {
                    Serial.println("Error: no commands in group.");
                    DBG_PRINTLN(CommandParser, "Group has no commands, aborting");
                }
                return;
            }

            // 6) Otherwise, look up the named command
            for (size_t j = 0; j < grp.command_count; ++j) {
                const Command& cmd = grp.commands[j];
                DBG_PRINTLN(CommandParser, "Checking command: " + String(cmd.name));
                if (command_name.equalsIgnoreCase(cmd.name)) {
                    DBG_PRINTLN(CommandParser, "Matched command: " + String(cmd.name));
                    // 7) Optional: require args if arg_count>0
                    if (cmd.arg_count > 0 && arguments.length() == 0) {
                        Serial.print("Error: '");
                        Serial.print(cmd.name);
                        Serial.println("' requires arguments.");
                        DBG_PRINTLN(CommandParser, "Missing arguments for required command, aborting");
                        return;
                    }
                    // 8) Dispatch!
                    DBG_PRINTLN(CommandParser, "Dispatching command with arguments: " + arguments);
                    cmd.function(arguments);
                    return;
                }
            }

            Serial.println("Unknown command in group.");
            DBG_PRINTLN(CommandParser, "No matching command found in group, aborting");
            return;
        }
    }

    Serial.println("Unknown command group.");
    DBG_PRINTLN(CommandParser, "No matching group found, aborting");
}
