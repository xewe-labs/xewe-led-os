/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Interfaces/Hardware/LedStrip/ModeController/Modes/Mode/Mode.cpp

#include "Mode.h"

ModeConfig::ModeConfig (uint8_t init_id,
                        std::string init_name,
                        const std::vector<ModeParam>& custom_params)
    : id(init_id)
    , name(std::move(init_name))
{
    DBG_PRINTF(Mode, "ModeConfig(): Creating mode id=%u, name='%s' with %u custom params.\n",
               static_cast<unsigned>(id),
               name.c_str(),
               static_cast<unsigned>(custom_params.size()));

    params.insert(params.end(), custom_params.begin(), custom_params.end());

    DBG_PRINTF(Mode, "ModeConfig(): Mode '%s' now has %u params.\n",
               name.c_str(),
               static_cast<unsigned>(params.size()));
}

Mode::Mode (ModeConfig mode_config,
            const std::map<std::string, uint16_t>& params)
    : config(std::move(mode_config))
{
    DBG_PRINTF(Mode, "Mode(): Initializing mode id=%u, name='%s' with %u override params.\n",
               static_cast<unsigned>(config.id),
               config.name.c_str(),
               static_cast<unsigned>(params.size()));

    for (auto& param : config.params) {
        auto it = params.find(param.key);
        if (it != params.end()) {
            DBG_PRINTF(Mode, "Mode(): Overriding param '%s' from %u to %u.\n",
                       param.key.c_str(),
                       static_cast<unsigned>(param.default_value),
                       static_cast<unsigned>(it->second));
            param.default_value = it->second;
        } else {
            DBG_PRINTF(Mode, "Mode(): Keeping default param '%s' = %u.\n",
                       param.key.c_str(),
                       static_cast<unsigned>(param.default_value));
        }
    }

    DBG_PRINTF(Mode, "Mode(): Initialization complete for mode '%s'.\n",
               config.name.c_str());
}

uint8_t Mode::get_id () const {
    DBG_PRINTF(Mode, "get_id(): Returning id=%u for mode '%s'.\n",
               static_cast<unsigned>(config.id),
               config.name.c_str());
    return config.id;
}

std::string_view Mode::get_name () const {
    DBG_PRINTF(Mode, "get_name(): Returning name='%s'.\n",
               config.name.c_str());
    return config.name;
}

std::vector<ModeParam> Mode::get_params () const {
    DBG_PRINTF(Mode, "get_params(): Returning %u params for mode '%s'.\n",
               static_cast<unsigned>(config.params.size()),
               config.name.c_str());
    return config.params;
}

const ModeConfig& Mode::get_config () const {
    DBG_PRINTF(Mode, "get_config(): Returning config for mode id=%u, name='%s'.\n",
               static_cast<unsigned>(config.id),
               config.name.c_str());
    return config;
}

uint16_t Mode::get_param (std::string_view key) const {
//    DBG_PRINTF(Mode, "get_param(): Looking up key='%.*s' in mode '%s'.\n",
//               static_cast<int>(key.size()),
//               key.data(),
//               config.name.c_str());

    for (const auto& p : config.params) {
        if (p.key == key) {
//            DBG_PRINTF(Mode, "get_param(): Found key='%s', value=%u.\n",
//                       p.key.c_str(),
//                       static_cast<unsigned>(p.default_value));
            return p.default_value;
        }
    }

//    DBG_PRINTF(Mode, "get_param(): Key='%.*s' not found, returning 0.\n",
//               static_cast<int>(key.size()),
//               key.data());
    return 0;
}