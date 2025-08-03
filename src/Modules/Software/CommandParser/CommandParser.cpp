// src/Modules/Software/CommandParser/CommandParser.cpp

#include "CommandParser.h"
#include "../../../SystemController.h"

CommandParser::CommandParser(SystemController& controller)
  : Module(controller, "cmd_parser", "cmd_parser", true)
{
    // generic: help, status, enable, disable, reset
    add_generic_commands(cmd_commands);

    // no additional subcommands
    commands_group = CommandsGroup{
        "cmd_parser",
        std::span<const Command>(cmd_commands)
    };
}

void CommandParser::begin(const ModuleConfig& cfg) {
    const auto& config = static_cast<const ParserConfig&>(cfg);
    groups      = config.groups;
    group_count = config.group_count;
}

void CommandParser::loop() {
    if (!enabled) return;
}

void CommandParser::enable() {
    if (can_be_disabled) enabled = true;
}

void CommandParser::disable() {
    if (can_be_disabled) enabled = false;
}

void CommandParser::reset() {
    // No internal state to clear
}

std::string_view CommandParser::status() const {
    return enabled ? "enabled" : "disabled";
}

void CommandParser::print_help(const std::string& group_name) const {
    std::string target = group_name;
    std::transform(target.begin(), target.end(), target.begin(), ::tolower);

    for (size_t i = 0; i < group_count; ++i) {
        const auto& grp = groups[i];
        std::string name = grp.name;
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (target == name) {
            Serial.println("----------------------------------------");
            Serial.print(grp.name.c_str());
            Serial.println(" commands:");
            for (size_t j = 0; j < grp.commands.size(); ++j) {
                const auto& cmd = grp.commands[j];
                Serial.print("  $");
                Serial.print(grp.name.c_str());
                Serial.print(" ");
                Serial.print(cmd.name.c_str());
                int pad = 20 - int(grp.name.size() + cmd.name.size());
                for (int k = 0; k < pad; ++k) Serial.print(" ");
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
    for (size_t i = 0; i < group_count; ++i) {
        if (!groups[i].name.empty()) {
            print_help(groups[i].name);
        }
    }
    Serial.println("==================================");
}

void CommandParser::parse(std::string_view input_line) const {
    // Copy into mutable std::string
    std::string local(input_line.begin(), input_line.end());
    auto is_space = [](char c){ return std::isspace(static_cast<unsigned char>(c)); };

    // Trim whitespace
    size_t b = local.find_first_not_of(" \t\r\n"),
           e = local.find_last_not_of(" \t\r\n");
    if (b == std::string::npos) return;
    local = local.substr(b, e - b + 1);

    // Must start with $
    if (local.empty() || local[0] != '$') {
        Serial.println("Error: commands must start with '$'");
        return;
    }

    // Drop '$' and trim again
    local.erase(0,1);
    b = local.find_first_not_of(" \t\r\n");
    e = local.find_last_not_of(" \t\r\n");
    if (b == std::string::npos) local.clear();
    else                        local = local.substr(b, e - b + 1);

    // Split off group name
    size_t sp = local.find(' ');
    std::string group = (sp == std::string::npos) ? local : local.substr(0, sp);

    // Handle $help specially
    std::string gl = group;
    std::transform(gl.begin(), gl.end(), gl.begin(), ::tolower);
    if (gl == "help") {
        print_all_commands();
        return;
    }

    // Extract rest of line
    std::string rest = (sp == std::string::npos)
                       ? std::string()
                       : local.substr(sp+1);
    b = rest.find_first_not_of(" \t\r\n");
    e = rest.find_last_not_of(" \t\r\n");
    if (b == std::string::npos) rest.clear();
    else                        rest = rest.substr(b, e - b + 1);

    // Tokenize (supports quoted)
    struct Token { std::string value; bool quoted; };
    std::vector<Token> toks;
    size_t pos = 0;
    while (pos < rest.size()) {
        while (pos < rest.size() && is_space(rest[pos])) ++pos;
        if (pos >= rest.size()) break;

        bool quoted = false;
        std::string tok;
        if (rest[pos] == '"') {
            quoted = true;
            size_t q = rest.find('"', pos+1);
            if (q == std::string::npos) {
                Serial.println("Error: Unterminated quote in command.");
                return;
            }
            tok = rest.substr(pos+1, q-pos-1);
            pos = q+1;
        } else {
            size_t q = rest.find(' ', pos);
            if (q == std::string::npos) {
                tok = rest.substr(pos);
                pos = rest.size();
            } else {
                tok = rest.substr(pos, q-pos);
                pos = q;
            }
        }
        toks.push_back({tok, quoted});
    }

    // Separate cmd name and arguments
    std::string cmd;
    std::vector<Token> args;
    if (!toks.empty()) {
        cmd  = toks[0].value;
        args.assign(toks.begin()+1, toks.end());
    }

    // Lookup group
    for (size_t gi = 0; gi < group_count; ++gi) {
        const auto& grp = groups[gi];
        std::string name = grp.name;
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (gl == name) {
            // No subcommand → run first
            if (cmd.empty()) {
                if (!grp.commands.empty())
                    grp.commands[0].function(std::string_view{});
                else
                    Serial.println("Error: no commands in group.");
                return;
            }
            // Find matching command
            std::string cl = cmd;
            std::transform(cl.begin(), cl.end(), cl.begin(), ::tolower);
            for (const auto& c : grp.commands) {
                std::string cn = c.name;
                std::transform(cn.begin(), cn.end(), cn.begin(), ::tolower);
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
                    std::string rebuilt;
                    for (size_t ai = 0; ai < args.size(); ++ai) {
                        auto& tk = args[ai];
                        if (tk.quoted && tk.value.find(' ') != std::string::npos) {
                            rebuilt += '"'; rebuilt += tk.value; rebuilt += '"';
                        } else {
                            rebuilt += tk.value;
                        }
                        if (ai + 1 < args.size()) rebuilt += ' ';
                    }
                    c.function(std::string_view(rebuilt));
                    return;
                }
            }
            Serial.printf("Error: Unknown command '%s' in group '%s'\n",
                          cmd.c_str(), group.c_str());
            return;
        }
    }

    Serial.printf("Error: Unknown command group '%s'\n", group.c_str());
}