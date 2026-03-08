/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Interfaces/Hardware/LedStrip/ModeController/ModeRegistry/ModeRegistry.h

#pragma once

#include <map>
#include <memory>
#include <string>

#include "../Modes/Mode/Mode.h"

using ModeFactory = std::function<std::unique_ptr<Mode>(const std::map<std::string, uint16_t>&)>;

class ModeRegistry {
public:
    static std::map<uint8_t, ModeFactory>& get_registry() {
        static std::map<uint8_t, ModeFactory> registry;
        return registry;
    }

    static void register_mode(uint8_t id, ModeFactory factory) {
        get_registry()[id] = factory;
    }
};

template<typename T>
class ModeRegistrar {
public:
    ModeRegistrar(uint8_t id) {
        ModeRegistry::register_mode(id, [](const std::map<std::string, uint16_t>& params) {
            return std::make_unique<T>(params);
        });
    }
};