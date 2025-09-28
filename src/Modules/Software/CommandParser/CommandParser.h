// src/Modules/Software/CommandParser/CommandParser.h
#pragma once

#include "../../Module/Module.h"
#include "../../../Debug.h"
#include <vector>


struct ParserConfig : public ModuleConfig {
    const CommandsGroup*        groups                      = nullptr;
    std::size_t                 group_count                 = 0;
};


class CommandParser : public Module {
public:
    explicit                    CommandParser               (SystemController& controller);

    // required implementation
    void                        begin                       (const ModuleConfig& cfg)       override;
    void                        loop                        ()                              override;
    void                        reset                       (bool verbose=false)            override;

    // other methods
    void                        print_help                  (const std::string& group_name) const;
    void                        print_all_commands          ()                              const;
    void                        parse                       (std::string_view input_line)   const;

private:
    const CommandsGroup*        groups                      = nullptr;
    std::size_t                 group_count                 = 0;
    std::vector<Command>        cmd_commands;
};

