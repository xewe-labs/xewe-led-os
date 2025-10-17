/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/



// src/Interfaces/Homekit/Homekit.h
#pragma once

#include "../../Interface/Interface.h"
#include "../../../Config.h"
#include "../../../Debug.h"
#include "../../Hardware/LedStrip/LedModes/LedMode.h"

#include "HomeSpan.h"
#include <array>
#include <string>
#include <cmath>


struct HomekitConfig : public ModuleConfig {};


class Homekit : public Interface {
public:
    explicit                    Homekit              (SystemController& controller);

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
//    void                begin_routines_regular      (const ModuleConfig& cfg)       override;
//    void                begin_routines_common       (const ModuleConfig& cfg)       override;
//
    void                loop                        ()                                      override;
//
    void                reset                       (const bool verbose=false,
                                                     const bool do_restart=true)            override;
//
//    void                enable                      (const bool verbose=false)      override;
//    void                disable                     (const bool verbose=false)      override;
//
    std::string         status                      (const bool verbose=false)      const override;

//    bool                is_enabled                  (const bool verbose=false)      const override;
//    bool                is_disabled                 (const bool verbose=false)      const override;
//    bool                init_setup_complete         (const bool verbose=false)      const override;

    // other methods

private:
    static void status_callback(HS_STATUS s);

    // HomeSpan LightBulb service wired to SystemController
    struct NeoPixel_RGB : Service::LightBulb {
        Characteristic::On          power   {0,   true};
        Characteristic::Hue         H       {0,   true};
        Characteristic::Saturation  S       {0,   true};
        Characteristic::Brightness  V       {100, true};

        SystemController* controller;

        explicit NeoPixel_RGB(SystemController* ctrl);
        boolean update() override; // Homekit -> System
    };

    NeoPixel_RGB*       device                  = nullptr;
    static Homekit*     instance;
    uint8_t             hs_status               = 0;
};
