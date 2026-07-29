// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Software/System/System.h
#pragma once

#include <esp_system.h>
#include <esp_chip_info.h>
#include <esp_mac.h>
#include <mbedtls/sha256.h>

#include "../../Module/Module.h"


struct SystemConfig : public ModuleConfig {};


class System : public Module {
public:
    explicit                    System                      (ModuleController& controller);

    void                        begin_routines_required     (const ModuleConfig& cfg)       override;
    void                        begin_routines_init         (const ModuleConfig& cfg)       override;
    void                        reset                       (const bool verbose=false,
                                                             const bool do_restart=true,
                                                             const bool keep_enabled=true)    override;
    std::string                 status                      (const bool verbose=false)      const override;

    std::string                 get_device_name             ();
    void                        restart                     (uint16_t delay_ms=1000);
};
