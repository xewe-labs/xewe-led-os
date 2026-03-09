/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/
// src/Interfaces/Software/HomeKit/HomeKit.h

#pragma once

#include "HomeSpan.h"
#include <cmath>

#include "../../Interface/Interface.h"
#include "../../Hardware/LedStrip/ModeController/ModeController.h"

struct HomeKitConfig : public ModuleConfig {};

class HomeKit : public Interface {
public:
    explicit                    HomeKit              (SystemController& controller);

    // required implementation
    void                        sync_color                  (std::array<uint8_t,3> color)   override;
    void                        sync_brightness             (uint8_t brightness)            override;
    void                        sync_state                  (uint8_t state)                 override;
    void                        sync_mode                   (uint8_t mode)                  override;
    void                        sync_length                 (uint16_t length)               override;

    // optional implementation
    void                        sync_all                    (std::array<uint8_t,3> color,
                                                             uint8_t brightness,
                                                             uint8_t state,
                                                             uint8_t mode,
                                                             uint16_t length)               override;

    void                        begin_routines_required     (const ModuleConfig& cfg)       override;
    void                        begin_routines_init         (const ModuleConfig& cfg)       override;
    void                        loop                        ()                              override;
    void                        reset                       (const bool verbose=false,
                                                             const bool do_restart=true,
                                                             const bool keep_enabled=true)  override;

    string                      status                      (const bool verbose=false)      const override;

private:
    static void status_callback(HS_STATUS s);

    struct NeoPixel_RGB : Service::LightBulb {
        Characteristic::On          power   {0,   true};
        Characteristic::Hue         H       {0,   true};
        Characteristic::Saturation  S       {0,   true};
        Characteristic::Brightness  V       {100, true};

        SystemController* controller;

        explicit NeoPixel_RGB(SystemController* ctrl);
        boolean update() override;
    };

    NeoPixel_RGB*       device                  = nullptr;
    static HomeKit*     instance;
    uint8_t             hs_status               = 0;
};
