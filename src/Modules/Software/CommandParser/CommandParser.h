// src/Modules/Software/CommandParser/CommandParser.h
#pragma once

#include "../../Module.h"
#include "../../../Debug.h"

#include <vector>
#include <string_view>

struct ParserConfig : public ModuleConfig {
    const CommandsGroup* groups     = nullptr;
    std::size_t         group_count = 0;
};

class CommandParser : public Module {
public:
    explicit CommandParser(SystemController& controller);

    void begin(const ModuleConfig& cfg) override;
    void loop() override;
    void enable() override;
    void disable() override;
    void reset() override;
    std::string_view status() const override;

    void print_help(const std::string& group_name) const;
    void print_all_commands() const;
    void parse(std::string_view input_line) const;

    const CommandsGroup& get_command_group() const { return commands_group; }

private:
    const CommandsGroup*       groups     = nullptr;
    std::size_t                group_count = 0;
    std::vector<Command>       cmd_commands;
};
