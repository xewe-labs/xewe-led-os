/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/

// src/Modules/Software/CommandParser/CommandParser.cpp

#include "CommandParser.h"
#include "../../../SystemController/SystemController.h"

CommandParser::CommandParser(SystemController& controller)
      : Module(controller,
               /* module_name         */ "Command_Parser",
               /* module_description  */ "Allows to parse text from the serial port in the action function calls with parameters",
               /* nvs_key             */ "cmd",
               /* requires_init_setup */ false,
               /* can_be_disabled     */ false,
               /* has_cli_cmds        */ false)
{}

void CommandParser::begin_routines_required(const ModuleConfig& cfg) {
    command_groups.clear();

    auto& modules = controller.get_modules(); // IMPORTANT: reference, not copy

    if (modules.empty())
        return;

    for (Module* module : modules) {
        if (!module)
            continue;

        auto grp = module->get_commands_group();
        if (!grp.commands.empty())
            command_groups.push_back(grp);
    }
}


void CommandParser::print_help(const string& group_name) const {
    string target = group_name;
    transform(target.begin(), target.end(), target.begin(), ::tolower);

    for (const auto& grp : command_groups) {
        string g_name = grp.name;
        string g_code = grp.group;
        transform(g_name.begin(), g_name.end(), g_name.begin(), ::tolower);
        transform(g_code.begin(), g_code.end(), g_code.begin(), ::tolower);

        if (target == g_code || target == g_name) {
            vector<vector<string_view>> table_data;
            table_data.push_back({"Name", "Description", "Sample Usage"});

            vector<string> arg_counts_store;
            arg_counts_store.reserve(grp.commands.size());

            for (const auto& cmd : grp.commands) {
                arg_counts_store.push_back(to_string(cmd.arg_count));

                table_data.push_back({
                    cmd.name,
                    cmd.description,
                    cmd.sample_usage
                });
            }

            controller.serial_port.print_table(
                table_data,
                grp.name + " Commands"
            );
            
            return;
        }
    }

    Serial.print("Error: Command group '");
    Serial.print(group_name.c_str());
    Serial.println("' not found.");
}

void CommandParser::print_all_commands() const {
    // We iterate manually to add spacing between tables
    for (size_t i = 0; i < command_groups.size(); ++i) {
        if (!command_groups[i].name.empty()) {
            print_help(command_groups[i].name);
            Serial.print(""); // Spacer between tables
        }
    }
}

void CommandParser::parse(string_view input_line) const {
    // Copy into mutable string
    string local(input_line.begin(), input_line.end());
    auto is_space = [](char c){ return isspace(static_cast<unsigned char>(c)); };

    // Trim whitespace
    size_t b = local.find_first_not_of(" \t\r\n"),
           e = local.find_last_not_of(" \t\r\n");
    if (b == string::npos) return;
    local = local.substr(b, e - b + 1);

    // Must start with $
    if (local.empty() || local[0] != '$') {
        Serial.println("Error: commands must start with '$'; type $help");
        return;
    }

    // Drop '$' and trim again
    local.erase(0,1);
    b = local.find_first_not_of(" \t\r\n");
    e = local.find_last_not_of(" \t\r\n");
    if (b == string::npos) local.clear();
    else                   local = local.substr(b, e - b + 1);

    // Split off group name
    size_t sp = local.find(' ');
    string group = (sp == string::npos) ? local : local.substr(0, sp);

    // Handle $help specially
    string gl = group;
    transform(gl.begin(), gl.end(), gl.begin(), ::tolower);
    if (gl == "help") {
        print_all_commands();
        return;
    }

    // Extract rest of line
    string rest = (sp == string::npos)
                       ? string()
                       : local.substr(sp+1);
    b = rest.find_first_not_of(" \t\r\n");
    e = rest.find_last_not_of(" \t\r\n");
    if (b == string::npos) rest.clear();
    else                   rest = rest.substr(b, e - b + 1);

    // Tokenize (supports quoted and escaped characters)
    struct Token { string value; bool quoted; };
    vector<Token> toks;
    size_t pos = 0;
    while (pos < rest.size()) {
        while (pos < rest.size() && is_space(rest[pos])) ++pos;
        if (pos >= rest.size()) break;

        bool quoted = false;
        string tok;

        if (rest[pos] == '"') {
            quoted = true;
            size_t start = pos + 1;
            bool escape = false;
            bool closed = false;
            pos++; // skip initial quote

            while (pos < rest.size()) {
                if (escape) {
                    escape = false; // Previous char was '\', so we skip evaluating this one
                } else if (rest[pos] == '\\') {
                    escape = true;  // Enter escape mode for the next char
                } else if (rest[pos] == '"') {
                    closed = true;  // Unescaped quote -> end of string
                    break;
                }
                pos++;
            }

            if (!closed) {
                Serial.println("Error: Unterminated quote in command.");
                return;
            }

            // Extract everything between the quotes, including the literal backslashes
            tok = rest.substr(start, pos - start);
            pos++; // skip closing quote
        } else {
            size_t start = pos;
            while (pos < rest.size() && !is_space(rest[pos])) {
                pos++;
            }
            tok = rest.substr(start, pos - start);
        }
        toks.push_back({tok, quoted});
    }

    // Separate cmd name and arguments
    string cmd;
    vector<Token> args;
    if (!toks.empty()) {
        cmd  = toks[0].value;
        args.assign(toks.begin()+1, toks.end());
    }

    // Lookup group
    for (size_t gi = 0; gi < command_groups.size(); ++gi) {
        const auto& grp = command_groups[gi];
        string name = grp.name;
        transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (gl == name) {
            // If no subcommand provided, show help for this group
            if (cmd.empty()) {
                print_help(grp.name);
                return;
            }
            // Find matching command
            string cl = cmd;
            transform(cl.begin(), cl.end(), cl.begin(), ::tolower);
            for (const auto& c : grp.commands) {
                string cn = c.name;
                transform(cn.begin(), cn.end(), cn.begin(), ::tolower);
                if (cl == cn) {
                    if (c.arg_count != args.size()) {
                        Serial.printf(
                          "Error: '%s' expects %u args, but got %u\n",
                           c.name.c_str(),
                           unsigned(c.arg_count),
                           unsigned(args.size())
                        );
                        return;
                    }
                    // Rebuild args string
                    string rebuilt;
                    for (size_t ai = 0; ai < args.size(); ++ai) {
                        auto& tk = args[ai];
                        // Always wrap back in quotes if it was originally quoted.
                        // This protects nested strings and empty strings like "".
                        if (tk.quoted) {
                            rebuilt += '"'; rebuilt += tk.value; rebuilt += '"';
                        } else {
                            rebuilt += tk.value;
                        }
                        if (ai + 1 < args.size()) rebuilt += ' ';
                    }
                    // pass a string, not a string_view
                    c.function(rebuilt);
                    return;
                }
            }
            Serial.printf("Error: Unknown command '%s'; type $%s to see available commands\n",
                          cmd.c_str(), group.c_str());
            return;
        }
    }

    Serial.printf("Error: Unknown command group '%s'; type $help\n", group.c_str());
}
