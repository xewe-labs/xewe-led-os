// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Software/CommandExecutor/CommandExecutor.h
#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <cctype>
#include <span>

#include "../../Module/Module.h"


struct CommandExecutorConfig : public ModuleConfig {};


class CommandExecutor : public Module {
public:
    explicit                    CommandExecutor             (ModuleController& controller);

    void                        loop                        ()                              override;

    void                        parse                       (std::string_view input_line)   const;
    void                        print_help                  (std::string_view id)           const;
    void                        print_all_commands          ()                              const;

private:
    static std::string          trim_copy                   (std::string_view value);
    static std::string          lower_copy                  (std::string_view value);

    bool                        tokenize                    (std::string_view input,
                                                             std::vector<std::string>& out) const;
};