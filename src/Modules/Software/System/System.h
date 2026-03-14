/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Modules/Software/System/System.h
#pragma once

#include "../../Module/Module.h"

#include <esp_system.h>
#include <esp_chip_info.h>
#include <esp_mac.h>
#include <mbedtls/sha256.h>


struct SystemConfig : public ModuleConfig {};


class System : public Module {
public:
    explicit                    System                      (SystemController& controller);

    void                        begin_routines_required     (const ModuleConfig& cfg)       override;
    void                        begin_routines_init         (const ModuleConfig& cfg)       override;
    void                        reset                       (const bool verbose=false,
                                                             const bool do_restart=true,
                                                             const bool keep_enabled=true)    override;
    string                      status                      (const bool verbose=false)      const override;

    std::string                 get_device_name             ();
    void                        restart                     (uint16_t delay_ms=1000);
};

