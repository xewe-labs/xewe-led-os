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
    std::size_t         group_count  = 0;
};

class CommandParser : public Module {
public:
    /// Construct with reference to the system controller.
    explicit CommandParser(SystemController& controller);

    /// Initialize with ParserConfig (cast from ModuleConfig).
    void begin(const ModuleConfig& cfg) override;

    /// Called repeatedly; reads any pending input.
    void loop() override;

    void enable() override;
    void disable() override;
    void reset() override;

    /// Returns current status string.
    std::string_view status() const override;

    /// Helpers for printing usage.
    void print_help(const std::string& group_name) const;
    void print_all_commands() const;

    /// Parse a single input line.
    void parse(const String& input_line) const;

private:
    /// Stored command groups from config.
    const CommandsGroup* groups       = nullptr;
    std::size_t         group_count   = 0;
};
