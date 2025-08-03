// src/Modules/Software/CommandParser/CommandParser.h
#pragma once

#include <Arduino.h>                     // for String
#include "../../Module.h"
#include "../../../Debug.h"
#include <vector>
#include <string>
#include <string_view>
#include <cstddef>

/// Configuration for the CommandParser module.
struct ParserConfig : public ModuleConfig {
    const CommandsGroup* groups      = nullptr;
    std::size_t         group_count = 0;
};

class CommandParser : public Module {
public:
    /// Construct with reference to the system controller.
    explicit CommandParser(SystemController& controller);

    /// Initialize with ParserConfig (cast from ModuleConfig).
    void begin(const ModuleConfig& cfg) override;

    /// Called repeatedly; `args` is the input line to parse.
    void loop(const std::string& args) override;

    void enable() override;
    void disable() override;
    void reset() override;

    /// Returns current status string.
    std::string_view status() const override;

    /// Helpers for printing usage.
    void print_help(const std::string& group_name) const;
    void print_all_commands() const;

private:
    /// Parse a single input line (Arduino String version).
    void parse(const String& line) const;

    /// Stored command groups from config.
    const CommandsGroup* groups_      = nullptr;
    std::size_t         group_count_ = 0;
};
