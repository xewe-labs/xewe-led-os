/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/



// src/Modules/Software/CommandParser/CommandParser.h
#pragma once

#include "../../Module/Module.h"
#include "../../../Config.h"
#include "../../../Debug.h"


struct CommandParserConfig : public ModuleConfig {
    const CommandsGroup*        groups                      = nullptr;
    std::size_t                 group_count                 = 0;
};


class CommandParser: public Module {
public:
    explicit                    CommandParser               (SystemController& controller);

    void                        begin_routines_required     (const ModuleConfig& cfg)       override;

    // other methods
    void                        print_help                  (const std::string& group_name) const;
    void                        print_all_commands          ()                              const;
    void                        parse                       (std::string_view input_line)   const;

private:
    const CommandsGroup*        groups                      = nullptr;
    std::size_t                 group_count                 = 0;
    std::vector<Command>        cmd_commands;
};
