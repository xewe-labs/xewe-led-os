// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Software/CommandExecutor/CommandExecutor.cpp

#include "CommandExecutor.h"
#include "../../Module/ModuleController.h"


CommandExecutor::CommandExecutor(ModuleController& controller)
      : Module(controller,
               /* id                  */ "cmd",
               /* name                */ "Command Executor",
               /* description         */ "Executes commands",
               /* requires_init_setup */ false,
               /* can_be_disabled     */ false,
               /* has_cli_cmds        */ false)
{}

void CommandExecutor::loop() {
    if (controller.serial_port.has_line()) {
        parse(controller.serial_port.read_line());
    }
}

void CommandExecutor::print_help(std::string_view module_id) const {
    const std::string id = trim_copy(module_id);

    if (id.empty()) {
        print_all_commands();
        return;
    }

    Module* module = controller.get_module(id);

    if (module == nullptr) {
        controller.serial_port.printf(
            "Error: Unknown command group '%s'\r\n",
            id.c_str()
        );
        return;
    }

    const auto commands = module->get_commands();

    if (commands.empty()) {
        controller.serial_port.printf(
            "Error: Command group '%s' has no CLI commands\r\n",
            id.c_str()
        );
        return;
    }

    std::vector<std::vector<std::string_view>> table_data;
    table_data.push_back({"Command", "Args", "Description", "Sample Usage"});

    std::vector<std::string> arg_counts;
    arg_counts.reserve(commands.size());

    for (const Command& command : commands) {
        if (command.name.empty() || !command.function) {
            continue;
        }

        arg_counts.push_back(std::to_string(command.arg_count));

        table_data.push_back({
            command.name,
            arg_counts.back(),
            command.description,
            command.sample_usage
        });
    }

    const std::string header =
        std::string(module->get_name()) +
        " Commands [" +
        std::string(module->get_id()) +
        "]";

    controller.serial_port.print_table(table_data, header);
}

void CommandExecutor::print_all_commands() const {
    for (const auto& [id, module] : controller.get_modules()) {
        if (module && !module->get_commands().empty()) {
            print_help(id);
        }
    }
}

void CommandExecutor::parse(std::string_view input_line) const {
    std::string local = trim_copy(input_line);

    if (local.empty()) return;

    if (local[0] != '$') {
        controller.serial_port.print(
            "Error: commands must start with '$'; type $help",
            xewe::str::kCRLF
        );
        return;
    }

    local.erase(0, 1);
    local = trim_copy(local);

    if (local.empty()) {
        controller.serial_port.print(
            "Error: Missing command group; usage: $<group> <command> [args...]",
            xewe::str::kCRLF
        );
        return;
    }

    std::vector<std::string> tokens;

    if (!tokenize(local, tokens)) return;

    if (tokens.empty()) {
        controller.serial_port.print(
            "Error: Missing command group; usage: $<group> <command> [args...]",
            xewe::str::kCRLF
        );
        return;
    }

    if (lower_copy(tokens[0]) == "help") {
        if (tokens.size() == 1) {
            print_all_commands();
            return;
        }

        if (tokens.size() != 2) {
            controller.serial_port.print(
                "Error: Argument count mismatch for '$help'; usage: $help <group>",
                xewe::str::kCRLF
            );
            return;
        }

        print_help(tokens[1]);
        return;
    }

    Module* module = controller.get_module(tokens[0]);

    if (module == nullptr) {
        controller.serial_port.printf(
            "Error: Unknown command group '%s'\r\n",
            tokens[0].c_str()
        );
        return;
    }

    const auto commands = module->get_commands();

    if (commands.empty()) {
        controller.serial_port.printf(
            "Error: Command group '%s' has no CLI commands\r\n",
            tokens[0].c_str()
        );
        return;
    }

    if (tokens.size() == 1) {
        print_help(tokens[0]);
        return;
    }

    if (tokens[1].empty()) {
        controller.serial_port.printf(
            "Error: Missing command in command group '%s'; usage: $%s <command> [args...]\r\n",
            tokens[0].c_str(),
            tokens[0].c_str()
        );
        return;
    }

    if (lower_copy(tokens[1]) == "help") {
        print_help(tokens[0]);
        return;
    }

    const std::string command_name = lower_copy(tokens[1]);
    const Command* matched_command = nullptr;

    for (const Command& command : commands) {
        if (command.name.empty() || !command.function) {
            continue;
        }

        if (lower_copy(command.name) == command_name) {
            matched_command = &command;
            break;
        }
    }

    if (matched_command == nullptr) {
        controller.serial_port.printf(
            "Error: Unknown command '%s' in command group '%s'\r\n",
            tokens[1].c_str(),
            tokens[0].c_str()
        );
        return;
    }

    const std::size_t provided_arg_count = tokens.size() - 2;
    const std::size_t expected_arg_count = matched_command->arg_count;

    if (provided_arg_count != expected_arg_count) {
        controller.serial_port.printf(
            "Error: Argument count mismatch for '$%s %s'; expected %u, got %u\r\n",
            tokens[0].c_str(),
            tokens[1].c_str(),
            static_cast<unsigned>(expected_arg_count),
            static_cast<unsigned>(provided_arg_count)
        );

        if (!matched_command->sample_usage.empty()) {
            controller.serial_port.printf(
                "Usage: %s\r\n",
                std::string(matched_command->sample_usage).c_str()
            );
        }

        return;
    }

    std::vector<std::string> recipients;
    recipients.push_back(tokens[0]);

    std::vector<std::string> args;
    args.reserve(provided_arg_count);

    for (std::size_t i = 2; i < tokens.size(); ++i) {
        args.push_back(tokens[i]);
    }

    controller.send_command(
        std::span<const std::string>(recipients.data(), recipients.size()),
        matched_command->name,
        std::span<const std::string>(args.data(), args.size())
    );
}

std::string CommandExecutor::trim_copy(std::string_view value) {
    const auto is_space = [](unsigned char c) {
        return std::isspace(c) != 0;
    };

    std::size_t begin = 0;
    std::size_t end = value.size();

    while (begin < end && is_space(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }

    while (end > begin && is_space(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }

    return std::string(value.substr(begin, end - begin));
}

std::string CommandExecutor::lower_copy(std::string_view value) {
    std::string out(value.begin(), value.end());

    std::transform(
        out.begin(),
        out.end(),
        out.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        }
    );

    return out;
}

bool CommandExecutor::tokenize(std::string_view input, std::vector<std::string>& out) const {
    out.clear();

    std::size_t pos = 0;

    while (pos < input.size()) {
        while (pos < input.size() &&
               std::isspace(static_cast<unsigned char>(input[pos])) != 0) {
            ++pos;
        }

        if (pos >= input.size()) {
            break;
        }

        std::string token;

        if (input[pos] == '"') {
            ++pos;

            bool closed = false;
            bool escape = false;

            while (pos < input.size()) {
                const char c = input[pos++];

                if (escape) {
                    token.push_back(c);
                    escape = false;
                    continue;
                }

                if (c == '\\') {
                    escape = true;
                    continue;
                }

                if (c == '"') {
                    closed = true;
                    break;
                }

                token.push_back(c);
            }

            if (escape) {
                token.push_back('\\');
            }

            if (!closed) {
                controller.serial_port.print(
                    "Error: Unterminated quote in command.",
                    xewe::str::kCRLF
                );
                return false;
            }
        } else {
            while (pos < input.size() &&
                   std::isspace(static_cast<unsigned char>(input[pos])) == 0) {
                token.push_back(input[pos++]);
            }
        }

        out.push_back(token);
    }

    return true;
}